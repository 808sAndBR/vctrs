#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

#include <stdbool.h>
#include "hash.h"

bool is_data_frame(SEXP x) {
  return TYPEOF(x) == VECSXP && Rf_inherits(x, "data.frame");
}

R_len_t vec_length(SEXP x) {
  if (!is_data_frame(x)) {
    return Rf_length(x);
  }

  // Automatically generates integer vector of correct length
  SEXP rn = Rf_getAttrib(x, R_RowNamesSymbol);
  switch(TYPEOF(rn)) {
  case INTSXP:
  case STRSXP:
    return Rf_length(rn);
  default:
    Rf_errorcall(R_NilValue, "Corrupt data frame: invalid row names");
  }
}

// boost::hash_combine from https://stackoverflow.com/questions/35985960
int32_t hash_combine(int x, int y) {
  return x ^ y + 0x9e3779b9 + (x << 6) + (x >> 2);
}

// same approach as java 7
// https://docs.oracle.com/javase/7/docs/api/java/lang/Double.html#hashCode()
int32_t hash_double(double x) {
  union {
    double d;
    uint32_t i[2];
  } value;
  value.d = x;

  return value.i[0] ^ value.i[1];
}

// https://github.com/attractivechaos/klib/blob/master/khash.h#L385
int32_t hash_int64(int64_t x) {
  return x >> 33 ^ x ^ x << 11;
}

int32_t hash_scalar(SEXP x, R_len_t i) {
  switch(TYPEOF(x)) {
  // Vector types ----------------------------------------------------------
  case LGLSXP:
    return LOGICAL(x)[i];
  case INTSXP:
    return INTEGER(x)[i];
  case REALSXP: {
    double val = REAL(x)[i];
    // Hash all NAs and NaNs to same value (i.e. ignoring significand)
    if (R_IsNA(val))
      val = NA_REAL;
    else if (R_IsNaN(val))
      val = R_NaN;

    return hash_double(val);
  }
  case STRSXP: {
    // currently assuming 64-bit pointer size
    return hash_int64((intptr_t) STRING_ELT(x, i));
  }
  case VECSXP: {
    if (is_data_frame(x)) {
      uint32_t hash = 0;

      int p = Rf_length(x);
      for (int j = 0; j < p; ++j) {
        SEXP col = VECTOR_ELT(x, j);
        hash = hash_combine(hash, hash_scalar(col, i));
      }
      return hash;
    } else {
      return hash_object(VECTOR_ELT(x, i));
    }
  }

  default:
    Rf_errorcall(R_NilValue, "Unsupported type %s", Rf_type2char(TYPEOF(x)));
  }
}

int32_t hash_object(SEXP x) {
  R_len_t n = vec_length(x);
  int32_t hash = 0;

  switch(TYPEOF(x)) {
  case NILSXP:
    break;

  case LGLSXP:
  case INTSXP:
  case REALSXP:
  case STRSXP:
  case VECSXP:
    for (R_len_t i = 0; i < n; ++i) {
      hash = hash_combine(hash, hash_scalar(x, i));
    }
    break;

  case SYMSXP:
    hash = hash_object(PRINTNAME(x));
    break;
  case DOTSXP:
  case LANGSXP:
  case LISTSXP:
  case BCODESXP:
    hash = hash_combine(hash, hash_object(CAR(x)));
    hash = hash_combine(hash, hash_object(CDR(x)));
    break;
  case CLOSXP:
    hash = hash_combine(hash, hash_object(BODY(x)));
    hash = hash_combine(hash, hash_object(CLOENV(x)));
    hash = hash_combine(hash, hash_object(FORMALS(x)));
    break;

  case SPECIALSXP:
  case BUILTINSXP:
  case CHARSXP:
  case ENVSXP:
  case EXTPTRSXP:
    hash = hash_int64((intptr_t) x);
    break;

  default:
    Rf_errorcall(R_NilValue, "Unsupported type %s", Rf_type2char(TYPEOF(x)));
  }


  return hash;
}

// Equality --------------------------------------------------------------------

bool equal_scalar(SEXP x, int i, SEXP y, int j) {
  switch(TYPEOF(x)) {
  case LGLSXP:
    return LOGICAL(x)[i] == LOGICAL(y)[j];
  case INTSXP:
    return INTEGER(x)[i] == INTEGER(y)[j];
  case REALSXP: {
    double xi = REAL(x)[i], yj = REAL(y)[j];
    if (R_IsNA(xi)) return R_IsNA(yj);
    if (R_IsNaN(xi)) return R_IsNaN(yj);
    return xi == yj;
  }
  case STRSXP:
    // Ignoring encoding for now
    return STRING_ELT(x, i) == STRING_ELT(y, j);
  case VECSXP:
    if (is_data_frame(x)) {
      int p = Rf_length(x);
      if (p != Rf_length(y))
        return false; // shouldn't happen because types are enforced in R

      for (int k = 0; k < p; ++k) {
        SEXP col_x = VECTOR_ELT(x, k);
        SEXP col_y = VECTOR_ELT(y, k);
        if (!equal_scalar(col_x, i, col_y, j))
          return false;
      }
      return true;

    } else {
      return R_compute_identical(VECTOR_ELT(x, i), VECTOR_ELT(y, j), 16);
    }
    // Need to add support for data frame

  default:
    Rf_errorcall(R_NilValue, "Unsupported type %s", Rf_type2char(TYPEOF(x)));
  }
}

// R interface -----------------------------------------------------------------

SEXP vctrs_hash(SEXP x) {
  R_len_t n = vec_length(x);
  SEXP out = PROTECT(Rf_allocVector(INTSXP, n));

  int32_t* pOut = INTEGER(out);
  for (R_len_t i = 0; i < n; ++i) {
    pOut[i] = hash_scalar(x, i);
  }

  UNPROTECT(1);
  return out;
}

SEXP vctrs_hash_object(SEXP x) {
  return Rf_ScalarInteger(hash_object(x));
}

SEXP vctrs_equal(SEXP x, SEXP y) {
  if (TYPEOF(x) != TYPEOF(y))
    Rf_errorcall(R_NilValue, "`x` and `y` must have same types");
  if (vec_length(y) != 1) {
    Rf_errorcall(R_NilValue, "`y` must have length 1");
  }

  R_len_t n = vec_length(x);
  SEXP out = PROTECT(Rf_allocVector(LGLSXP, n));
  int32_t* p_out = LOGICAL(out);

  for (R_len_t i = 0; i < n; ++i) {
    p_out[i] = equal_scalar(x, i, y, 0);
  }

  UNPROTECT(1);
  return out;
}