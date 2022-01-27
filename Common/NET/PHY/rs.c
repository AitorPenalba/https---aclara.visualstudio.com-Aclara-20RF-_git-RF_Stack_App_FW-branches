/*             rs.c        */
/* This program is an encoder/decoder for Reed-Solomon codes. Encoding is in
   systematic form, decoding via the Berlekamp iterative algorithm.
   In the present form , the constants mm, nn, tt, and kk=nn-2tt must be
   specified  (the double letters are used simply to avoid clashes with
   other n,k,t used in other programs into which this was incorporated!)
   Also, the irreducible polynomial used to generate GF(2**mm) must also be
   entered -- these can be found in Lin and Costello, and also Clark and Cain.

   The representation of the elements of GF(2**m) is either in index form,
   where the number is the power of the primitive element alpha, which is
   convenient for multiplication (add the powers modulo 2**m-1) or in
   polynomial form, where the bits represent the coefficients of the
   polynomial representation of the number, which is the most convenient form
   for addition.  The two forms are swapped between via lookup tables.
   This leads to fairly messy looking expressions, but unfortunately, there
   is no easy alternative when working with Galois arithmetic.

   The code is not written in the most elegant way, but to the best
   of my knowledge, (no absolute guarantees!), it works.
   However, when including it into a simulation program, you may want to do
   some conversion of global variables (used here because I am lazy!) to
   local variables where appropriate, and passing parameters (eg array
   addresses) to the functions  may be a sensible move to reduce the number
   of global variables and thus decrease the chance of a bug being introduced.

   This program does not handle erasures at present, but should not be hard
   to adapt to do this, as it is just an adjustment to the Berlekamp-Massey
   algorithm. It also does not attempt to decode past the BCH bound -- see
   Blahut "Theory and practice of error control codes" for how to do this.

              Simon Rockliff, University of Adelaide   21/9/89

   26/6/91 Slight modifications to remove a compiler dependent bug which hadn't
           previously surfaced. A few extra comments added for clarity.
           Appears to all work fine, ready for posting to net!

                  Notice
                 --------
   This program may be freely modified and/or given to whoever wants it.
   A condition of such distribution is that the author's contribution be
   acknowledged by his name being left in the comments heading the program,
   however no responsibility is accepted for any financial or other loss which
   may result from some unforseen errors or malfunctioning of the program
   during use.
                                 Simon Rockliff, 26th June 1991
*/

/*
 * File:     RS.c
 * Title:    Encoder/decoder for RS codes in C
 * Author:   Hari Moorthy (hmoorthy@aclara.com)
 * Date:     May 2016
 * Copyright: Aclara Inc. All Rights Reserved
 */

/*
    Reed-Solomon Encoder, Decoder over Galois-Field of size 2^m.
    length is "nn",
    dimension is "kk" and
    error correcting capability is "tt".

    The generator polynomial is displayed in two formats.
        coefficients as exponents of the primitive element @ of the field.
        coefficients by their representation in the basis {@^7,...,@,1}.

    These codewords can then be transmitted over a noisy channel and
    decoded using the rsdec program.

   After "nn", "tt" and "kk" are set, the program must be recompiled.

   Almost always when the channel causes more than tt errors, the decoder fails
   to produce a codeword. In such a case the decoder outputs the uncorrected
   information bytes as they were received from the channel.

   The syndrome of the received word is shown. This consists of 2*tt bytes
   {S(1),S(2), ... ,S(2*tt)}. Each S(i) is an element in the field GF(n).
   NOTE: Iff all the S(i)'s are zero, the received word is a codeword in the
   code. In such a case, the channel presumably caused no errors.

  INPUT:
    Change the Galois Field and error correcting capability by
    setting the appropriate global variables below including
    1. primitive polynomial 2. error correcting capability
    3. dimension 4. length.

  FUNCTIONS:
    generate_gf() generates the field.
    gen_poly() generates the generator polynomial.
    encode_rs() encodes in systematic form.
    decode_rs() errors-only decodes a vector assumed to be encoded in systematic form.
    eras_dec_rs() errors+erasures decoding of a vector assumed to be encoded in systematic form.
*/

#include "project.h"
#include <stdbool.h>
#include <string.h> // for memset
#include <stdlib.h>
#include <math.h>

#include "PHY_protocol.h"
#include "rs.h"

// Maximum values for MM, N, NN, TT and KK of any RS code used
#define MM  8         // RS code over GF(2^mm) - change to suit
#define NN  255       // nn = 2^mm -1   length of codeword
#define TT  8         // number of symbol-errors that can be corrected
//#define KK  239        // kk = nn -2*tt. Degree of g(x) = 2*tt

/* nn_short = Length of shortened-code in symbols.
kk_short = #info_symbols of shortened-code.
max_no_eras = max # of erasures that demod will declare (input to decoder).
*/

typedef struct
{
   int16_t mm;       // RS code over GF(2^mm) - change to suit
   int16_t nn;       // nn = 2^mm -1   length of codeword
   int16_t tt;       // number of symbol-errors that can be corrected
   int16_t kk;       // kk = nn -2*tt. Degree of g(x) = 2*tt
   int16_t pp[MM+1]; // Primitive polynomials
   int16_t alpha_to[NN+1]; // Galois field 2^MM from polynomial
   int16_t index_of[NN+1]; // Galois field 2^MM from polynomial
   int16_t gg[(2*TT)+1];   // Generator polynomial
}RSCodeAttr_t;

