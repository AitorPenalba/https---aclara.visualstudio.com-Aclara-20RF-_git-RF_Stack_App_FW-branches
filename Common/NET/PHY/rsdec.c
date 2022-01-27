/*
* Reed-Solomon Decoder
* Copyright (c) 2005-2008 Aclara RF Systems, Inc.  All Rights Reserved.
* 7/15/2005 Glenn A. Emelko
*
* Adapted for MSP430 using GF(63) and generator x^6+x+1 on 3/24/06 GAE
*/

#include "rsfec.h"

/* Decoder Data */
static unsigned char SYNBuf[FECLEN+1];
static unsigned char Lambda[FECLEN*2+1];		// Error locator polynomial
static unsigned char Omega[FECLEN*2+1];		// Error evaluator polynomial

/* error locations found using Chien's search*/
static unsigned char ErrorLocs[(FECLEN>>1)+2];
static unsigned char NErrors;

/* erasure flags */
//static unsigned char ErasureLocs[FECLEN+1];
//static unsigned char NErasures;

static void RSGamma(unsigned char * gamma);

// Reed Solomon Decoder
//
// return "adjusted length" if the packet is good
// return negative "adjusted length" if there's an error (and set up Syndrome)
//
// Example: RS packet length 14 is tested:
//  if it is good, length will be returned as 11 (last 3 bytes are no longer needed)
//  if it has an error, length is -20 symbols (w/ last 3 bytes moved and 0 inserted)
//

int RSDecode(unsigned char *data, unsigned char len)
{
    unsigned char i, i1, j, flen, sym, syn, result;
    result = 1;				// Init result to TRUE
    // Re-pad the data buffer to the correct length
    if(len<3) return 0;
    flen = len/3;
    flen *= 3;
    if (flen != len)
    {
        flen += 3;                          // feclen is next-upper-multiple of 3
        data[flen-1]=data[len-1];           // move FEC bytes out to end of buffer
        data[flen-2]=data[len-2];
        data[flen-3]=data[len-3];
        data[len-3]=0;                      // pad the first byte skipped with 0
        if (flen != len+1)                  // see if there is a second
        {
            data[len-2]=0;                  //   if so pad it also with 0
        }
    }

    flen = (unsigned char)((flen/3)<<2);    // Entire packet length in symbols (6 bit each)

    for(j=0;j<FECLEN;j++)                   // For each FEC symbol
    {
        syn=0;				// compute the syndrome
        for(i=0;i<flen;i++) 	        // go through the message
        {
            i1 = (unsigned char)(((i & 0xfc)*3)/4);
            switch ((i & 3))
            {
              case 0:
                sym = data[i1];                // Pull from location [A5..0]
                break;
              case 1:
                sym = (unsigned char)(((data[i1+1]<<2) | (data[i1]>>6))); // locations [B5..0]
                break;
              case 2:
                sym = (unsigned char)(((data[i1+2]<<4) | (data[i1+1]>>4))); // locations [C5..0]
                break;
              default:
                sym = data[i1+2]>>2;           // Locations [D5..0]
            }
            syn=(sym & 0x3f) ^ GFMult(GFMat[j+1],syn); // Execute LFSR for the symbol
        }
        SYNBuf[j]=syn;              // assign syndrome and test: result should be 0
        if(SYNBuf[j])
        {
            result=0;				// if not, then SYNBuf has clues to where the error is
        }
    }
    if (result==1)
    {
        return len-3;	                // If no error, then return packet length
    }
    else
    {
        return -flen;                       // If error, return fec-message length (As neg)
    }
}

