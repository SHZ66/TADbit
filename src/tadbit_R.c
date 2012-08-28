#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>

#include "tadbit.h"

// Declare and register R/C interface.
SEXP
tadbit_R_call(
  SEXP list,
  //SEXP max_tad_size,
  SEXP n_threads,
  SEXP verbose,
  SEXP speed
);

R_CallMethodDef callMethods[] = {
   {"tadbit_R_call", (DL_FUNC) &tadbit_R_call, 4},
   {NULL, NULL, 0}
};

void R_init_tadbit(DllInfo *info) {
   R_registerRoutines(info, NULL, callMethods, NULL, NULL);
}


SEXP
tadbit_R_call(
  SEXP list,
  //SEXP max_tad_size,
  SEXP n_threads,
  SEXP verbose,
  SEXP speed
){

/*
   * This is a tadbit wrapper for R. The matrices have to passed
   * in a list (in R). Checks that the input consists of numeric
   * square matrices, with identical dimensions. The list is
   * is converted to pointer of pointers to doubles and passed
   * to 'tadbit'.
   * Assume that NAs can be passed from R and are ignored in the
   * computation.
*/

   R_len_t i, m = length(list);
   int first = 1, n, *dim_int;

   SEXP dim;
   PROTECT(dim = allocVector(INTSXP, 2));

   // Convert 'obs_list' to pointer of pointer to double.
   double **obs = (double **) malloc(m * sizeof(double **));
   for (i = 0 ; i < m ; i++) {
      // This fails is list element is not numeric.
      obs[i] = REAL(coerceVector(VECTOR_ELT(list, i), REALSXP));
      // Check that input is a matrix.
      if (!isMatrix(VECTOR_ELT(list, i))) {
         error("input must be square matrix");
      }
      // Check the dimension.
      dim = getAttrib(VECTOR_ELT(list, i), R_DimSymbol);
      dim_int = INTEGER(dim);
      if (dim_int[0] != dim_int[1]) {
         error("input must be square matrix");
      }
      if (first) {
         n = dim_int[0];
         first = 0;
      }
      else {
         if (n != dim_int[0]) {
            error("all matrices must have same dimensions");
         }
      }
   }

   UNPROTECT(1);

   tadbit_output *seg = (tadbit_output *) malloc(sizeof(tadbit_output));
   
   // Call 'tadbit'.
   tadbit(obs, n, m, /*REAL(max_tad_size)[0],*/ INTEGER(n_threads)[0],
         INTEGER(verbose)[0], INTEGER(speed)[0], seg);

   // Copy output to R-readable variables.
   SEXP nbreaks_SEXP;
   SEXP llikmat_SEXP;
   SEXP mllik_SEXP;
   SEXP bkpts_SEXP;

   PROTECT(nbreaks_SEXP = allocVector(INTSXP, 1));
   PROTECT(llikmat_SEXP = allocVector(REALSXP, n*n));
   PROTECT(mllik_SEXP = allocVector(REALSXP, n/4));
   PROTECT(bkpts_SEXP = allocVector(INTSXP, n*(n/4-1)));

   int *nbreaks_opt = INTEGER(nbreaks_SEXP); 
   double *llikmat = REAL(llikmat_SEXP);
   double *mllik = REAL(mllik_SEXP);
   int *bkpts = INTEGER(bkpts_SEXP);

   nbreaks_opt[0] = seg->nbreaks_opt;
   for (i = 0 ; i < n*n ; i++) llikmat[i] = seg->llikmat[i];
   for (i = 0 ; i < n/4 ; i++) mllik[i] = seg->mllik[i];
   // Remove first column associated with 0 breaks. Itcontains only
   // 0s and shifts the index in R (vectors start at position 1).
   for (i = n ; i < n*n/4 ; i++) bkpts[i-n] = seg->bkpts[i];


   // Set 'dim' attributes.
   SEXP dim_llikmat;
   PROTECT(dim_llikmat = allocVector(INTSXP, 2));
   INTEGER(dim_llikmat)[0] = n;
   INTEGER(dim_llikmat)[1] = n;
   setAttrib(llikmat_SEXP, R_DimSymbol, dim_llikmat);

   SEXP dim_breaks;
   PROTECT(dim_breaks = allocVector(INTSXP, 2));
   INTEGER(dim_breaks)[0] = n;
   INTEGER(dim_breaks)[1] = n/4-1;
   setAttrib(bkpts_SEXP, R_DimSymbol, dim_breaks);

   free_tadbit_output(seg);

   SEXP list_SEXP;
   PROTECT(list_SEXP = allocVector(VECSXP, 4));
   SET_VECTOR_ELT(list_SEXP, 0, nbreaks_SEXP);
   SET_VECTOR_ELT(list_SEXP, 1, llikmat_SEXP);
   SET_VECTOR_ELT(list_SEXP, 2, mllik_SEXP);
   SET_VECTOR_ELT(list_SEXP, 3, bkpts_SEXP);
   UNPROTECT(7);

   return list_SEXP;

}