static const RSCodeAttr_t RSCode[RS_LAST] =
{
   {  // SRFN
      .mm = 6,
      .nn = 63,
      .tt = 2,
      .kk = 59,
      .pp = { 1, 0, 0, 0, 0, 1, 1, 0, 0 }, // 1 + x^5 + x^6 for GF(2^6)
      .alpha_to = { 1,  2,  4,  8, 16, 32, 33, 35, 39, 47, 63, 31, 62, 29, 58, 21,
                   42, 53, 11, 22, 44, 57, 19, 38, 45, 59, 23, 46, 61, 27, 54, 13,
                   26, 52,  9, 18, 36, 41, 51,  7, 14, 28, 56, 17, 34, 37, 43, 55,
                   15, 30, 60, 25, 50,  5, 10, 20, 40, 49,  3,  6, 12, 24, 48,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
      .index_of = {  -1,  0,  1, 58,  2, 53, 59, 39,  3, 34, 54, 18, 60, 31, 40, 48,
                      4, 43, 35, 22, 55, 15, 19, 26, 61, 51, 32, 29, 41, 13, 49, 11,
                      5,  6, 44,  7, 36, 45, 23,  8, 56, 37, 16, 46, 20, 24, 27,  9,
                     62, 57, 52, 38, 33, 17, 30, 47, 42, 21, 14, 25, 50, 28, 12, 10,
                      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
      .gg = { 10, 54, 32, 49, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
   },
   {
      .mm = 8,
      .nn = 255,
      .tt = 8,
      .kk = 239,
      .pp = { 1, 0, 1, 1, 1, 0, 0, 0, 1 }, // 1 + x^2 + x^3 + x^4 + x^8 for GF(2^8)
      .alpha_to = { 1,   2,   4,   8,  16,  32,  64, 128,  29,  58, 116, 232, 205, 135,  19,  38,
                   76, 152,  45,  90, 180, 117, 234, 201, 143,   3,   6,  12,  24,  48,  96, 192,
                  157,  39,  78, 156,  37,  74, 148,  53, 106, 212, 181, 119, 238, 193, 159,  35,
                   70, 140,   5,  10,  20,  40,  80, 160,  93, 186, 105, 210, 185, 111, 222, 161,
                   95, 190,  97, 194, 153,  47,  94, 188, 101, 202, 137,  15,  30,  60, 120, 240,
                  253, 231, 211, 187, 107, 214, 177, 127, 254, 225, 223, 163,  91, 182, 113, 226,
                  217, 175,  67, 134,  17,  34,  68, 136,  13,  26,  52, 104, 208, 189, 103, 206,
                  129,  31,  62, 124, 248, 237, 199, 147,  59, 118, 236, 197, 151,  51, 102, 204,
                  133,  23,  46,  92, 184, 109, 218, 169,  79, 158,  33,  66, 132,  21,  42,  84,
                  168,  77, 154,  41,  82, 164,  85, 170,  73, 146,  57, 114, 228, 213, 183, 115,
                  230, 209, 191,  99, 198, 145,  63, 126, 252, 229, 215, 179, 123, 246, 241, 255,
                  227, 219, 171,  75, 150,  49,  98, 196, 149,  55, 110, 220, 165,  87, 174,  65,
                  130,  25,  50, 100, 200, 141,   7,  14,  28,  56, 112, 224, 221, 167,  83, 166,
                   81, 162,  89, 178, 121, 242, 249, 239, 195, 155,  43,  86, 172,  69, 138,   9,
                   18,  36,  72, 144,  61, 122, 244, 245, 247, 243, 251, 235, 203, 139,  11,  22,
                   44,  88, 176, 125, 250, 233, 207, 131,  27,  54, 108, 216, 173,  71, 142,   0 },
      .index_of = {-1,   0,   1,  25,   2,  50,  26, 198,   3, 223,  51, 238,  27, 104, 199,  75,
                    4, 100, 224,  14,  52, 141, 239, 129,  28, 193, 105, 248, 200,   8,  76, 113,
                    5, 138, 101,  47, 225,  36,  15,  33,  53, 147, 142, 218, 240,  18, 130,  69,
                   29, 181, 194, 125, 106,  39, 249, 185, 201, 154,   9, 120,  77, 228, 114, 166,
                    6, 191, 139,  98, 102, 221,  48, 253, 226, 152,  37, 179,  16, 145,  34, 136,
                   54, 208, 148, 206, 143, 150, 219, 189, 241, 210,  19,  92, 131,  56,  70,  64,
                   30,  66, 182, 163, 195,  72, 126, 110, 107,  58,  40,  84, 250, 133, 186,  61,
                  202,  94, 155, 159,  10,  21, 121,  43,  78, 212, 229, 172, 115, 243, 167,  87,
                    7, 112, 192, 247, 140, 128,  99,  13, 103,  74, 222, 237,  49, 197, 254,  24,
                  227, 165, 153, 119,  38, 184, 180, 124,  17,  68, 146, 217,  35,  32, 137,  46,
                   55,  63, 209,  91, 149, 188, 207, 205, 144, 135, 151, 178, 220, 252, 190,  97,
                  242,  86, 211, 171,  20,  42,  93, 158, 132,  60,  57,  83,  71, 109,  65, 162,
                   31,  45,  67, 216, 183, 123, 164, 118, 196,  23,  73, 236, 127,  12, 111, 246,
                  108, 161,  59,  82,  41, 157,  85, 170, 251,  96, 134, 177, 187, 204,  62,  90,
                  203,  89,  95, 176, 156, 169, 160,  81,  11, 245,  22, 235, 122, 117,  44, 215,
                   79, 174, 213, 233, 230, 231, 173, 232, 116, 214, 244, 234, 168,  80,  88, 175 },
      .gg = { 136, 240, 208, 195, 181, 158, 201, 100, 11, 83, 167, 107, 113, 110, 106, 121, 0 }
   },
   {  // STAR
      .mm = 6,
      .nn = 63,
      .tt = 2,
      .kk = 59,
      .pp = { 1, 1, 0, 0, 0, 0, 1, 0, 0 }, // 1 + x^1 + x^6 for GF(2^6)
      .alpha_to = { 1,  2,  4,  8, 16, 32,  3,  6, 12, 24, 48, 35,  5, 10, 20, 40,
                   19, 38, 15, 30, 60, 59, 53, 41, 17, 34,  7, 14, 28, 56, 51, 37,
                    9, 18, 36, 11, 22, 44, 27, 54, 47, 29, 58, 55, 45, 25, 50, 39,
                   13, 26, 52, 43, 21, 42, 23, 46, 31, 62, 63, 61, 57, 49, 33,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
      .index_of = {-1,  0,  1,  6,  2, 12,  7, 26,  3, 32, 13, 35,  8, 48, 27, 18,
                    4, 24, 33, 16, 14, 52, 36, 54,  9, 45, 49, 38, 28, 41, 19, 56,
                    5, 62, 25, 11, 34, 31, 17, 47, 15, 23, 53, 51, 37, 44, 55, 40,
                   10, 61, 46, 30, 50, 22, 39, 43, 29, 60, 42, 21, 20, 59, 57, 58,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
      .gg = { 10, 24, 41, 19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
   }
};

/*  generator polynomial gg,
    polynomial representation (alpha_to),
    log_base_primitive representation (index_of)
    of GF elements.
*/
//int alpha_to[NN + 1];
//int index_of[NN + 1];
//int gg[(2*TT) + 1];

/*  b0   specified first root of generator poly
    g(x) has roots @**b0, @**(b0+1), ... ,@^(b0+2*tt-1)
*/
static int16_t const b0 = 1;

/*  data[]  = the info vector,
    bb[]    = the parity vector,
    recd[]  = the noise corrupted received vector
*/
static int16_t recd[NN];
static int16_t bb[2*TT];

/*  eras_pos[] is the index of erased locations
    no_eras is actual # of erasures declared.
    max_no_eras = max # of erasures to declare.
*/
//int16_t eras_pos[2 * TT] = {0};

// Return MM
int16_t RS_GetMM(RSCode_e rscode)
{
   return (RSCode[rscode].mm);
}

// Return KK
int16_t RS_GetKK(RSCode_e rscode)
{
   return (RSCode[rscode].kk);
}

// Return number of bytes needed to hold parity
uint32_t RS_GetParityLength(RSCode_e rscode) {
   return (uint32_t)((((RSCode[rscode].nn-RSCode[rscode].kk) * RSCode[rscode].mm) + 7) / 8);
}

// Return RS code based on mode and modeParameters
RSCode_e RS_GetCode(PHY_MODE_e mode, PHY_MODE_PARAMETERS_e modeParameters)
{
   // Check for SRFN
   if (mode == ePHY_MODE_1) {
      switch (modeParameters) {
         case ePHY_MODE_PARAMETERS_0: return (RS_SRFN_63_59);
         case ePHY_MODE_PARAMETERS_1: return (RS_SRFN_255_239);
         case ePHY_MODE_PARAMETERS_LAST:
         default: return (RS_SRFN_63_59);
      }
   } else {
      // Default to STAR
      return (RS_STAR_63_59);
   }
}

#if 0
void generate_gf(RSCodeAttr_t *rscode)
 /* generate GF(2^m) from the irreducible polynomial p(X) in p[0]..p[m]
   lookup tables:  index->polynomial form   alpha_to[] contains j=alpha^i;
                   polynomial form -> index form  index_of[j=alpha^i] = i
   alpha=2 is the primitive element of GF(2^m)
   HARI's COMMENT: (4/13/94) alpha_to[] can be used as follows:
        Let @ represent the primitive element commonly called "alpha" that
   is the root of the primitive polynomial p(x). Then in GF(2^m), for any
   0 <= i <= 2^m-2,
        @^i = a(0) + a(1) @ + a(2) @^2 + ... + a(m-1) @^(m-1)
   where the binary vector (a(0),a(1),a(2),...,a(m-1)) is the representation
   of the integer "alpha_to[i]" with a(0) being the LSB and a(m-1) the MSB. Thus for
   example the polynomial representation of @^5 would be given by the binary
   representation of the integer "alpha_to[5]".
                   Similarily, index_of[] can be used as follows:
        As above, let @ represent the primitive element of GF(2^m) that is
   the root of the primitive polynomial p(x). In order to find the power
   of @ (alpha) that has the polynomial representation
        a(0) + a(1) @ + a(2) @^2 + ... + a(m-1) @^(m-1)
   we consider the integer "i" whose binary representation with a(0) being LSB
   and a(m-1) MSB is (a(0),a(1),...,a(m-1)) and locate the entry
   "index_of[i]". Now, @^index_of[i] is that element whose polynomial
    representation is (a(0),a(1),a(2),...,a(m-1)).
   NOTE:
        The element alpha_to[2^m-1] = 0 always signifying that the
   representation of "@^infinity" = 0 is (0,0,0,...,0).
        Similarily, the element index_of[0] = -1 always signifying
   that the power of alpha which has the polynomial representation
   (0,0,...,0) is "infinity".
*/
{
   int i;
   int16_t mask ;
   int16_t *alpha_to;  // Galois field 2^MM from polynomial
   int16_t *index_of;  // Galois field 2^MM from polynomial

   alpha_to = rscode->alpha_to;
   index_of = rscode->index_of;

   // obtain polynomial representation of @^m = (p(0), p(1),...,p(mm-1))
   // where primitive-poly = p(0) + p(1)x + ... p(mm-1)x^(mm-1) + x^mm
   mask = 1 ;
   alpha_to[rscode->mm] = 0 ;
   for (i=0; i < rscode->mm; i++)
   {
      alpha_to[i] = mask ; //Since @^i has poly-form (0,...0,1,0,...0) for 0 <= i < mm
      index_of[alpha_to[i]] = i ;
      if (rscode->pp[i] != 0) /* If pp[i] == 1 then, term @^i occurs in poly-repr of @^mm */
         alpha_to[rscode->mm] ^= mask ;  /* Bit-wise EXOR operation */
      mask <<= 1 ; /* single left-shift */
   }
   index_of[alpha_to[rscode->mm]] = rscode->mm ;

   /*
     Have obtained poly-repr of @^mm. Poly-repr of @^(i+1) is given by
     poly-repr of @^i shifted left one-bit and accounting for any @^mm
     term that may occur when poly-repr of @^i is shifted.
   */
   mask >>= 1; // mask should equal 2^(mm-1)
   for (i = rscode->mm +1; i < rscode->nn; i++)
   {
      if (alpha_to[i-1] >= mask) //if @^(i-1) has 1 in @^(m-1) position, @^i must account for poly-repr of @^mm.
        alpha_to[i] = alpha_to[rscode->mm] ^ ((alpha_to[i-1]^mask)<<1) ;
      else
         alpha_to[i] = alpha_to[i-1] << 1; // since @^(i-1) has poly-form representation < 2^(mm-1)

      index_of[alpha_to[i]] = i;
   }
   index_of[0] = -1; //Since no power of @ has poly-repr (0,...,0)
}

void gen_poly(RSCodeAttr_t *rscode)
/*
   0) INPUT: b0, nn -kk, alpha_to, index_of.
   1) OUTPUT: generator-poly gg[] in index form.
   2) Obtain the generator polynomial of the tt-error correcting, length = nn =(2^mm -1)
      The generator-poly has form 1 + gg(1)*x + ... gg(nn-kk-1)*x^(nn-kk-1) + x^(nn-kk)
      generator-poly = product of (X + @^(b0 + i)), i = 0, ... ,(2*tt-1). 2*tt = nn -kk.
   3) Examples:
      If b0 = 1, tt = 1. deg(g(x)) = 2*tt = 2.
         g(x) = (x + @) (x + @^2)
      If b0 = 0, tt = 2. deg(g(x)) = 2*tt = 4.
         g(x) = (x + 1) (x + @) (x + @^2) (x + @^3)
*/
{
   int i, j;
   int16_t *alpha_to;  // Galois field 2^MM from polynomial
   int16_t *index_of;  // Galois field 2^MM from polynomial
   int16_t *gg;        // Generator polynomial

   alpha_to = rscode->alpha_to;
   index_of = rscode->index_of;
   gg = rscode->gg;

   gg[0] = alpha_to[b0];
   gg[1] = 1 ;    /* g(x) = (@^b0 + x) initially */
   for (i = 2; i <= rscode->nn-rscode->kk; i++)
   {
      gg[i] = 1;
      /* gn(x) = (gg[0] + gg[1]*x + ... + gg[i]*x^i) * (@^(b0+i-1) + x) */
      for (j = i-1; j > 0; j--) {
         if (gg[j] != 0)
            gg[j] = gg[j-1] ^ alpha_to[((index_of[gg[j]]) +b0 +i -1)%rscode->nn]; // 2 terms contribute to x^j in gn(x)
         else
            gg[j] = gg[j-1]; // No x^(j-1) term in gg(x).
      }
      gg[0] = alpha_to[((index_of[gg[0]]) +b0 +i -1)%rscode->nn] ;     /* gg[0] can never be zero */
   }
   /* convert gg[] to index form for quicker encoding */
   for (i=0; i <= rscode->nn-rscode->kk; i++)  gg[i] = index_of[gg[i]];
}
#endif

static void encode_rs(RSCodeAttr_t const *rscode)
/*
   0) INPUT: recd[], bb[] in poly-form but gg[] in index-form. Tables.
   1) OUTPUT: parity symbols bb[0,..., nn-kk-1] in poly form.
   2) Takes string of info-symbols in recd[i], i=0..(kk-1) <-> recd(x)
      and encodes systematically to produce
      Output = 2*tt = (nn-kk) parity-symbols in bb[0]...bb[2*tt-1] <-> bb(x).
      bb[] is in poly-form.
   3) recd[] is info-symbols, bb[] is parity-symbols.
   4) Encoding is done by using a feedback shift register with appropriate
      connections specified by the elements of gg[], which was generated above.
      Codeword is c(x) = recd(x)* x^(nn-kk) + b(x)
*/
{
   int32_t i, j;
   int16_t feedback;
   int16_t kk, nn;           // RS variables
   int16_t const *alpha_to;  // Galois field 2^MM from polynomial
   int16_t const *index_of;  // Galois field 2^MM from polynomial
   int16_t const *gg;        // Generator polynomial

   kk = rscode->kk;
   nn = rscode->nn;
   gg = rscode->gg;
   alpha_to = rscode->alpha_to;
   index_of = rscode->index_of;

   for (i=0; i < nn-kk; i++) bb[i] = 0;
   for (i= kk -1; i >= 0; i--)
   {
      feedback = index_of[recd[i] ^ bb[(nn-kk)-1]];

      if (feedback != -1) /* feedback term is non-zero */
      {
         for (j = (nn-kk)-1; j > 0; j--) {
            if (gg[j] != -1)
               bb[j] = bb[j-1] ^ alpha_to[(gg[j] + feedback)%nn];
            else
               bb[j] = bb[j-1];
         }
         bb[0] = alpha_to[(gg[0] + feedback)%nn];
      }
      else /* feedback term is zero. encoder becomes a single-byte shifter */
      {
         for (j = (nn-kk)-1; j > 0; j--)
            bb[j] = bb[j-1] ;
         bb[0] = 0 ;
      }
   }
}

static bool eras_dec_rs(RSCodeAttr_t const *rscode, int16_t no_erase)
/*
    0) INPUT: recd[] in poly form, nn, kk, Tables.
                 int no_eras = # of erasures in i/p received vector.
                 int [16] eras_pos[] = Error Location Numbers (@^err_loc[l] where 0 <= err_loc[0] < err_loc[1] < ... n).
    1) OUTPUT: DecodedCW recd[] in poly form (if successful) OR un-changed recd[].
    2) Perform Errors + Erasures decoding of RS-code.
    4) Return value is 1 when success, 0 when failure to decode
*/
{
   int16_t i, j, r, q, tmp, /*tmp1,*/ num1, den, pres_root, pres_loc;
   int16_t phi[2*TT+1] = {0}, tmp_pol[2*TT+1] = {0}; /* The Erasure locator polynomial (RLP) in poly form */
   int16_t /*U,*/ discr_r, deg_phi, deg_lambda, L, deg_omega;
   int16_t lambda[2*TT+1] = {0}, s[2*TT+1] = {0}, lambda_pr[2*TT+1];/* Error+Erasure Locator Poly (ERLP) and Syndrome poly */
   int16_t b[2*TT+1] = {0}, T[2*TT+1], omega[2*TT+1];
   int16_t syn_error=0, root[2*TT] = {0}, err[NN] = {0}, reg[2*TT+1] = {0} ;
   int16_t loc[2*TT] = {0}, cnt = 0;
   bool result = true; // Default to success
   int16_t kk, nn, tt;       // RS variables
   int16_t const *alpha_to;  // Galois field 2^MM from polynomial
   int16_t const *index_of;  // Galois field 2^MM from polynomial

   kk = rscode->kk;
   nn = rscode->nn;
   tt = rscode->tt;
   alpha_to = rscode->alpha_to;
   index_of = rscode->index_of;

#if 0
   // MKD 2016-06-15 Disabled erasure for now
   if (no_erase > 0)
   {
      phi[0] = 1; // phi(x) = 1 + x*eras_pos[0] in poly-form.
      phi[1] = alpha_to[eras_pos[0]];
      for (i= 1; i < no_erase; i++)
      { // (phi(0) + phi(1)*x + phi(2)*x^2 + ... ) * (1 + U*x), U = eras_pos[i].
         U = eras_pos[i];
         for (j= 1;j < i+2; j++)
         {
            tmp1 = index_of[phi[j-1]];
            tmp_pol[j] = (tmp1 == -1) ?  0 : alpha_to[(U + tmp1)%nn];
         }

         for (j=1;j < i+2;j++)
            phi[j] = phi[j]^tmp_pol[j];
      }

      /* put phi[x] in index form */
      for (i=0; i < (nn-kk)+1; i++) phi[i] = index_of[phi[i]];

      /* find roots of the erasure location polynomial */
      for (i=1; i <= no_erase; i++) reg[i] = phi[i] ;
      cnt = 0 ;
      for (i=1; i <= nn; i++)
      {
         q = 1 ;
         for (j=1; j <= no_erase; j++)
            if (reg[j] != -1)
            {
               reg[j] = (reg[j]+j)%nn ;
               q ^= alpha_to[(reg[j])%nn] ;
            }

         if (!q)        /* store root and error location number indices */
         {
            root[cnt] = i;
            loc[cnt] = nn-i ;
            cnt++ ;
         }
      }

      if (cnt != no_erase)
      {
         return (false);
      }
   }
#endif
   /* recd[] is in polynomial form, convert to index form */
   for (i= 0; i < nn; i++) recd[i] = index_of[recd[i]];

   /* first form the syndromes; i.e., evaluate recd(x) at roots of g(x) namely
      @^(b0+i), i = 0, ... ,(2*tt-1) */
   for (i=1; i <= nn-kk; i++)
   {
      s[i] = 0 ;
      for (j=0; j < nn; j++)
         if (recd[j] != -1)
            s[i] ^= alpha_to[(recd[j] + (b0+i-1)*j)%nn] ; /* recd[j] in index form */
      /* convert syndrome from polynomial form to index form  */
      if (s[i] != 0)  syn_error = 1 ;   /* set flag if non-zero syndrome => error */
      s[i] = index_of[s[i]] ;
   }

   if (syn_error) /* if syndrome is zero, modified recd[] is a codeword */
   {
      /* Begin Berlekamp-Massey algorithm to determine error+erasure locator polynomial */
      r = no_erase;
      deg_phi = no_erase;
      L = no_erase;
      if (no_erase > 0) /* Initialize lambda(x) and b(x) (in poly-form) to phi(x) */
      {
         for (i=0; i < deg_phi +1; i++)
            lambda[i] = (phi[i] == -1)? 0 : alpha_to[phi[i]];

         for (i=deg_phi+1;i < 2*tt+1;i++) lambda[i] = 0;

         for (i= 0; i < 2*tt +1; i++)
            b[i] = lambda[i];
      }
      else
      {
         lambda[0] = 1;
         for (i=1;i < 2*tt+1;i++) lambda[i] = 0;
         for (i=0;i < 2*tt+1;i++) b[i] = lambda[i];
      }

      while (++r <= 2*tt) /* r is the step number */
      {
         /* Compute discrepancy at the r-th step in poly-form */
         discr_r = 0;
         for (i= 0; i < 2*tt +1; i++)
         {
            if ((lambda[i] != 0) && (s[r-i] != -1))
            {
               tmp = alpha_to[(index_of[lambda[i]]+s[r-i])%nn];
               discr_r ^= tmp;
            }
         }

         if (discr_r == 0)
         {
            /* 3 lines below: B(x) <-- x*B(x) */
            tmp_pol[0] = 0;
            for (i=1;i < 2*tt+1;i++) tmp_pol[i] = b[i-1];
            for (i=0;i < 2*tt+1;i++) b[i] = tmp_pol[i];
         }
         else
         {
            /* 5 lines below: T(x) <-- lambda(x) - discr_r*x*b(x) */
            T[0] = lambda[0];
            for (i=1;i < 2*tt+1;i++)
            {
               tmp =  (b[i-1] == 0)? 0 : alpha_to[(index_of[discr_r]+index_of[b[i-1]])%nn];
               T[i] = lambda[i]^tmp;
            }

            if (2*L <= r+no_erase-1)
            {
               L = r+no_erase-L;
               /* 2 lines below: B(x) <-- inv(discr_r) * lambda(x) */
               for (i=0; i < 2*tt+1; i++)
                  b[i] = (lambda[i] == 0) ? 0 : alpha_to[((index_of[lambda[i]]-index_of[discr_r])+nn)%nn];
               for (i=0; i < 2*tt+1; i++) lambda[i] = T[i];
            }
            else
            {
               for (i=0; i < 2*tt+1; i++) lambda[i] = T[i];
               /* 3 lines below: B(x) <-- x*B(x) */
               tmp_pol[0] = 0;
               for (i=1; i < 2*tt+1; i++) tmp_pol[i] = b[i-1];
               for (i=0; i < 2*tt+1; i++) b[i] = tmp_pol[i];
            }
         }
      }

      /* Put lambda(x) into index form */
      for (i=0; i < 2*tt+1; i++)
         lambda[i] = index_of[lambda[i]];

      /* Compute deg(lambda(x)) */
      deg_lambda = (int16_t)(2*tt);
      while ((lambda[deg_lambda] == -1) && (deg_lambda > 0))
         --deg_lambda;

      if (deg_lambda <= 2*tt)
      {
         /* Find roots of the error+erasure locator polynomial. By Chien Search */
         for (i=1; i < 2*tt+1; i++) reg[i] = lambda[i] ;
         cnt = 0 ; /* Number of roots of lambda(x) */
         for (i=1; i <= nn; i++)
         {
            q = 1 ;
            for (j=1; j <= deg_lambda; j++)
            if (reg[j] != -1)
            {
               reg[j] = (reg[j]+j)%nn ;
               q ^= alpha_to[(reg[j])%nn] ;
            }
            if (!q)        /* store root (index-form) and error location number */
            {
               root[cnt] = i;
               loc[cnt] = nn-i;
               cnt++;
            }
         }

         if (deg_lambda == cnt) // Compute err+eras evaluator poly omega(x) = s(x)*lambda(x) (modulo x^(nn-kk)).
         {  /* correctable error */
            for (i=0; i < 2*tt; i++)
            {
               omega[i] = 0;
               for (j=0;(j < deg_lambda+1) && (j < i+1);j++)
               {
                  if ((s[i+1-j] != -1) && (lambda[j] != -1))
                     tmp = alpha_to[(s[i+1-j]+lambda[j])%nn];
                  else
                     tmp = 0;
                  omega[i] ^= tmp;
               }
            }

            omega[2*tt] = 0;
            /* Compute lambda_pr(x) = formal derivative of lambda(x) in poly-form */
            for (i= 0; i < tt; i++)
            {
               lambda_pr[2*i+1] = 0;
               lambda_pr[2*i] = (lambda[2*i+1] == -1)? 0 : alpha_to[lambda[2*i+1]];
            }

            lambda_pr[2*tt] = 0;
            /* Compute deg(omega(x)) */
            deg_omega = (int16_t)(2*tt);
            while ((omega[deg_omega] == 0) && (deg_omega > 0))
               --deg_omega;

            /* Compute error values in poly-form. num1 = omega(inv(X(l))),
            num2 = inv(X(l))^(b0-1) and den = lambda_pr(inv(X(l))) all in poly-form */
            for (j= 0; j < cnt; j++)
            {
               pres_root = root[j];
               pres_loc = loc[j];
               num1 = 0;
               for (i=0;i < deg_omega+1;i++)
               {
                  if (omega[i] != 0)
                     tmp = alpha_to[(index_of[omega[i]]+i*pres_root)%nn];
                  else
                     tmp = 0;

                  num1 ^= tmp;
               }

               den = 0;
               for (i= 0; i < deg_lambda +1; i++)
               {
                  if (lambda_pr[i] != 0)
                     tmp = alpha_to[(index_of[lambda_pr[i]]+i*pres_root)%nn];
                  else
                     tmp = 0;

                  den ^= tmp;
               }

               err[pres_loc] = 0;
               if (num1 != 0)
               {
                  err[pres_loc] = alpha_to[(index_of[num1]+index_of[alpha_to[0]]+(nn-index_of[den]))%nn];
               }
            }

            /* Correct word by subtracting out error bytes. First convert recd[] into poly-form */
            for (i=0; i < nn; i++) recd[i] = (recd[i] == -1)? 0 : alpha_to[recd[i]];
            for (j=0; j < cnt; j++)
               recd[loc[j]] ^= err[loc[j]];

            return (true);
         }
         else /* deg(lambda) unequal to number of roots => uncorrectable error detected */
            result = false;
      }
      else /* deg(lambda) > 2*tt => uncorrectable error detected */
         result = false;
   }

   /* Convert recd[] to polynomial form */
   for (i=0; i < nn; i++) {
      if (recd[i]!=-1)
         recd[i] = alpha_to[recd[i]];
      else recd[i] = 0;
   }

   return result;
}


void RS_Encode(RSCode_e rscode, uint8_t const * const msg, uint8_t * const parity, uint8_t len)
{
   uint16_t i; // Loop counter
   uint16_t mask;
   uint16_t mm2;
   uint16_t tt;
   uint32_t slen; // Message length in symbols

   mm2 = (uint16_t)RSCode[rscode].mm;
   tt = (uint16_t)RSCode[rscode].tt;
   mask = (uint16_t)(1u << (mm2-1));

#if 0
   // Use this code to compute alpha_to, index_of and gg when adding a new RS code
   /* Generate the Galois Field GF(2**mm2) */
   generate_gf(&RSCode[rscode]);

   /* Compute the generator polynomial for this RS code */
   gen_poly(&RSCode[rscode]);
#endif

   (void)memset(recd, 0, sizeof(recd));

   // Different processing for STAR and SRFN
   if (rscode == RS_STAR_63_59) {
      // Remap msg into an array of Galois field 2^mm bit by bit LSB first
      // Also map symbols in reverse order (i.e first symbol is a the end of recd array)
      slen = ((len+2)/3)<<2;      // Entire packet length in symbols (padded with 0's)
      for (i=0; i<len*8; i++) {
         if (msg[i/8] & (1<<(i%8))) {
            recd[(slen-(i/mm2))-1] |= (int16_t)(1u<<(i%mm2));
         }
      }
   } else {
      // Remap msg into an array of Galois field 2^mm bit by bit MSB first
      for (i=0; i<len*8; i++) {
         if (msg[i/8] & (0x80>>(i%8))) {
            recd[i/mm2] |= (int16_t)(mask >> (i%mm2));
         }
      }
   }

   (void)memset(bb, 0, sizeof(bb));
   encode_rs(&RSCode[rscode]);

   // Add parity to msg using a Galois field 2^mm
   /*lint --e{661,662} no out of bounds errors in setting parity bits   */
   // Different processing for STAR and SRFN
   if (rscode == RS_STAR_63_59) {
      // Reverse FEC order so that values of bb[0], bb[1], bb[2] and bb[3] are move to bb[3], bb[2], bb[1] and bb[0]
      // bb[4] is temporary holder
      bb[4] = bb[0]; // save to temp
      bb[0] = bb[3]; // swap bb[0] and bb[3]
      bb[3] = bb[4];
      bb[4] = bb[1]; // save to temp
      bb[1] = bb[2]; // swap bb[1] and bb[2];
      bb[2] = bb[4];

      for (i=0; i<2*tt*mm2; i++) {
         if (bb[i/mm2] & (int16_t)(1u << (i%mm2))) {
            parity[i/8] |= 1u<<(i%8);
         } else {
            parity[i/8] &= ~(1u<<(i%8));
         }
      }
   } else {
      for (i=0; i<2*tt*mm2; i++) {
         if (bb[i/mm2] & (int16_t)(mask >> (i%mm2))) {
            parity[i/8] |= 0x80>>(i%8);
         } else {
            parity[i/8] &= ~(0x80>>(i%8));
         }
      }
   }
}

bool RS_Decode(RSCode_e rscode, uint8_t * const msg, uint8_t * const parity, uint8_t len)
{
   uint16_t i; // Loop counter
   uint16_t j; // Index
   uint16_t mask;
   uint16_t kk, mm2, nn; // RS variables
   uint16_t offset; // Size of zero padding when doing STAR
   bool     result;
   uint32_t slen=0; // Message length in symbols

   kk = (uint16_t)RSCode[rscode].kk;
   mm2 = (uint16_t)RSCode[rscode].mm;
   nn = (uint16_t)RSCode[rscode].nn;
   mask = (uint16_t)(1u << (mm2-1));

   (void)memset(recd, 0, sizeof(recd));

   // Different processing for STAR and SRFN
   if (rscode == RS_STAR_63_59) {
      // Remap msg into an array of Galois field 2^mm bit by bit LSB first
      // Also map symbols in reverse order (i.e first symbol is a the end of recd array)
      slen = ((len+2)/3)<<2;      // Entire packet length in symbols (padded with 0's)
      for (i=0; i<len*8; i++) {
         if (msg[i/8] & (1<<(i%8))) {
            recd[(slen-(i/mm2))-1] |= (int16_t)(1u<<(i%mm2));
         }
      }

      // Remap parity bits while leaving a gap because of the shortened block
      for (i=(uint16_t)(RSCode[rscode].kk*mm2), j=0; i<RSCode[rscode].nn*mm2; i++, j++) {
         if (parity[j/8] & (1<<(j%8))) {
            recd[(nn+kk-(i/mm2))-1] |= (int16_t)(1u<<(i%mm2));
         }
      }
   } else {
      // Remap msg into an array of Galois field 2^mm bit by bit
      for (i=0; i<len*8; i++) {
         if (msg[i/8] & (0x80>>(i%8))) {
            recd[i/mm2] |= (int16_t)(mask >> (i%mm2));
         }
      }

      // Remap parity bits while leaving a gap because of the shortened block
      for (i=(uint16_t)(RSCode[rscode].kk*mm2), j=0; i<RSCode[rscode].nn*mm2; i++, j++) {
         if (parity[j/8] & (0x80>>(j%8))) {
            recd[i/mm2] |= (int16_t)(mask >> (i%mm2));
         }
      }
   }

   // Decode message
   result=eras_dec_rs(&RSCode[rscode], 0);

   // Remap symbols into msg
   // Different processing for STAR and SRFN
   if (rscode == RS_STAR_63_59) {
       for (i=0; i<(slen*mm2); i++) {
          if (recd[(slen-(i/mm2))-1] & (int16_t)(1u<<(i%mm2))) { //lint !e661 !e662  Possible creation of out-of-bounds pointer
             msg[i/8] |= (int16_t)(1u<<(i%8));
          } else {
             msg[i/8] &= ~(int16_t)(1u<<(i%8));
          }
       }

       // Compute how many 0's to add between message and FEC
       offset = (uint16_t)((slen*mm2/8)-len);

       // Remap parity bits while leaving a gap because of the shortened block
       for (i=(uint16_t)(RSCode[rscode].kk*mm2), j=0; i<RSCode[rscode].nn*mm2; i++, j++) {
          if (recd[(nn+kk-(i/mm2))-1] & (uint16_t)(1u<<(i%mm2))) {
             parity[(j/8)+offset] |= 1u<<(j%8);
          } else {
             parity[(j/8)+offset] &= ~(1u<<(j%8));
          }
       }
   } else {
      for (i=0; i<len*8; i++) {
         if (recd[i/mm2] & (mask >> (i%mm2))) {
            msg[i/8] |= (0x80>>(i%8));
         } else {
            msg[i/8] &= ~(0x80>>(i%8));
         }
      }

      // Remap parity bits symbols into msg
      (void)memset(parity, 0, RS_GetParityLength(rscode));
      for (i=(uint16_t)(kk*mm2), j=0; i<nn*mm2; i++, j++) {
         if (recd[i/mm2] & (mask >> (i%mm2))) {
            parity[j/8] |= 0x80>>(j%8);
         } else {
            parity[j/8] &= ~(0x80>>(j%8));
         }
      }
   }

   // Check that the decoder didn't jump to the wrong codeword by changing the padding.
   if (result) {
      // Compute first bit position used as padding
      if (rscode == RS_STAR_63_59) {
         i = (uint16_t)(slen*mm2);
      } else {
         i = len*8;
      }
      // Make sure that padding symbols are still 0.
      // If padding is not 0 then it means that the decoder jumped to the wrong codeword (false positive)
      for (; i<kk*mm2; i++) {
         if (recd[i/mm2] & (mask >> (i%mm2))) {
            result = false;
            break;
         }
      }
   }

   return (result);
}


#if 0
void testrs()
{
  register int i;

/* generate the Galois Field GF(2**mm) */
  generate_gf() ;
  printf("Look-up tables for GF(2**%2d)\n",MM) ;
  printf("  i   alpha_to[i]  index_of[i]\n") ;
//  for (i=0; i<=NN; i++)
//   printf("%3d      %3d          %3d\n",i,alpha_to[i],index_of[i]) ;
//  printf("\n\n") ;

/* compute the generator polynomial for this RS code */
  gen_poly() ;


/* for known data, stick a few numbers into a zero codeword. Data is in
   polynomial form.
*/
for  (i=0; i<KK; i++)   data[i] = 0 ;

/* for example, say we transmit the following message (nothing special!) */
data[0] = 8 ;
data[1] = 6 ;
data[2] = 8 ;
data[3] = 1 ;
data[4] = 2 ;
data[5] = 4 ;
//data[6] = 15 ;
//data[7] = 9 ;
//data[8] = 9 ;

/* encode data[] to produce parity in bb[].  Data input and parity output
   is in polynomial form
*/
  encode_rs() ;

/* put the transmitted codeword, made up of data plus parity, in recd[] */
  for (i=0; i<NN-KK; i++)  recd[i] = bb[i] ;
  for (i=0; i<KK; i++) recd[i+NN-KK] = data[i] ;

/* if you want to test the program, corrupt some of the elements of recd[]
   here. This can also be done easily in a debugger. */
/* Again, lets say that a middle element is changed */
//  recd[nn-nn/2] = 3 ;
  recd[2*TT+3] = 3 ;

  for (i=0; i<NN; i++)
     recd[i] = index_of[recd[i]] ;          /* put recd[i] into index form */

/* decode recv[] */
  decode_rs() ;         /* recd[] is returned in polynomial form */

/* print out the relevant stuff - initial and decoded {parity and message} */
  printf("Results for Reed-Solomon code (n=%3d, k=%3d, t= %3d)\n\n",NN,KK,TT) ;
  printf("  i  data[i]   recd[i](decoded)   (data, recd in polynomial form)\n");
  for (i=0; i<NN-KK; i++)
    printf("%3d    %3d      %3d\n",i, bb[i], recd[i]) ;
  for (i=NN-KK; i<NN; i++)
    printf("%3d    %3d      %3d\n",i, data[i-NN+KK], recd[i]) ;
}
#endif
