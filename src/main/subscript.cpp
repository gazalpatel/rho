/*CXXR $Id$
 *CXXR
 *CXXR This file is part of CXXR, a project to refactor the R interpreter
 *CXXR into C++.  It may consist in whole or in part of program code and
 *CXXR documentation taken from the R project itself, incorporated into
 *CXXR CXXR (and possibly MODIFIED) under the terms of the GNU General Public
 *CXXR Licence.
 *CXXR 
 *CXXR CXXR is Copyright (C) 2008-12 Andrew R. Runnalls, subject to such other
 *CXXR copyrights and copyright restrictions as may be stated below.
 *CXXR 
 *CXXR CXXR is not part of the R project, and bugs and other issues should
 *CXXR not be reported via r-bugs or other R project channels; instead refer
 *CXXR to the CXXR website.
 *CXXR */

/*
 *  R : A Computer Language for Statistical Data Analysis
 *  Copyright (C) 1995, 1996  Robert Gentleman and Ross Ihaka
 *  Copyright (C) 1997--2010  The R Development Core Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 *
 * EXPORTS:
 *
 *  OneIndex()        -- used for "[[<-" in ./subassign.c
 *  get1index()       -- used for "[["   in ./subassign.c & subset.c
 *  vectorIndex()     -- used for "[[" with a vector arg

 *  mat2indsub()      -- for "mat[i]"     "    "            "

 *  makeSubscript()   -- for "[" and "[<-" in ./subset.c and ./subassign.c,
 *			 and "[[<-" with a scalar in ./subassign.c
 *  vectorSubscript() -- for makeSubscript()   {currently unused externally}
 *  arraySubscript()  -- for "[i,j,..." and "[<-..." in ./subset.c, ./subassign.c
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Defn.h>

#include "CXXR/GCStackRoot.hpp"
#include "CXXR/Subscripting.hpp"

using namespace std;
using namespace CXXR;

/* We might get a call with R_NilValue from subassignment code */
#define ECALL(call, yy) if(call == R_NilValue) error(yy); else errorcall(call, yy);

// Note by arr after analysing code: If x is a vector of length 2,
// then x[[-1]] and x[[-2]] are legal, and mean the same as x[[2]] and
// x[[1]] respectively.  Otherwise a negative index is illegal for [[,
// as is a zero index.  The following function looks after this, as
// well as rebasing indices off 0 rather than 1.

static int integerOneIndex(int i, int len, SEXP call)
{
    int indx = -1;

    if (i > 0)
	indx = i - 1;
    else if (i == 0 || len < 2) {
	ECALL(call, _("attempt to select less than one element"));
    } else if (len == 2 && i > -3)
	indx = 2 + i;
    else {
	ECALL(call, _("attempt to select more than one element"));
    }
    return(indx);
}

/* Utility used (only in) do_subassign2_dflt(), i.e. "[[<-" in ./subassign.c : */
int attribute_hidden
OneIndex(SEXP x, SEXP s, int len, int partial, SEXP *newname, int pos, SEXP call)
{
    SEXP names;
    int i, indx, nx;

    if (pos < 0 && length(s) > 1) {
	ECALL(call, _("attempt to select more than one element"));
    }
    if (pos < 0 && length(s) < 1) {
	ECALL(call, _("attempt to select less than one element"));
    }

    if(pos < 0) pos = 0;

    indx = -1;
    *newname = R_NilValue;
    switch(TYPEOF(s)) {
    case LGLSXP:
    case INTSXP:
	indx = integerOneIndex(INTEGER(s)[pos], len, call);
	break;
    case REALSXP:
	indx = integerOneIndex(CXXRCONSTRUCT(int, REAL(s)[pos]), len, call);
	break;
    case STRSXP:
	nx = length(x);
	names = getAttrib(x, R_NamesSymbol);
	if (names != R_NilValue) {
	    /* Try for exact match */
	    for (i = 0; i < nx; i++) {
		const char *tmp = translateChar(STRING_ELT(names, i));
		if (!tmp[0]) continue;
		if (streql(tmp, translateChar(STRING_ELT(s, pos)))) {
		    indx = i;
		    break;
		}
	    }
	    /* Try for partial match */
	    if (partial && indx < 0) {
		len = strlen(translateChar(STRING_ELT(s, pos)));
		for(i = 0; i < nx; i++) {
		    const char *tmp = translateChar(STRING_ELT(names, i));
		    if (!tmp[0]) continue;
		    if(!strncmp(tmp, translateChar(STRING_ELT(s, pos)), len)) {
			if(indx == -1 )
			    indx = i;
			else
			    indx = -2;
		    }
		}
	    }
	}
	if (indx == -1)
	    indx = nx;
	*newname = STRING_ELT(s, pos);
	break;
    case SYMSXP:
	nx = length(x);
	names = getAttrib(x, R_NamesSymbol);
	if (names != R_NilValue) {
	    for (i = 0; i < nx; i++)
		if (streql(translateChar(STRING_ELT(names, i)),
			   translateChar(PRINTNAME(s)))) {
		    indx = i;
		    break;
		}
	}
	if (indx == -1)
	    indx = nx;
	*newname = STRING_ELT(s, pos);
	break;
    default:
	if (call == R_NilValue)
	    error(_("invalid subscript type '%s'"), type2char(TYPEOF(s)));
	else
	    errorcall(call, _("invalid subscript type '%s'"),
		      type2char(TYPEOF(s)));
    }
    return indx;
}

