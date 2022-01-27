/*
* Reed-Solomon Encoder
* Copyright (c) 2005-2008 Aclara RF Systems, Inc.  All Rights Reserved.
* 7/15/2005 Glenn A. Emelko
*
* Adapted for MSP430 using GF(63) and generator x^6+x+1 on 3/24/06 GAE
*/

#include "rsfec.h"

// Encode Symbol - using GF(63) and generator x^6+x+1 compute adjusted symdrome values
//
// The generator x^6+x+1 in GF[63] creates the following RS poly's (6 bit):
//   Poly1 = x^4+x^3+x^2+x   0x1e
//   Poly2 = x^4+x^3+x^2+1   0x1d
//   Poly3 = x^4+1           0x11
//   Poly4 = x^5+x^4         0x30
//
// These can be aligned as bits to determine the locations where (sym^RS[0])^N is used
//    N:   6 5 4 3 2 1
// 0x1e => 0 1 1 1 1 0  (Poly1, used with RS[1])
// 0x1d => 0 1 1 1 0 1  (Poly2, used with RS[2])
// 0x11 => 0 1 0 0 0 1  (Poly3, used with RS[3])
// 0x30 => 1 1 0 0 0 0  (Poly4, used with RS[4])
//
// So (RS[0]^sym)^1 needs to be combined in RS[2] and RS[3]
// Likewise, (RS[0]^sym)^2 (in GF(63) of course) is combined with only RS[1]
// and so on...
//
// When we're done, shift the RS[] matrix left for the next round.
//

void EncodeSymbol (unsigned char sym, unsigned char * rs)
{
    rs[4]=0;
    sym ^= rs[0];
    rs[2] ^= sym;
    rs[3] ^= sym;
    if ((sym <<= 1) & 0x40) sym ^= 0x43; // Adjust for galois generator
    rs[1] ^= sym;
    if ((sym <<= 1) & 0x40) sym ^= 0x43; // Adjust for galois generator
    rs[1] ^= sym;
    rs[2] ^= sym;
    if ((sym <<= 1) & 0x40) sym ^= 0x43; // Adjust for galois generator
    rs[1] ^= sym;
    rs[2] ^= sym;
    if ((sym <<= 1) & 0x40) sym ^= 0x43; // Adjust for galois generator
    rs[1] ^= sym;
    rs[2] ^= sym;
    rs[3] ^= sym;
    rs[4] ^= sym;
    if ((sym <<= 1) & 0x40) sym ^= 0x43; // Adjust for galois generator
    rs[4] ^= sym;
    rs[0]=rs[1]; // Done, shift the syndrome left
    rs[1]=rs[2];
    rs[2]=rs[3];
    rs[3]=rs[4];
}

// RSEncode
// Pass a message in msb buffer of length len (in bytes) to encode
// make sure there's 3 spare bytes to postfix the FEC codes
//
// Note that thought the message is constructed with bytes, the GF is computed on 6
// bit boundaries as follows:
//
//    msg[0] = [B1 B0 A5 A4 A3 A2 A1 A0]
//    msg[1] = [C3 C2 C1 C0 B5 B4 B3 B2]
//    msg[2] = [D5 D4 D3 D2 D1 D0 C5 C4]
//    ... and so on.  When done, the reed-solomon code is postfixed to the end of msg
//    in the same way - four 6-bit symbols packed into 3 bytes.
//
// Note that the actual message may be upper-bit padded with 0's on the last byte(s) to
// an even 3-byte boundary... So any missing/partial symbols have 0's in their high bits.

void RSEncode (unsigned char * msg, unsigned char len)
{
    // Temporary holding locations for postfix code computation
    unsigned char rs[5],sym;
    int i,i1,flen;

    msg[len]=0;                 // Pad with 0's
    msg[len+1]=0;
    rs[0]=0;                    // Initialize RS computation (no need to init rs[4])
    rs[1]=0;
    rs[2]=0;
    rs[3]=0;

    flen = ((len+2)/3)<<2;      // Entire packet length in symbols (padded with 0's)

    for(i=0;i<flen;i++)
    {       // for each symbol
        i1=((i & 0xfc)*3)>>2;
        switch ((i & 3))
        {
          case 0:
            sym = msg[i1];                // Pull from location [A5..0]
            break;
          case 1:
            sym = (unsigned char)(((msg[i1+1]<<2) | (msg[i1]>>6))); // locations [B5..0]
            break;
          case 2:
            sym = (unsigned char)(((msg[i1+2]<<4) | (msg[i1+1]>>4))); // locations [C5..0]
            break;
          default:
            sym = msg[i1+2]>>2;           // Locations [D5..0]
        }
        EncodeSymbol(0x3f & sym,rs);     // Process symbol
    }
    msg[len+0] = (unsigned char)((rs[1]<<6) | rs[0]); // Pack last four FEC symbols into last three bytes
    msg[len+1] = (unsigned char)((rs[2]<<4) | (rs[1]>>2));
    msg[len+2] = (unsigned char)((rs[3]<<2) | (rs[2]>>4));
}
