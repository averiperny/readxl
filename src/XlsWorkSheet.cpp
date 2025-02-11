#include <unistd.h>
#include <sys/time.h>
#include "ColSpec.h"
#include "XlsWorkSheet.h"
#include "libxls/xls.h"

[[cpp11::register]]
cpp11::list read_xls_(cpp11::strings path, int sheet_i,
               cpp11::integers limits, bool shim,
               cpp11::sexp col_names, cpp11::sexp col_types,
               std::vector<std::string> na, bool trim_ws,
               int guess_max = 1000, bool progress = true) {

  std::string filename(Rf_translateChar(STRING_ELT(path, 0)));
  // Construct worksheet ----------------------------------------------
  XlsWorkSheet ws(filename, sheet_i, limits, shim, progress);

  // catches empty sheets and sheets where requested rectangle contains no data
  if (ws.nrow() == 0 && ws.ncol() == 0) {
    using namespace cpp11::literals;
    return cpp11::writable::list (0_xl);
  }

  // Get column names -------------------------------------------------
  cpp11::writable::strings colNames;
  bool has_col_names = false;
  switch(TYPEOF(col_names)) {
  case STRSXP:
    colNames = cpp11::writable::strings(static_cast<SEXP>(col_names));
    break;
  case LGLSXP:
    has_col_names = cpp11::as_cpp<bool>(col_names);
    colNames = has_col_names ? ws.colNames(na, trim_ws) : cpp11::writable::strings(ws.ncol());
    break;
  default:
    cpp11::stop("`col_names` must be a logical or character vector");
  }

  // Get column types --------------------------------------------------
  if (TYPEOF(col_types) != STRSXP) {
    cpp11::stop("`col_types` must be a character vector");
  }
  std::vector<ColType> colTypes = colTypeStrings(static_cast<SEXP>(col_types));
  colTypes = recycleTypes(colTypes, ws.ncol());
  if ((int) colTypes.size() != ws.ncol()) {
    cpp11::stop("Sheet %d has %d columns, but `col_types` has length %d.",
               sheet_i + 1, ws.ncol(), colTypes.size());
  }
  if (requiresGuess(colTypes)) {
    colTypes = ws.colTypes(colTypes, na, trim_ws, guess_max, has_col_names);
  }
  colTypes = finalizeTypes(colTypes);

  // Reconcile column names and types ----------------------------------
  colNames = reconcileNames(colNames, colTypes, sheet_i);

  // Get data ----------------------------------------------------------
  return ws.readCols(colNames, colTypes, na, trim_ws, has_col_names);
}