// Note by arr after analysing code: if the 'pos' argument is set to
// -1, this signifies that this is not 'recursive' indexing, i.e. the index
// is a vector of length 1. A non-negative value signifies that this
// is recursive indexing, and gives the position (counting from 0)
// within the index vector which should be used for this call.

int attribute_hidden
get1index(SEXP s, SEXP names, int len, int pok, int pos, SEXP call)
{
    /* Get a single index for the [[ operator.
       Check that only one index is being selected.
       pok : is "partial ok" ?
       if pok is -1, warn if partial matching occurs
    */
    bool warn_pok = false;
    if (pok == -1) {
	pok = 1;
	warn_pok = true;
    }

    if (pos < 0 && length(s) != 1) {
	if (length(s) > 1) {
	    ECALL(call, _("attempt to select more than one element"));
	} else {
	    ECALL(call, _("attempt to select less than one element"));
	}
    } else
	if (pos >= length(s)) {
	    ECALL(call, _("internal error in use of recursive indexing"));
	}
    if (pos < 0)
	pos = 0;
    int indx = -1;
    switch (TYPEOF(s)) {
    case LGLSXP:
    case INTSXP:
	{
	    int i = INTEGER(s)[pos];
	    if (i != NA_INTEGER)
		indx = integerOneIndex(i, len, call);
	}
	break;
    case REALSXP:
	{
	    double dblind = REAL(s)[pos];
	    if (!ISNAN(dblind))
		indx = integerOneIndex(int(dblind), len, call);
	}
	break;
    case STRSXP:
	{
	    /* NA matches nothing */
	    if (STRING_ELT(s, pos) == NA_STRING)
		break;
	    /* "" matches nothing: see names.Rd */
	    if (!CHAR(STRING_ELT(s, pos))[0])
		break;

	    /* Try for exact match */
	    const char* ss = translateChar(STRING_ELT(s, pos));
	    for (int i = 0; i < length(names); i++)
		if (STRING_ELT(names, i) != NA_STRING) {
		    if (streql(translateChar(STRING_ELT(names, i)), ss)) {
			indx = i;
			break;
		    }
		}
	    /* Try for partial match */
	    if (pok && indx < 0) {
		len = strlen(ss);
		for(int i = 0; i < length(names); i++) {
		    if (STRING_ELT(names, i) != NA_STRING) {
			const char* cur_name = translateChar(STRING_ELT(names, i));
			if (!strncmp(cur_name, ss, len)) {
			    if (indx == -1) {/* first one */
				indx = i;
				if (warn_pok) {
				    if (call == R_NilValue)
					warning(_("partial match of '%s' to '%s'"),
						ss, cur_name);
				    else
					warningcall(call,
						    _("partial match of '%s' to '%s'"),
						    ss, cur_name);
				}
			    }
			    else {
				indx = -2;/* more than one partial match */
				if (warn_pok) /* already given context */
				    warningcall(R_NilValue,
						_("further partial match of '%s' to '%s'"),
						ss, cur_name);
				break;
			    }
			}
		    }
		}
	    }
	}
	break;
    case SYMSXP:
	for (int i = 0; i < length(names); i++)
	    if (STRING_ELT(names, i) != NA_STRING &&
		streql(translateChar(STRING_ELT(names, i)),
		       CHAR(PRINTNAME(s)))) {
		indx = i;
		break;
	    }
    default:
	if (call == R_NilValue)
	    error(_("invalid subscript type '%s'"), type2char(TYPEOF(s)));
	else
	    errorcall(call, _("invalid subscript type '%s'"),
		      type2char(TYPEOF(s)));
    }
    return indx;
}

