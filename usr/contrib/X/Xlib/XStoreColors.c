#include <X/mit-copyright.h>

/* $Header: XStoreColors.c,v 10.4 86/02/01 15:40:12 tony Rel $ */
/* Copyright    Massachusetts Institute of Technology    1985	*/

#include "XlibInternal.h"
XStoreColors (ncolors, defs)
    	int ncolors;
	register Color *defs;
{
	register Display *dpy;
	register XReq *req;
	GetReq (X_StoreColors, 0);
	req->param.s[0] = ncolors;

	/* Color structure in library uses int for pixel,
	   but protocol's ColorDef wants a short;
	   thus, data must be copied */
        {
	int nbytes = ncolors*sizeof(ColorDef);
	register int i;
	register ColorDef *proto_defs = (ColorDef *) malloc (nbytes);
	for (i=0;i<ncolors;i++) {
	    proto_defs[i].pixel = defs[i].pixel;
	    proto_defs[i].red = defs[i].red;
	    proto_defs[i].green = defs[i].green;
	    proto_defs[i].blue = defs[i].blue;
	    }
	Data (dpy, proto_defs, nbytes);
	free (proto_defs);
	}
}
