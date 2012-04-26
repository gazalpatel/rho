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
 *  Copyright (C) 2004   The R Development Core Team.
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
#include "modreg.h" /* for declarations for registration */

void kmeans_Lloyd(double *x, int *pn, int *pp, double *cen, int *pk, int *cl,
		  int *pmaxiter, int *nc, double *wss)
{
    int n = *pn, k = *pk, p = *pp, maxiter = *pmaxiter;
    int iter, i, j, c, it, inew = 0;
    double best, dd, tmp;
    Rboolean updated;

    for(i = 0; i < n; i++) cl[i] = -1;
    for(iter = 0; iter < maxiter; iter++) {
	updated = FALSE;
	for(i = 0; i < n; i++) {
	    /* find nearest centre for each point */
	    best = R_PosInf;
	    for(j = 0; j < k; j++) {
		dd = 0.0;
		for(c = 0; c < p; c++) {
		    tmp = x[i+n*c] - cen[j+k*c];
		    dd += tmp * tmp;
		}
		if(dd < best) {
		    best = dd;
		    inew = j+1;
		}
	    }
	    if(cl[i] != inew) {
		updated = TRUE;
		cl[i] = inew;
	    }
	}
	if(!updated) break;
	/* update each centre */
	for(j = 0; j < k*p; j++) cen[j] = 0.0;
	for(j = 0; j < k; j++) nc[j] = 0;
	for(i = 0; i < n; i++) {
	    it = cl[i] - 1; nc[it]++;
	    for(c = 0; c < p; c++) cen[it+c*k] += x[i+c*n];
	}
	for(j = 0; j < k*p; j++) cen[j] /= nc[j % k];
    }

    *pmaxiter = iter + 1;
    for(j = 0; j < k; j++) wss[j] = 0.0;
    for(i = 0; i < n; i++) {
	it = cl[i] - 1;
	for(c = 0; c < p; c++) {
	    tmp = x[i+n*c] - cen[it+k*c];
	    wss[it] += tmp * tmp;
	}
    }
}

void kmeans_MacQueen(double *x, int *pn, int *pp, double *cen, int *pk,
		     int *cl, int *pmaxiter, int *nc, double *wss)
{
    int n = *pn, k = *pk, p = *pp, maxiter = *pmaxiter;
    int iter, i, j, c, it, inew = 0, iold;
    double best, dd, tmp;
    Rboolean updated;

    /* first assign each point to the nearest cluster centre */
    for(i = 0; i < n; i++) {
	best = R_PosInf;
	for(j = 0; j < k; j++) {
	    dd = 0.0;
	    for(c = 0; c < p; c++) {
		tmp = x[i+n*c] - cen[j+k*c];
		dd += tmp * tmp;
	    }
	    if(dd < best) {
		best = dd;
		inew = j+1;
	    }
	}
	if(cl[i] != inew) cl[i] = inew;
    }
   /* and recompute centres as centroids */
    for(j = 0; j < k*p; j++) cen[j] = 0.0;
    for(j = 0; j < k; j++) nc[j] = 0;
    for(i = 0; i < n; i++) {
	it = cl[i] - 1; nc[it]++;
	for(c = 0; c < p; c++) cen[it+c*k] += x[i+c*n];
    }
    for(j = 0; j < k*p; j++) cen[j] /= nc[j % k];

    for(iter = 0; iter < maxiter; iter++) {
	updated = FALSE;
	for(i = 0; i < n; i++) {
	    best = R_PosInf;
	    for(j = 0; j < k; j++) {
		dd = 0.0;
		for(c = 0; c < p; c++) {
		    tmp = x[i+n*c] - cen[j+k*c];
		    dd += tmp * tmp;
		}
		if(dd < best) {
		    best = dd;
		    inew = j;
		}
	    }
	    if((iold = cl[i] - 1) != inew) {
		updated = TRUE;
		cl[i] = inew + 1;
		nc[iold]--; nc[inew]++;
		/* update old and new cluster centres */
		for(c = 0; c < p; c++) {
		    cen[iold+k*c] += (cen[iold+k*c] - x[i+n*c])/nc[iold];
		    cen[inew+k*c] += (x[i+n*c] - cen[inew+k*c])/nc[inew];
		}
	    }
	}
	if(!updated) break;
    }

    *pmaxiter = iter + 1;
    for(j = 0; j < k; j++) wss[j] = 0.0;
    for(i = 0; i < n; i++) {
	it = cl[i] - 1;
	for(c = 0; c < p; c++) {
	    tmp = x[i+n*c] - cen[it+k*c];
	    wss[it] += tmp * tmp;
	}
    }
}