SEXP attribute_hidden
vectorIndex(SEXP x, SEXP thesub, int start, int stop, int pok, SEXP call) 
{
    int i, offset;

    for(i = start; i < stop; i++) {
	if(!isVectorList(x) && !isPairList(x)) {
	    if (i)
		errorcall(call, _("recursive indexing failed at level %d\n"), i+1);
	    else
		errorcall(call, _("attempt to select more than one element"));
	}
	offset = get1index(thesub, getAttrib(x, R_NamesSymbol),
		           length(x), pok, i, call);
	if(offset < 0 || offset >= length(x))
	    errorcall(call, _("no such index at level %d\n"), i+1);
	if(isPairList(x)) {
	    x = CAR(nthcdr(x, offset));
	} else {
	    x = VECTOR_ELT(x, offset);
    	}
    }
    return x;
}

/* Special Matrix Subscripting: Handles the case x[i] where */
/* x is an n-way array and i is a matrix with n columns. */
/* This code returns a vector containing the integer subscripts */
/* to be extracted when x is regarded as unravelled. */
/* Negative indices are not allowed. */
/* A zero anywhere in a row will cause a zero in the same */
/* position in the result. */

SEXP attribute_hidden mat2indsub(SEXP dims, SEXP s, SEXP call)
{
    int tdim, j, i, k, nrs = nrows(s);
    SEXP rvec;

    if (ncols(s) != LENGTH(dims)) {
	ECALL(call, _("incorrect number of columns in matrix subscript"));
    }

    PROTECT(rvec = allocVector(INTSXP, nrs));
    s = coerceVector(s, INTSXP);
    setIVector(INTEGER(rvec), nrs, 0);

    for (i = 0; i < nrs; i++) {
	tdim = 1;
	/* compute 0-based subscripts for a row (0 in the input gets -1
	   in the output here) */
	for (j = 0; j < LENGTH(dims); j++) {
	    k = INTEGER(s)[i + j * nrs];
	    if(k == NA_INTEGER) {
		INTEGER(rvec)[i] = NA_INTEGER;
		break;
	    }
	    if(k < 0) {
		ECALL(call, _("negative values are not allowed in a matrix subscript"));
	    }
	    if(k == 0) {
		INTEGER(rvec)[i] = -1;
		break;
	    }
	    if (k > INTEGER(dims)[j]) {
		ECALL(call, _("subscript out of bounds"));
	    }
	    INTEGER(rvec)[i] += (k - 1) * tdim;
	    tdim *= INTEGER(dims)[j];
	}
	/* transform to 1 based subscripting (0 in the input gets 0
	   in the output here) */
	if(INTEGER(rvec)[i] != NA_INTEGER)
	    INTEGER(rvec)[i]++;
    }
    UNPROTECT(1);
    return (rvec);
}

/*
Special Matrix Subscripting: For the case x[i] where x is an n-way
array and i is a character matrix with n columns, this code converts i
to an integer matrix by matching against the dimnames of x. NA values
in any row of i propagate to the result.  Unmatched entries result in
a subscript out of bounds error.  */

