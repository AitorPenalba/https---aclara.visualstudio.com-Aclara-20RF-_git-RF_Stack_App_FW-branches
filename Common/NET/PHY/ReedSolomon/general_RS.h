#define _CRT_SECURE_NO_WARNINGS

/* Use appropriate statement to enable-printing OR disable-printing to console */
//#define PRINTF printf
//#define PRINTF //

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define ENCODE
#define DECODE
#define NO_PRINT

/* nn_short = Length of shortened-code in symbols.
kk_short = #info_symbols of shortened-code.
max_no_eras = max # of erasures that demod will declare (input to decoder).
*/
int DecoderType = 1; //0 Errors-only, 1 Errors+Erasures decoder.
int no_cws = 1000;
long seed = 7365;
double prob_symb_err = 0.06;

#if 1
  #define mm  8            // RS code over GF(2^mm) - change to suit
  #define n   256     		// n = size of the field
  #define nn  255          // nn = 2^mm -1   length of codeword
  #define tt  8				// number of symbol-errors that can be corrected
  #define kk  239          // kk = nn -2*tt. Degree of g(x) = 2*tt

	int nn_short = 149;
	int kk_short = 133;
	int max_no_eras = 10;

#else
	#define mm  6           // RS code over GF(2^mm) - change to suit
	#define n   64   			// n = size of the field
	#define nn  63				// nn = 2^mm -1   length of codeword
	#define tt  4				// number of symbol-errors that can be corrected
	#define kk  55				// kk = nn -2*tt. Degree of g(x) = 2*tt

	int nn_short = 63;
	int kk_short = 55;
	int max_no_eras = 2;
#endif

/**** Primitive polynomials for different GFs ****/
   int pp[mm + 1] = { 1, 0, 1, 1, 1, 0, 0, 0, 1 }; // 1 + x^2 + x^3 + x^4 + x^8 for GF(2^8)
// int pp[mm +1] = {1, 0, 0, 1, 0, 0, 0, 1}; // 1 + x^3 + x^7 for GF(2^7)
// int pp[mm +1] = {1, 0, 0, 0, 0, 1, 1}; // 1 + x^5 + x^6 for GF1(2^6)
// int pp[mm +1] = {1, 1, 0, 0, 0, 0, 1}; // 1 + x + x^6 for GF2(2^6)
//	int pp[mm +1] = {1, 1, 0, 0, 1}; // 1 + x + x^4 for GF(2^4)
// int pp[mm +1] = {1, 1, 0, 1}; // 1 + x + x^3 for GF(2^3)

/*	generator polynomial gg,
	polynomial representation (alpha_to),
	log_base_primitive representation (index_of)
	of GF elements.
*/
int alpha_to[nn + 1], index_of[nn + 1], gg[nn - kk + 1];

/*	b0	specified first root of generator poly
	g(x) has roots @**b0, @**(b0+1), ... ,@^(b0+2*tt-1)
*/
int b0 = 1;

/*	data[]	= the info vector,
	bb[]	= the parity vector,
	recd[]	= the noise corrupted received vector
*/
int recd[nn], data[kk], bb[nn - kk], errloc[nn];

/*	eras_pos[] is the index of erased locations
	no_eras is actual # of erasures declared.
	max_no_eras = max # of erasures to declare.
*/
int eras_pos[2 * tt];
int no_eras;
