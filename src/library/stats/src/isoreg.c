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

/* --- Isotonic regression ---
 * code simplified from VR_mds_fn() which is part of MASS.c,
 * Copyright (C) 1995  Brian Ripley
 * ---
 *  R : A Computer Language for Statistical Data Analysis
 *  Copyright (C) 2003	The R Foundation
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
 */
#include <R.h>
#include "modreg.h"

SEXP R_isoreg(SEXP y)
{
    int n = LENGTH(y), i, ip, known, n_ip;
    double tmp, slope;
    SEXP yc, yf, iKnots, ans;
    const char *anms[] = {"y", "yc", "yf", "iKnots", ""};

    /* unneeded: y = coerceVector(y, REALSXP); */

    PROTECT(ans = mkNamed(VECSXP, anms));

    SET_VECTOR_ELT(ans, 0, y);
    SET_VECTOR_ELT(ans, 1, yc = allocVector(REALSXP, n+1));
    SET_VECTOR_ELT(ans, 2, yf = allocVector(REALSXP, n));
    SET_VECTOR_ELT(ans, 3, iKnots= allocVector(INTSXP, n));

    /* yc := cumsum(0,y) */
    REAL(yc)[0] = 0.;
    tmp = 0.;
    for (i = 0; i < n; i++) {
	tmp += REAL(y)[i];
	REAL(yc)[i + 1] = tmp;
    }
    known = 0; ip = 0, n_ip = 0;
    if (n > 0) do {
	slope = R_PosInf;/*1e+200*/
	for (i = known + 1; i <= n; i++) {
	    tmp = (REAL(yc)[i] - REAL(yc)[known]) / (i - known);
	    if (tmp < slope) {
		slope = tmp;
		ip = i;
	    }
	}/* tmp := max{i= kn+1,.., n} slope(p[kn] -> p[i])  and
	  *  ip = argmax{...}... */
	INTEGER(iKnots)[n_ip++] = ip;
	for (i = known; i < ip; i++)
	    REAL(yf)[i] = (REAL(yc)[ip] - REAL(yc)[known]) / (ip - known);
    } while ((known = ip) < n);

    SETLENGTH(iKnots, n_ip);
    UNPROTECT(1);
    return(ans);
}