SEXP attribute_hidden strmat2intmat(SEXP s, SEXP dnamelist, SEXP call)
{
    /* XXX: assumes all args are protected */
    int nr = nrows(s), i, j, v, idx;
    SEXP dnames, snames, si, sicol, s_elt;
    PROTECT(snames = allocVector(STRSXP, nr));
    PROTECT(si = allocVector(INTSXP, length(s)));
    dimgets(si, getAttrib(s, R_DimSymbol));
    for (i = 0; i < length(dnamelist); i++) {
        dnames = VECTOR_ELT(dnamelist, i);
        for (j = 0; j < nr; j++) {
            SET_STRING_ELT(snames, j, STRING_ELT(s, j + (i * nr)));
        }
        PROTECT(sicol = match(dnames, snames, 0));
        for (j = 0; j < nr; j++) {
            v = INTEGER(sicol)[j];
            idx = j + (i * nr);
            s_elt = STRING_ELT(s, idx);
            if (s_elt == NA_STRING) v = NA_INTEGER;
            if (!CHAR(s_elt)[0]) v = 0; /* disallow "" match */
            if (v == 0) errorcall(call, _("subscript out of bounds"));
            INTEGER(si)[idx] = v;
        }
        UNPROTECT(1);
    }
    UNPROTECT(2);
    return si;
}

static SEXP nullSubscript(int n)
{
    SEXP indx = allocVector(INTSXP, n);
    for (int i = 0; i < n; i++)
	INTEGER(indx)[i] = i + 1;
    return indx;
}

static SEXP logicalSubscript(SEXP s, int ns, int nx, int *stretch, SEXP call)
{
    bool canstretch = (*stretch != 0);
    *stretch = 0;
    pair<const IntVector*, size_t> pr
	= Subscripting::canonicalize(SEXP_downcast<LogicalVector*>(s), nx);
    if (int(pr.second) > nx) {
	if (!canstretch) {
	    ECALL(call, _("subscript out of bounds"));
	} else *stretch = pr.second;
    }
    return const_cast<IntVector*>(pr.first);
}

static SEXP integerSubscript(SEXP s, int ns, int nx, int *stretch, SEXP call)
{
    bool canstretch = (*stretch != 0);
    *stretch = 0;
    pair<const IntVector*, size_t> pr
	= Subscripting::canonicalize(SEXP_downcast<IntVector*>(s), nx);
    if (int(pr.second) > nx) {
	if (!canstretch) {
	    ECALL(call, _("subscript out of bounds"));
	} else *stretch = pr.second;
    }
    return const_cast<IntVector*>(pr.first);
}

typedef SEXP (*StringEltGetter)(SEXP x, int i);

/* This uses a couple of horrible hacks in conjunction with
 * VectorAssign (in subassign.c).  If subscripting is used for
 * assignment, it is possible to extend a vector by supplying new
 * names, and we want to give the extended vector those names, so they
 * are returned as the use.names attribute. Also, unset elements of the vector
 * of new names (places where a match was found) are indicated by
 * setting the element of the newnames vector to NULL.
*/

static SEXP
stringSubscript(SEXP sarg, int ns, int nx, SEXP namesarg,
		StringEltGetter strg, int *stretch, SEXP call)
{
    bool canstretch = (*stretch != 0);
    *stretch = 0;
    pair<const IntVector*, size_t> pr
	= Subscripting::canonicalize(SEXP_downcast<StringVector*>(sarg), nx,
				     SEXP_downcast<StringVector*>(namesarg));
    if (int(pr.second) > nx) {
	if (!canstretch) {
	    ECALL(call, _("subscript out of bounds"));
	} else *stretch = pr.second;
    }
    return const_cast<IntVector*>(pr.first);
}

/* Array Subscripts.
    dim is the dimension (0 to k-1)
    s is the subscript list,
    dims is the dimensions of x
    dng is a function (usually getAttrib) that obtains the dimnames
    x is the array to be subscripted.
*/

typedef SEXP AttrGetter(SEXP x, SEXP data);

