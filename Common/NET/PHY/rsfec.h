/*
 * Header file for Reed-Solomon Encoder and Decoder
 * Copyright (c) 2005-2008 Aclara RF Systems, Inc.  All Rights Reserved.
 * 7/15/2005 Glenn A. Emelko
 */

#include "project.h"

#define NBITS  6u			// Symbol size (if under 8 use only lsb's)
#define NPOW   64u		    // Set to 2^NBITS
#define FECLEN 4u			// 4 symbols of 6 bits each (24 bits FEC)

#define STAR_CRC_LENGTH		2u
#define STAR_FEC_LENGTH		3u

//#define MAXLEN 63                     // Number of symbols in max msg, 6 bits each

//extern int MSGBuf[MSGLEN];	// Buffer for message
//extern int SYNBuf[FECLEN];	// Buffer for Syndrome field

// Function prototypes

// Encoder
void EncodeSymbol (unsigned char sym, unsigned char * rs);
void RSEncode (unsigned char * msg, unsigned char len);
void CheckFECBounds(void);

// Decoder
int RSDecode (unsigned char * data, unsigned char len);	// Decode Syndrome, return TRUE/FALSE

void Berlekamp_Massey(void);
void Find_Roots(void);

/* galois arithmetic functions */
void PrepGFMatLog(void);		        // Initialize GFMat and GFLog
void PrepRSPoly(void);				// Initialize Reed-Solomon Generator Polynomial
unsigned char GFMod(unsigned char n);		// Compute (n mod NPOW)
unsigned char GFRec(unsigned char p1);		// Compute (1/p1) in GF(2^N)
unsigned char GFMult(unsigned char p1, unsigned char p2); // Compute (p1*p2) in GF(2^N)

/* Find and fix errors in a coded message */
bool RSRepair (unsigned char * RSBuf, unsigned char len);

/* galois arithmetic tables */
extern unsigned char GFMat[];			// Powers of a
extern unsigned char GFLog[];			// GFLog(x)=a^x
extern unsigned char RSPoly[];			// Reed Solomon Polynomial

#define GFPoly 67				// Using x^6+x+1 (from table below)

// The following table needs checked (07/15/2005 Glenn Emelko)
//
// Good generator polynomials for GF(2) thru GF(8) are
//
//
// Field Extension: Poly		Sum: Expansion
//
// GF(2): x+1                    3: 2+1
//
// GF(4): x^2+x+1                7: 4+2+1
//
// GF(8): x^3+x+1                11: 8+2+1
//		  x^3+x^2+1              13: 8+4+1
//
// GF(16): x^4+x+1               19: 16+2+1
//         x^4+x^3+1             25: 16+8+1
//         x^4+x^3+x^2+x+1       31: 16+8+4+2+1
//
// GF(32): x^5+x^2+1             37: 32+4+1
//         x^5+x^3+1             41: 32+8+1
//         x^5+x^3+x^2+x+1       47: 32+8+4+2+1
//         x^5+x^4+x^2+x+1       55: 32+16+4+2+1
//         x^5+x^4+x^3+x+1       59: 32+16+8+2+1
//         x^5+x^4+x^3+x^2+1     61: 32+16+8+4+1
//
// GF(64): x^6+x+1               67: 64+2+1
//         x^6+x^3+1             73: 64+8+1
//         x^6+x^4+x^2+x+1       87: 64+16+4+2+1
//         x^6+x^4+x^3+x+1       91: 64+16+8+2+1
//         x^6+x^5+1             97: 64+32+1
//         x^6+x^5+x^2+x+1       103: 64+32+4+2+1
//         x^6+x^5+x^3+x^2+1     109: 64+32+8+4+1
//         x^6+x^5+x^4+x+1       115: 64+32+16+2+1
//         x^6+x^5+x^4+x^2+1     117: 64+32+16+4+1
//
// Subset of GF(128) based on 3 taps (note 3 is min!)
// GF(128): x^7+x+1              131: 128+2+1
//          x^7+x^3+1            137: 128+8+1
//          x^7+x^4+1            145: 128+16+1
//          x^7+x^6+1            193: 128+64+1
//
// Subset of GF(256) based on 5 taps (note 5 is min!)
// GF(256): x^8+x^4+x^3+x+1      283: 256+16+8+2+1
//          x^8+x^4+x^3+x^2+1    285: 256+16+8+4+1
//          x^8+x^5+x^3+x+1      299: 256+32+8+2+1
//          x^8+x^5+x^3+x^2+1    301: 256+32+8+4+1
//          x^8+x^5+x^4+x^3+1    313: 256+32+16+8+1
//          x^8+x^6+x^3+x^2+1    333: 256+64+8+4+1
//          x^8+x^6+x^5+x+1      355: 256+64+32+2+1
//          x^8+x^6+x^5+x^2+1    357: 256+64+32+4+1
//          x^8+x^6+x^5+x^3+1    361: 256+64+32+8+1
//          x^8+x^6+x^5+x^4+1    369: 256+64+32+16+1
//          x^8+x^7+x^2+x+1      391: 256+128+4+2+1
//          x^8+x^7+x^3+x+1      395: 256+128+8+2+1
//          x^8+x^7+x^3+x^2+1    397: 256+128+8+4+1
//          x^8+x^7+x^5+x+1      419: 256+128+32+2+1
//          x^8+x^7+x^5+x^3+1    425: 256+128+32+8+1
//          x^8+x^7+x^5+x^4+1    433: 256+128+32+16+1
//          x^8+x^7+x^6+x+1      451: 256+128+64+2+1

