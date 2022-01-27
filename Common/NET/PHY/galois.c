/*
* Galois Math Support for Reed-Solomon Encoder and Decoder
* Copyright (c) 2005-2008 Aclara RF Systems, Inc.  All Rights Reserverd
* 7/15/2005 Glenn A. Emelko
*/

#include <stdio.h>
#include <stdlib.h>
#include "rsfec.h"

unsigned char GFMat[NPOW];				// Galois Field a^0 thru a^NPOW-1
unsigned char GFLog[NPOW];				// Inverse Field GFLog(0..NPOW-1)

// Return (n Modulus NPOW-1) for GF Math
unsigned char GFMod(unsigned char n)
{
    while(n>NPOW-1)
    {
        n -= NPOW-1;			// Perform MOD(NPOW-1) by repeated subtraction
    }
    return n;
}


// Return (p1*p2) for polynomials in GF
unsigned char GFMult(unsigned char p1, unsigned char p2)
{
    if (p1==0 || p2==0) return(0);
    return(GFMat[GFMod(GFLog[p1]+GFLog[p2])]); // Multiply by adding log's
}

// Return 1/p1 for polynomial in GF
unsigned char GFRec(unsigned char p1)
{
    return (GFMat[(NPOW-1)-GFLog[p1]]);		// Reciprocal by subtracting log from 0 =(NPOW-1)
}

// Initialize the Galois Field and its Logarithm
void PrepGFMatLog()
{
    unsigned int i;

    GFMat[0]=1;					// Identity
    GFMat[NPOW-1]=1;			// ... because GFMod(NPOW-1)=GFMod(0)

    GFLog[0]=0;					// Undefined... GFLog(0)=0?
    GFLog[1]=0;					// GFLog(1)=0

    for(i=1;i<NPOW-1;i++)
    {
        GFMat[i]=(unsigned char)(GFMat[i-1]<<1); // Multiply by a
        if(GFMat[i] & NPOW)				      // if we exceed NBITS...
        {
            GFMat[i]=GFMat[i] ^ GFPoly;   // use GFPoly identity
        }
        GFLog[GFMat[i]]=(unsigned char)i; // Store the GFLog
    }
}

// Prepare the Reed-Solomon Generator Polynomial
/*
void PrepRSPoly()
{
	int i,j;

	RSPoly[0]=GFMat[1];			// Start with x+a^1
	RSPoly[1]=1;
	for(i=2;i<FECLEN+1;i++)
	{
		RSPoly[i]=1;			// Coeff in x^N is 1
		for(j=i-1;j>0;j--)		// for all sub-terms (except last)
		{
			// Multiply by x+a^i
			RSPoly[j] = RSPoly[j-1] ^ GFMult(RSPoly[j],GFMat[i]);
		}
		RSPoly[0] = GFMult(RSPoly[0],GFMat[i]); // handle the lsw
	}
}
*/