static SEXP
int_arraySubscript(int dim, SEXP s, SEXP dims, AttrGetter dng,
		   StringEltGetter strg, SEXP x)
{
    int stretch = 0;
    int ns = length(s);
    int nd = INTEGER(dims)[dim];

    switch (TYPEOF(s)) {
    case NILSXP:
	return allocVector(INTSXP, 0);
    case LGLSXP:
	return logicalSubscript(s, ns, nd, &stretch, 0);
    case INTSXP:
	return integerSubscript(s, ns, nd, &stretch, 0);
    case REALSXP:
	{
	    GCStackRoot<> tmp(coerceVector(s, INTSXP));
	    tmp = integerSubscript(tmp, ns, nd, &stretch, 0);
	    return tmp;
	}
    case STRSXP:
	{
	    SEXP dnames = dng(x, R_DimNamesSymbol);
	    if (dnames == R_NilValue) {
		ECALL(0, _("no 'dimnames' attribute for array"));
	    }
	    dnames = VECTOR_ELT(dnames, dim);
	    return stringSubscript(s, ns, nd, dnames, strg, &stretch, 0);
	}
    case SYMSXP:
	if (s == R_MissingArg)
	    return nullSubscript(nd);
    default:
	    error(_("invalid subscript type '%s'"), type2char(TYPEOF(s)));
    }
    return R_NilValue;
}

/* This is used by packages arules and cba. Seems dangerous as the
   typedef is not exported */
SEXP
arraySubscript(int dim, SEXP s, SEXP dims, AttrGetter dng,
	       StringEltGetter strg, SEXP x)
{
    return int_arraySubscript(dim, s, dims, dng, strg, x);
}

/* Subscript creation.  The first thing we do is check to see */
/* if there are any user supplied NULL's, these result in */
/* returning a vector of length 0. */
/* if stretch is zero on entry then the vector x cannot be
   "stretched",
   otherwise, stretch returns the new required length for x
*/

SEXP attribute_hidden makeSubscript(SEXP x, SEXP s, int *stretch, SEXP call)
{
    SEXP ans = R_NilValue;
    if (isVector(x) || isList(x) || isLanguage(x)) {
	int nx = length(x);

	ans = vectorSubscript(nx, s, stretch, getAttrib, (STRING_ELT),
			      x, call);
    }
    else {
	ECALL(call, _("subscripting on non-vector"));
    }
    return ans;

}

/* nx is the length of the object being subscripted,
   s is the R subscript value,
   dng gets a given attrib for x, which is the object we are
   subsetting,
*/

static SEXP
int_vectorSubscript(int nx, SEXP sarg, int *stretch, AttrGetter dng,
		    StringEltGetter strg, SEXP x, SEXP call)
{
    SEXP ans = R_NilValue;

    int ns = length(sarg);
    /* special case for simple indices -- does not duplicate */
    if (ns == 1 && TYPEOF(sarg) == INTSXP && ATTRIB(sarg) == R_NilValue) {
	int i = INTEGER(sarg)[0];
	if (0 < i && i <= nx) {
	    *stretch = 0;
	    return sarg;
	}
    }
    GCStackRoot<> s(duplicate(sarg));
    if (s) {
	s->clearAttributes();
    }
    switch (TYPEOF(s)) {
    case NILSXP:
	*stretch = 0;
	ans = allocVector(INTSXP, 0);
	break;
    case LGLSXP:
	/* *stretch = 0; */
	ans = logicalSubscript(s, ns, nx, stretch, call);
	break;
    case INTSXP:
	ans = integerSubscript(s, ns, nx, stretch, call);
	break;
    case REALSXP:
	{
	    GCStackRoot<> tmp(coerceVector(s, INTSXP));
	    ans = integerSubscript(tmp, ns, nx, stretch, call);
	}
	break;
    case STRSXP:
	{
	    // If x is a pairlist, names will be created on the fly,
	    // and so needs protecting:
	    GCStackRoot<> names(dng(x, R_NamesSymbol));
	    /* *stretch = 0; */
	    ans = stringSubscript(s, ns, nx, names, strg, stretch, call);
	}
	break;
    case SYMSXP:
	*stretch = 0;
	if (s == R_MissingArg) {
	    ans = nullSubscript(nx);
	    break;
	}
    default:
	if (call == R_NilValue)
	    error(_("invalid subscript type '%s'"), type2char(TYPEOF(s)));
	else
	    errorcall(call, _("invalid subscript type '%s'"),
		      type2char(TYPEOF(s)));
    }
    return ans;
}


SEXP attribute_hidden
vectorSubscript(int nx, SEXP s, int *stretch, AttrGetter dng,
		StringEltGetter strg, SEXP x, SEXP call)
{
    return int_vectorSubscript(nx, s, stretch, dng, strg, x, call);
}