void Berlekamp_Massey()
{
    unsigned char n, L, L2, i, j, discrep;
    signed char k; // needs to be able to be assigned to -1
    unsigned char psi[FECLEN+1], psi2[FECLEN+1], D[FECLEN+1];
    unsigned char gamma[FECLEN+1];

    /* initialize Gamma, the erasure locator polynomial */
    RSGamma(gamma);

    for(i=0;i<FECLEN;i++)
    {
        D[i+1]=gamma[i];
    }
    D[0]=0;

    for(i=0;i<=FECLEN;i++)
    {
        psi[i]=gamma[i];
    }
    k = -1;
    L = 0;

    for(n=0;n<FECLEN;n++)
    {
        discrep=0;
        for(i=0;i<=L;i++)
        {
            discrep ^= GFMult(psi[i],SYNBuf[n-i]);
        }

        if(discrep!=0)
        {
            /* psi2 = psi - d*D */
            for(i=0;i<=FECLEN;i++)
            {
                psi2[i] = psi[i] ^ GFMult(discrep, D[i]);
            }
            if(L<(n-k))
            {
                L2 = (unsigned char)(n-k);
                k = (signed char)(n-L);

                for(i=0;i<=FECLEN;i++)
                {
                    D[i]=GFMult(psi[i], GFRec(discrep));
                }
                L = L2;
            }

            /* psi = psi2 */
            for(i=0;i<=FECLEN;i++)
            {
                psi[i] = psi2[i];
            }
        }
        for(i=FECLEN;i>0;i--)
        {
            D[i]=D[i-1];
        }
        D[0]=0;
    }

    for(i=0;i<=FECLEN;i++)
    {
        Lambda[i] = psi[i];
    }

    for(i=0;i<=FECLEN;i++)
    {
        Omega[i]=0;
    }

    for(i=0;i<FECLEN;i++)
    {
        for(j=0;j<FECLEN-i;j++)
        {
            Omega[i+j] ^= GFMult(Lambda[i],SYNBuf[j]);
        }
    }
}


/* gamma = product (1-z*a^Ij) for erasure locs Ij */
static void RSGamma (unsigned char * gamma)
{
    unsigned char i;
    //, e, tmp[FECLEN+1];

    for(i=0;i<=FECLEN;i++)
    {
        gamma[i]=0;
    }
    gamma[0] = 1;
#if 0
    for (e = 0; e < NErasures; e++)
    {
        for(i=0;i<FECLEN;i++)
        {
            tmp[i+1]=GFMult(gamma[i],GFMat[ErasureLocs[e]]);
        }
        tmp[0]=0;
        for(i=0;i<=FECLEN;i++)
        {
            gamma[i] ^= tmp[i];
        }
    }
#endif
}

/*
* Finds the roots of an error-locator polynomial
* evaluate Lambda at each increasing value of alpha.
*/
void Find_Roots()
{
    unsigned char sum, r, k;
    NErrors=0;

    for(r=1;r<NPOW;r++)
    {
        sum=0;
        for(k=0;k<=FECLEN;k++)
        {
            sum ^= GFMult(GFMat[GFMod((unsigned char)(k*r))], Lambda[k]);
        }
        if (sum==0)
        {
            ErrorLocs[NErrors]=(NPOW-1)-r;
            NErrors++;
            if (NErrors > FECLEN/2) break;
        }
    }
}


/*
* Given RSBuf and the known erasure locations
*
* Evaluate Omega/Lambda' at each root 1/(alpha^i) for error location i.
*
* Returns TRUE if we can fix it, or FALSE if out-of-bounds
*
*/

bool RSRepair(unsigned char * RSBuf, unsigned char len)
{
    unsigned char r, i, j, err, i1, i2;
    unsigned char num, denom;

    Berlekamp_Massey();
    Find_Roots();

    if((NErrors <= FECLEN/2) && (NErrors>0))
    {

        /* first check for illegal error locs */
        for(r=0;r<NErrors;r++)
        {
            if (ErrorLocs[r] >= len)
            {
                return false; // Out of bounds
            }
        }

        for(r=0;r<NErrors;r++)
        {
            i = ErrorLocs[r];
            /* evaluate Omega at alpha^(-i) */

            num=0;
            for(j=0;j<FECLEN;j++)
            {
                num ^= GFMult(Omega[j], GFMat[GFMod((unsigned char)(((NPOW-1)-i)*j))]);
            }

            denom = 0;
            for(j=1;j<FECLEN;j+=2)
            {
                denom ^= GFMult(Lambda[j], GFMat[GFMod((unsigned char)(((NPOW-1)-i)*(j-1)))]);
            }

            err = GFMult(num, GFRec(denom));
            i1 = (len-1)-i;
            i2 = (unsigned char)(((i1 & 0xfc)*3)/4);
            switch ((i1 & 3))
            {
              case 0:
                RSBuf[i2] ^= err;                // Pull from location [A5..0]
                break;

              case 1:
                RSBuf[i2] ^= (unsigned char)((err<<6));
                RSBuf[i2+1] ^= (err>>2);
                break;

              case 2:
                RSBuf[i2+1] ^= (unsigned char)((err<<4));
                RSBuf[i2+2] ^= (err>>4);
                break;

              default:
                RSBuf[i2+2] ^= (unsigned char)((err<<2));
            }
            //      RSBuf[len-1-i] ^= err;
        }
        return true;
    }
    else
    {
        return false;
    }
}
