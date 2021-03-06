#pragma once

#include <algorithm>            // for min
#include <array>                // for array
#include <initializer_list>     // for initializer_list
#include <stdexcept>            // for out_of_range
#include "cpp11/as.hpp"         // for as_sexp
#include "cpp11/named_arg.hpp"  // for named_arg
#include "cpp11/protect.hpp"    // for INTEGER_ELT, protect_sexp, Rf_a...
#include "cpp11/vector.hpp"     // for vector, vector<>::proxy, vector<>::...

// Specializations for integers

namespace cpp11 {

template <>
inline SEXP vector<int>::valid_type(SEXP data) {
  if (TYPEOF(data) != INTSXP) {
    throw type_error(INTSXP, TYPEOF(data));
  }
  return data;
}

template <>
inline int vector<int>::operator[](const R_xlen_t pos) const {
  // NOPROTECT: likely too costly to unwind protect every elt
  return is_altrep_ ? INTEGER_ELT(data_, pos) : data_p_[pos];
}

template <>
inline int vector<int>::at(const R_xlen_t pos) const {
  if (pos < 0 || pos >= length_) {
    throw std::out_of_range("integers");
  }
  // NOPROTECT: likely too costly to unwind protect every elt
  return is_altrep_ ? INTEGER_ELT(data_, pos) : data_p_[pos];
}

template <>
inline int* vector<int>::get_p(bool is_altrep, SEXP data) {
  if (is_altrep) {
    return nullptr;
  } else {
    return INTEGER(data);
  }
}

template <>
inline void vector<int>::const_iterator::fill_buf(R_xlen_t pos) {
  length_ = std::min(64_xl, data_->size() - pos);
  unwind_protect([&] { INTEGER_GET_REGION(data_->data_, pos, length_, buf_.data()); });
  block_start_ = pos;
}

typedef vector<int> integers;

namespace writable {

template <>
inline typename vector<int>::proxy& vector<int>::proxy::operator=(int rhs) {
  if (is_altrep_) {
    // NOPROTECT: likely too costly to unwind protect every set elt
    SET_INTEGER_ELT(data_, index_, rhs);
  } else {
    *p_ = rhs;
  }
  return *this;
}

template <>
inline vector<int>::proxy::operator int() const {
  if (p_ == nullptr) {
    // NOPROTECT: likely too costly to unwind protect every elt
    return INTEGER_ELT(data_, index_);
  } else {
    return *p_;
  }
}

template <>
inline vector<int>::vector(std::initializer_list<int> il)
    : cpp11::vector<int>(as_sexp(il)), capacity_(il.size()) {}

template <>
inline void vector<int>::reserve(R_xlen_t new_capacity) {
  data_ = data_ == R_NilValue ? safe[Rf_allocVector](INTSXP, new_capacity)
                              : safe[Rf_xlengthgets](data_, new_capacity);
  SEXP old_protect = protect_;

  // Protect the new data
  protect_ = protect_sexp(data_);

  // Release the old protection;
  release_protect(old_protect);

  data_p_ = INTEGER(data_);
  capacity_ = new_capacity;
}

template <>
inline vector<int>::vector(std::initializer_list<named_arg> il)
    : cpp11::vector<int>(safe[Rf_allocVector](INTSXP, il.size())), capacity_(il.size()) {
  try {
    unwind_protect([&] {
      protect_ = protect_sexp(data_);
      attr("names") = Rf_allocVector(STRSXP, capacity_);
      SEXP names = attr("names");
      auto it = il.begin();
      for (R_xlen_t i = 0; i < capacity_; ++i, ++it) {
        data_p_[i] = integers(it->value())[0];
        SET_STRING_ELT(names, i, Rf_mkCharCE(it->name(), CE_UTF8));
      }
    });
  } catch (const unwind_exception& e) {
    release_protect(protect_);
    throw e;
  }
}

template <>
inline void vector<int>::push_back(int value) {
  while (length_ >= capacity_) {
    reserve(capacity_ == 0 ? 1 : capacity_ *= 2);
  }
  if (is_altrep_) {
    // NOPROTECT: likely too costly to unwind protect every elt
    SET_INTEGER_ELT(data_, length_, value);
  } else {
    data_p_[length_] = value;
  }
  ++length_;
}

typedef vector<int> integers;

}  // namespace writable

}  // namespace cpp11
