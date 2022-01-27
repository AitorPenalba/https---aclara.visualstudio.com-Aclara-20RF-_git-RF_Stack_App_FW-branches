/*
 * File:     general_RS.c
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

#include "general_RS.h"

void generate_gf()
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
   register int i, mask ;

   // obtain polynomial representation of @^m = (p(0), p(1),...,p(mm-1))
   // where primitive-poly = p(0) + p(1)x + ... p(mm-1)x^(mm-1) + x^mm
   mask = 1 ;
   alpha_to[mm] = 0 ;
   for (i=0; i < mm; i++)
   {
      alpha_to[i] = mask ; //Since @^i has poly-form (0,...0,1,0,...0) for 0 <= i < mm
      index_of[alpha_to[i]] = i ;
      if (pp[i] != 0) /* If pp[i] == 1 then, term @^i occurs in poly-repr of @^mm */
         alpha_to[mm] ^= mask ;  /* Bit-wise EXOR operation */
      mask <<= 1 ; /* single left-shift */
   }
   index_of[alpha_to[mm]] = mm ;

   /*
     Have obtained poly-repr of @^mm. Poly-repr of @^(i+1) is given by
     poly-repr of @^i shifted left one-bit and accounting for any @^mm
     term that may occur when poly-repr of @^i is shifted.
   */
   mask >>= 1; // mask should equal 2^(mm-1)
   for (i = mm +1; i < nn; i++)
   {
      if (alpha_to[i-1] >= mask) //if @^(i-1) has 1 in @^(m-1) position, @^i must account for poly-repr of @^mm.
        alpha_to[i] = alpha_to[mm] ^ ((alpha_to[i-1]^mask)<<1) ;
      else
         alpha_to[i] = alpha_to[i-1] << 1; // since @^(i-1) has poly-form representation < 2^(mm-1)

      index_of[alpha_to[i]] = i;
   }
   index_of[0] = -1; //Since no power of @ has poly-repr (0,...,0)
 }

void gen_poly()
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
   register int i, j;

   gg[0] = alpha_to[b0];
   gg[1] = 1 ;    /* g(x) = (@^b0 + x) initially */
   for (i = 2; i <= nn-kk; i++)
   {
      gg[i] = 1;
      /* gn(x) = (gg[0] + gg[1]*x + ... + gg[i]*x^i) * (@^(b0+i-1) + x) */
      for (j = i-1; j > 0; j--)
         if (gg[j] != 0)
            gg[j] = gg[j-1] ^ alpha_to[((index_of[gg[j]]) +b0 +i -1)%nn]; // 2 terms contribute to x^j in gn(x)
        else
            gg[j] = gg[j-1]; // No x^(j-1) term in gg(x).

      gg[0] = alpha_to[((index_of[gg[0]]) +b0 +i -1)%nn] ;     /* gg[0] can never be zero */
   }
   /* convert gg[] to index form for quicker encoding */
   for (i=0; i <= nn-kk; i++)  gg[i] = index_of[gg[i]];
 }

void encode_rs()
/*
   0) INPUT: data[], bb[] in poly-form but gg[] in index-form. Tables.
	1) OUTPUT: parity symbols bb[0,..., nn-kk-1] in poly form.
   2) Takes string of info-symbols in data[i], i=0..(kk-1) <-> data(x)
      and encodes systematically to produce
      Output = 2*tt = (nn-kk) parity-symbols in bb[0]...bb[2*tt-1] <-> bb(x).
      bb[] is in poly-form.
   3) data[] is info-symbols, bb[] is parity-symbols.
   4) Encoding is done by using a feedback shift register with appropriate
      connections specified by the elements of gg[], which was generated above.
      Codeword is c(x) = data(x)* x^(nn-kk) + b(x)
*/
 {
   register int i, j;
   int feedback;

   for (i=0; i < nn-kk; i++) bb[i] = 0;
   for (i= kk -1; i >= 0; i--)
   {
      feedback = index_of[data[i] ^ bb[nn-kk-1]];

      if (feedback != -1) /* feedback term is non-zero */
      {
         for (j = nn -kk -1; j > 0; j--)
            if (gg[j] != -1)
               bb[j] = bb[j-1] ^ alpha_to[(gg[j] + feedback)%nn];
            else
               bb[j] = bb[j-1];

            bb[0] = alpha_to[(gg[0] + feedback)%nn];
      }
      else /* feedback term is zero. encoder becomes a single-byte shifter */
      {
         for (j = nn -kk -1; j > 0; j--)
            bb[j] = bb[j-1] ;
         bb[0] = 0 ;
      }
   }
 }

int decode_rs()
/*
	0) INPUT: recd[] in poly form, nn, kk, Tables.
	1) OUTPUT: DecodedCW recd[] in poly form (if successful) OR un-changed recd[].
	2) Performs ERRORS-ONLY decoding of RS codes.
	   If channel caused no more than "tt" errors, the tranmitted codeword will
		be recovered.
	3) If decoding is successful, writes the codeword into recd[].
		Otherwise recd[] is unaltered.
   4) Return value is 0-3 which indicates 4 cases
      0 => received vector is a codeword (all syndromes = 0)
      1 => decoder corrected <= tt errors. o/p is a codeword.
      2 => deg(Error-Location-Polynomial) != #roots(ELP). Uncorrectable.
      3 => deg(Error-Location-Polynomial) > tt. Uncorrectable.
*/
{
   register int i, j, u, q;
   int elp[nn-kk+2][nn-kk], d[nn-kk+2], l[nn-kk+2], u_lu[nn-kk+2], s[nn-kk+1] ;
   int count=0, syn_error=0, root[tt], loc[tt], z[tt+1], err[nn], reg[tt+1] ;

	/* recd[] is in polynomial form, convert to index form */
	for (i=0;i < nn;i++) recd[i] = index_of[recd[i]];

	/* first form the syndromes; i.e., evaluate recd(x) at roots of g(x) namely
		@^(b0+i), i = 0, ... ,(2*tt-1) */
   for (i=1; i <= nn-kk; i++)
	{
		s[i] = 0 ;
      for (j=0; j < nn; j++)
			if (recd[j] != -1) /* recd[j] is in index form */
				s[i] ^= alpha_to[(recd[j] + (b0+i-1)*j)%nn] ;
				/* convert syndrome from polynomial form to index form  */
      if (s[i] != 0) /* set flag if non-zero syndrome => error */
			syn_error = 1 ;
      s[i] = index_of[s[i]] ;
   }

	if (syn_error)       /* if errors, try and correct */
   {
      /* compute the error location polynomial via the Berlekamp iterative algorithm,
         following the terminology of Lin and Costello :   d[u] is the 'mu'th
         discrepancy, where u = 'mu'+ 1 and 'mu' (the Greek letter!) is the step number
         ranging from -1 to 2*tt (see L&C),  l[u] is the
         degree of the elp at that step, and u_l[u] is the difference between the
         step number and the degree of the elp.

         The notation is the same as that in Lin and Costello's book; pages 155-156 and 175.
      */
      /* initialise table entries */
      d[0] = 0 ;           /* index form */
      d[1] = s[1] ;        /* index form */
      elp[0][0] = 0 ;      /* index form */
      elp[1][0] = 1 ;      /* polynomial form */
      for (i=1; i<nn-kk; i++)
      {
         elp[0][i] = -1 ;   /* index form */
         elp[1][i] = 0 ;   /* polynomial form */
      }
      l[0] = 0 ;
      l[1] = 0 ;
      u_lu[0] = -1 ;
      u_lu[1] = 0 ;
      u = 0 ;

      do
      {
         u++ ;
         if (d[u]==-1)
         {
            l[u+1] = l[u] ;
            for (i=0; i<=l[u]; i++)
            {
               elp[u+1][i] = elp[u][i] ;
               elp[u][i] = index_of[elp[u][i]] ;
            }
         }
         else /* search for words with greatest u_lu[q] for which d[q]!=0 */
         {
            q = u-1 ;
            while ((d[q]==-1) && (q > 0))
               q-- ;
            /* have found first non-zero d[q]  */
            if (q > 0)
            {
               j = q;
               do
               {
                  j-- ;
                  if ((d[j]!=-1) && (u_lu[q]<u_lu[j]))
                     q = j ;
					} while (j > 0);
            }
            /* have now found q such that d[u]!=0 and u_lu[q] is maximum */
            /* store degree of new elp polynomial */
            if (l[u] > l[q] +u -q)
               l[u+1] = l[u] ;
            else
               l[u+1] = l[q]+u-q ;

            /* form new elp(x) */
            for (i=0; i < nn -kk; i++)
               elp[u+1][i] = 0 ;

            for (i=0; i <= l[q]; i++)
               if (elp[q][i] != -1)
                  elp[u+1][i+u-q] = alpha_to[(d[u]+nn-d[q]+elp[q][i])%nn];

            for (i=0; i<=l[u]; i++)
            {
               elp[u+1][i] ^= elp[u][i] ;
               elp[u][i] = index_of[elp[u][i]] ;  /*convert old elp value to index*/
            }
         }

         u_lu[u+1] = u-l[u+1] ;

         /* form (u+1)th discrepancy */
         if (u < nn-kk)    /* no discrepancy computed on last iteration */
         {
            if (s[u+1] != -1)
               d[u+1] = alpha_to[s[u+1]] ;
            else
               d[u+1] = 0 ;

            for (i=1; i <= l[u+1]; i++)
               if ((s[u+1-i] != -1) && (elp[u+1][i] != 0))
                  d[u+1] ^= alpha_to[ (s[u+1-i] + index_of[elp[u+1][i]] )%nn];

            d[u+1] = index_of[d[u+1]] ; /* put d[u+1] into index form */
         }
		} while ((u < nn - kk) && (l[u + 1] <= tt));

      u++ ; // Final deg(ELP) is held in l[u].
      if (l[u] <= tt) /* can correct errors if deg(ELP) <= tt */
		{
			/* put elp into index form */
			for (i= 0; i <= l[u]; i++) elp[u][i] = index_of[elp[u][i]];

			/* find all roots of the error location polynomial by exhaustive computation */
			for (i= 1; i <= l[u]; i++)
				reg[i] = elp[u][i] ; // since reg[] is working variable.

			count = 0 ; // Iteratively evaluate ELP(x) at @, @^2, @^3, ...
			for (i= 1; i <= nn; i++)
			{  // ELP(x) = 1 + reg[1] x + reg[2] x^2 + ...
				q = 1 ; // accounts for reg[0] = ELP[0] = 1.
				for (j= 1; j <= l[u]; j++)
					if (reg[j] != -1)
					{
                  reg[j] = (reg[j] + j)%nn ;
						q ^= alpha_to[reg[j]] ;
					}

				if (!q) /* store root and error location number indices */
				{
					root[count] = i;
					loc[count] = nn-i ;
					count++ ;
				}
			}

#ifndef NO_PRINT
			printf("Computed Error Locations\n");
			for (i=0;i < l[u];i++) printf("%d ",loc[i]);
			printf("\n");
#endif
			if (count == l[u])    /* count = #roots = deg(ELP). So #errors <= tt. */
			{
            /* form polynomial z(x) */
				for (i=1; i <= l[u]; i++)        /* Z[0] = 1 always - do not need */
            {
					if ((s[i]!=-1) && (elp[u][i]!=-1))
						z[i] = alpha_to[s[i]] ^ alpha_to[elp[u][i]] ;
					else if ((s[i]!=-1) && (elp[u][i]==-1))
						z[i] = alpha_to[s[i]] ;
               else if ((s[i]==-1) && (elp[u][i]!=-1))
                  z[i] = alpha_to[elp[u][i]] ;
               else
						z[i] = 0 ;

					for (j=1; j < i; j++)
						if ((s[j] != -1) && (elp[u][i-j] != -1))
							z[i] ^= alpha_to[(elp[u][i-j] + s[j])%nn] ;
					z[i] = index_of[z[i]]; /* put into index form */
				}

				/* evaluate errors at locations given by error location numbers loc[i] */
				for (i=0; i<nn; i++)
				{
					err[i] = 0 ;
					if (recd[i]!=-1)        /* convert recd[] to polynomial form */
						recd[i] = alpha_to[recd[i]] ;
					else
						recd[i] = 0 ;
				}

				for (i=0; i < l[u]; i++)    /* compute numerator of error term first */
            {
					err[loc[i]] = 1;       /* accounts for z[0] */
					for (j=1; j<=l[u]; j++)
					{
						if (z[j]!=-1)
							err[loc[i]] ^= alpha_to[(z[j]+j*root[i])%nn] ;

					} /* z(x) evaluated at X(l)^(-1) */

					if (err[loc[i]] != 0) /* term X(l)^(1-b0) */
                  err[loc[i]] = alpha_to[(index_of[err[loc[i]]]+root[i]*(b0+nn-1))%nn];

					if (err[loc[i]]!=0)
               {
						err[loc[i]] = index_of[err[loc[i]]] ;
						q = 0 ;     /* form denominator of error term */
						for (j=0; j<l[u]; j++)
							if (j!=i)
								q += index_of[1^alpha_to[(loc[j]+root[i])%nn]] ;

                  q = q % nn ;
						err[loc[i]] = alpha_to[(err[loc[i]]-q+nn)%nn] ;
						recd[loc[i]] ^= err[loc[i]] ;  /*recd[i] must be in polynomial form */
               }
				}

				return(1); // decode_flag = 1 indicates decoder able to correct errors.
			}
			else /* count = #roots != degree of elp => cannot solve. #errors > tt */
			{
           	for (i= 0; i < nn; i++)
				{
					if (recd[i] != -1) /* convert recd[] to polynomial form */
						recd[i] = alpha_to[recd[i]] ;
               else
						recd[i] = 0;  /* just output received word as is */
				}
				return(2); // decode_flag = 2 indicates deg(ELP) != #roots.
			}
		}
      else /* deg(elp has degree has degree >tt hence cannot solve */
		{
         for (i= 0; i < nn; i++)
         {
            if (recd[i] != -1) /* convert recd[] to polynomial form */
               recd[i] = alpha_to[recd[i]] ;
            else
               recd[i] = 0; /* just output received word as is */
         }
         return(3); // decode_flag = 3 indicates deg(ELP) > tt
      }
   }
   else /* no non-zero syndromes => no errors. put out received word as-is */
   {
      for (i=0; i < nn; i++)
      {
         if (recd[i] != -1)        /* convert recd[] to polynomial form */
            recd[i] = alpha_to[recd[i]] ;
         else
            recd[i] = 0 ;
      }
      return(0); // decode_flag = 0 indicates received vector is a codeword.
   }
}

int eras_dec_rs()
/*
	0) INPUT: recd[] in poly form, nn, kk, Tables.
				 int no_eras = # of erasures in i/p received vector.
				 int [16] eras_pos[] = Error Location Numbers (@^err_loc[l] where 0 <= err_loc[0] < err_loc[1] < ... n).
	1) OUTPUT: DecodedCW recd[] in poly form (if successful) OR un-changed recd[].
	2) Perform Errors + Erasures decoding of RS-code.
	4) Return value is 0-3 which indicates 4 cases
		0 => received vector is a codeword (all syndromes = 0)
		1 => decoder corrected <= tt errors. o/p is a codeword.
		2 => deg(Error-Location-Polynomial) != #roots(ELP). Uncorrectable.
		3 => deg(Error-Location-Polynomial) > tt. Uncorrectable.
*/
{
  register int i, j, r, u, q, tmp, tmp1, tmp2, num1, num2, den, pres_root, pres_loc;
  int phi[2*tt+1], tmp_pol[2*tt+1]; /* The Erasure locator polynomial (RLP) in poly form */
  int U, syn_err, discr_r, deg_phi, deg_lambda, L, deg_omega, t_after_eras;
  int lambda[2*tt+1], s[2*tt+1], lambda_pr[2*tt+1];/* Error+Erasure Locator Poly (ERLP) and Syndrome poly */
  int b[2*tt+1], T[2*tt+1], omega[2*tt+1];
  int syn_error=0,  root[2*tt],  z[tt+1],  err[nn],  reg[2*tt+1] ;
  int loc[2*tt], count = 0;

   /* Maximum # ch errs correctable after "no_eras" erasures */
   t_after_eras = (int)floor((2.0*tt -no_eras)/2.0);

   /* Compute erasure locator polynomial phi[x] */
   for (i=0; i < nn -kk +1; i++) tmp_pol[i] = 0;
   for (i=0; i < nn -kk +1; i++) phi[i] = 0;
   if (no_eras > 0)
   {
      phi[0] = 1; // phi(x) = 1 + x*eras_pos[0] in poly-form.
      phi[1] = alpha_to[eras_pos[0]];
      for (i= 1; i < no_eras; i++)
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
      for (i=0; i < nn-kk+1; i++) phi[i] = index_of[phi[i]];

      /* find roots of the erasure location polynomial */
      for (i=1; i <= no_eras; i++) reg[i] = phi[i] ;
      count = 0 ;
      for (i=1; i <= nn; i++)
      {
         q = 1 ;
         for (j=1; j <= no_eras; j++)
            if (reg[j] != -1)
            {
               reg[j] = (reg[j]+j)%nn ;
               q ^= alpha_to[(reg[j])%nn] ;
            }

         if (!q)        /* store root and error location number indices */
         {
            root[count] = i;
            loc[count] = nn-i ;
            count++ ;
         }
      }

      if (count != no_eras)
      {
         printf("\n phi(x) is WRONG\n"); exit(7);
      }
#ifndef NO_PRINT
      printf("\n Erasure positions as determined by roots of Eras Loc Poly:\n");
      for (i= 0; i < count; i++) printf("%d ",loc[i]);
      printf("\n");
#endif
   }

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
   	r = no_eras;
   	deg_phi = no_eras;
   	L = no_eras;
   	if (no_eras > 0) /* Initialize lambda(x) and b(x) (in poly-form) to phi(x) */
      {
        	for (i=0; i < deg_phi +1; i++)
            lambda[i] = (phi[i] == -1)? 0 : alpha_to[phi[i]];

         for (i=deg_phi+1;i < 2*tt+1;i++) lambda[i] = 0;

         deg_lambda = deg_phi;
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

            if (2*L <= r+no_eras-1)
            {
               L = r+no_eras-L;
               /* 2 lines below: B(x) <-- inv(discr_r) * lambda(x) */
               for (i=0; i < 2*tt+1; i++)
                  b[i] = (lambda[i] == 0) ? 0 : alpha_to[(index_of[lambda[i]]-index_of[discr_r]+nn)%nn];
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
      deg_lambda = 2*tt;
      while ((lambda[deg_lambda] == -1) && (deg_lambda > 0))
         --deg_lambda;

      if (deg_lambda <= 2*tt)
      {
         /* Find roots of the error+erasure locator polynomial. By Chien Search */
         for (i=1; i < 2*tt+1; i++) reg[i] = lambda[i] ;
         count = 0 ; /* Number of roots of lambda(x) */
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
               root[count] = i;
               loc[count] = nn-i;
               count++;
            }
         }
#ifndef NO_PRINT
         printf("\n Final error positions:\t");
         for (i=0;i < count;i++) printf("%d ",loc[i]);
         printf("\n");
#endif
         if (deg_lambda == count) // Compute err+eras evaluator poly omega(x) = s(x)*lambda(x) (modulo x^(nn-kk)).
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
            deg_omega = 2*tt;
            while ((omega[deg_omega] == 0) && (deg_omega > 0))
               --deg_omega;

            /* Compute error values in poly-form. num1 = omega(inv(X(l))),
            num2 = inv(X(l))^(b0-1) and den = lambda_pr(inv(X(l))) all in poly-form */
            for (j= 0; j < count; j++)
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

               num2 = alpha_to[(pres_root*(b0-1))%nn];
               den = 0;
               for (i= 0; i < deg_lambda +1; i++)
               {
                  if (lambda_pr[i] != 0)
                     tmp = alpha_to[(index_of[lambda_pr[i]]+i*pres_root)%nn];
                  else
                     tmp = 0;

                  den ^= tmp;
               }

               if (den == 0)
               {
                  printf("\n ERROR: denominator = 0\n");
               }

               err[pres_loc] = 0;
               if (num1 != 0)
               {
                  err[pres_loc] = alpha_to[(index_of[num1]+index_of[num2]+(nn-index_of[den]))%nn];
               }
            }

            /* Correct word by subtracting out error bytes. First convert recd[] into poly-form */
            for (i=0; i < nn; i++) recd[i] = (recd[i] == -1)? 0 : alpha_to[recd[i]];
            for (j=0; j < count; j++)
               recd[loc[j]] ^= err[loc[j]];

            return(1);
         }
         else /* deg(lambda) unequal to number of roots => uncorrectable error detected */
            return(2);
      }
      else /* deg(lambda) > 2*tt => uncorrectable error detected */
         return(3);
   }
   else
   {  /* no non-zero syndromes => no errors: output received codeword */
      for (i=0; i < nn; i++)
      {
         if (recd[i] != -1) /* convert recd[] to polynomial form */
            recd[i] = alpha_to[(recd[i])%nn];
         else
            recd[i] = 0;
      }

      return(0);
   }
}

main()
/*
	Testbench for RS-encoder and 2 types of RS-decoders; Errors-only, Errors+Erasures.
	1) Generates GaloisField (specified by primitive-polynomial) arithmetic tables.
	2) Calc the generator-polynomial of RS-code.
	3) Prompt user and read in code parameters from console
	4) Generate random info_symbols and RSencode them into CodeWords.
	5) Pass thro channel that causes symbol-errors (SER specified). Each symbol-error
	is random mm-bit pattern.
	6) At channel o/p decode with Errors-only or Errors+Erasures decoder
	7) Track whether Decoder has error-conditions (should not occur)
	8) Must be recompiled for each different generator-polynomial after
	setting parameters in general_RS.h.
	9) Handles shortened RS-code with 0-padding.
*/
{
  int no_pure_errs;
  int i, j, byte, error_byte, no_ch_errs, iter, error_flag;
  int received[nn], cw[nn], decode_flag, no_times_err_cond1, no_times_err_cond2;
  long seed;
  unsigned short int tmp;
  double x;

 /* Generate the Galois field GF(2^mm) */
   printf("Generating the Galois field GF(%d)\n\n",n);
   generate_gf();

#ifdef DEBUG
   printf("Look-up tables for GF(2^%2d): \n", mm);
   printf("i\t\t alpha_to[i]\t index_of[i]\n");
   for (i=0; i < n; i++)
      printf("%3d \t\t %#x \t\t %d\n",i,alpha_to[i],index_of[i]) ;
   printf("\n {@^i : 0 <= i < %d} in poly-form \n",n);
   printf("is binary-representation of unsigned-integer alpha_to[i] \n");
   printf("with LSBit = coef(@^0)\n");
   printf(".... MSBit = coef(@^(nn-kk-1))\n");
   printf("\n The log-to-base @ of GF element x whose poly-form \n");
   printf("= binary-representation of unsigned-integer \n");
   printf("j, 0 <= j < %d is given by the unsigned-integer index_of[j]\n",n);
   printf("Therefore, @^j = x \n");
   printf("\n EXAMPLES IN GF(256): \n");
   printf("The polynomial-form of @^8 is the binary-representation of alpha_to[8] = 0x1d\n");
   printf("So, @^8 = @^4 + @^3 + @^2 + @^0 \n");
   printf("The index of x whose poly-form is 0x72 (#155) is index_of[155] = 217");
   printf("So @^217 = @^6 + @^5 + @^4 + @^1 \n");
#endif

   printf("generator-poly = product of (X + @^(b0 + i)), i = 0, ... ,(2*tt-1). 2*tt = nn -kk. \n");

#ifdef SCANF
   printf("Enter b0 = index of the first root \n");
   scanf("%d",&b0);
#endif

   gen_poly();

   printf("\n g(x) of the %d-error correcting RS (%d,%d) code: \n",tt,nn,kk);
   printf("Coefficient is the exponent of @ = primitive element\n");
   for (i=0;i <= nn-kk ;i++)
   {
      printf("%d x^%d ",gg[i],i);
      if (i < nn-kk) printf(" + ");
      if (i && (i % 7 == 0)) printf("\n"); /* 8 coefficients per line */
   }
   printf("\n");

   printf("\n Coefficient is the representation in basis {@^%d,...,@,1}\n",mm-1);
   for (i=0;i <= nn-kk ;i++)
   {
      printf("%#x x^%d ",alpha_to[gg[i]],i);
      if (i < nn-kk) printf(" + ");
      if (i && (i % 7 == 0)) printf("\n"); /* 8 coefficients per line */
   }
  printf("\n\n");

#ifdef SCANF
  printf("Enter positive integer to seed Random # generator \n");
  scanf("%ld", &seed);
  srand(seed);
#endif

#ifdef ENCODE

#ifdef SCANF
   printf("Enter length of the shortened code\n");
   scanf("%d",&nn_short);
#endif

   if ((nn_short < 2*tt) || (nn_short > nn))
   {
      printf("Invalid entry %d for shortened length\n",nn_short);
      exit(1);
   }

	kk_short = kk - (nn -nn_short); /* compute dimension of shortened code */
   printf("The (%d,%d) %d-error correcting RS code\n",nn_short,kk_short,tt);

#ifdef SCANF
   printf("Enter the number of codewords desired \n");
   scanf("%d",&no_cws);
#endif

#endif

#ifdef DECODE

#ifdef SCANF
   printf("Enter Probability of symbol error \n");
   scanf("%le", &prob_symb_err);

	printf("Enter maximum #erasures to declare [0, %d] (actual may be smaller) \n",2*tt);
	scanf("%d", &max_no_eras);
#endif

	no_times_err_cond1 = 0;
	no_times_err_cond2 = 0;
   iter = -1;
   while (++iter < no_cws)
   {
		//============= Generate random info_symbols, Encode into CodeWord.
		// Pad with zeros rightmost (kk-kk_short) positions
		for (j = kk_short; j < kk; j++) data[j] = 0;

		for (j = 0; j < (int)kk_short; j++) data[j] = (int)(rand() % n);

		encode_rs();

		// CodeWord = [Parity_symbols (0 to nn-kk-1) : Info_symbols (0 to kk_short -1)]
		for (i= 0; i < nn_short; i++)
		{
			if (i < nn - kk)
				cw[i] = bb[i]; // fills cw[0,...,nn-kk-1]
			else if (i < nn_short)
				cw[i] = data[i - nn + kk]; // fills cw[nn-kk, nn_short-1] with data[0,...kk_short -1]
		}

#ifndef NO_PRINT
      printf("The Transmitted Codeword\n");
      for (i=0;i < nn_short;i++) printf("%02x ",cw[i]); printf("\n");
      printf("\n\n\n\n Transmitting codeword %d \n",iter);
      printf("Channel caused following errors Location (Error): \n");
#endif

		//============ Pass thro channel. Create random symbol errors.
		//	Keep track of #symbol errors caused by channel & their locations
		no_ch_errs = 0;
      for (i= 0; i < nn_short; i++)
      {
         x = (double)rand() / (double)RAND_MAX;
         if (x < prob_symb_err)
         {
            error_byte = (int) (rand() % n);
            received[i] = cw[i] ^ error_byte;
				errloc[no_ch_errs] = i;
            ++no_ch_errs;
#ifndef NO_PRINT
            printf("%d (%#x) ",i,error_byte);
#endif
         }
         else
            received[i] = cw[i];
      }

#ifndef NO_PRINT
      printf("\n");
      printf("Channel caused %d errors \n",no_ch_errs);
      for (i=0;i < nn_short;i++) printf("%02x ",received[i]); printf("\n");
#endif

      //=============== Pad with zeros and decode as if full-length primitive tt-error correcting RScode
      for (i= 0; i < nn-kk; i++)  recd[i] = received[i]; /* parity bytes */
      for (i= nn -kk; i < nn -kk +kk_short; i++) recd[i] = received[i]; /* info bytes */
      for (i= nn -kk + kk_short; i < nn; i++) recd[i] = 0; /* zero padding */

		if (DecoderType)
		{
			if (no_ch_errs <= max_no_eras)
				no_eras = no_ch_errs; // all channel errors are erased.
			else
				no_eras = max_no_eras; // some channel errors could not be erased.

			if (no_ch_errs <= no_eras)
				no_pure_errs = 0; // all channel errors were erased.
			else
				no_pure_errs = no_ch_errs - no_eras;

			for (i = 0; i < no_eras; i++)
				eras_pos[i] = errloc[i];

			decode_flag = eras_dec_rs(); // i/p must be in recd[]. o/p will be in recd[].
		}
		else
		{
			no_eras = 0;
			decode_flag = decode_rs(); // i/p must be in recd[]. o/p will be in recd[].
		}

		printf("#erasures = %d, #errors = %d, decoder returned = %d \n",no_eras, no_ch_errs, decode_flag);

#ifndef NO_PRINT
      printf("decode_rs() returned %d\n",decode_flag);
#endif
		//============================== Statistics Genie
		error_flag = 0; // error_flag doesn't care about errors in parity-symbols.
      for (i= 0; i < kk_short; i++)
      {					// error_flag only compares kk_short info_symbols at decoder o/p to encoder i/p.
         if (recd[i+nn-kk] != cw[i+nn-kk])
         {
            error_flag = 1;
            break;
         }
      }

		if (DecoderType == 0)
		{	// Verify that all ErrorPatterns with weight <= tt were corrected by RSdecoder.
			if (no_ch_errs <= tt && (decode_flag == 2 || decode_flag == 3))
			{	// Errors-only DECODER ERROR CONDITION (should not occur).
				printf("DECODER ERROR: #chan errs = %d <= %d but decode_flag = %d \n", no_ch_errs, tt, decode_flag);

				if (decode_flag == 2)
				{
					++no_times_err_cond1;
					printf("\n deg(lambda) unequal to #roots \n");
				}
				else if (decode_flag == 3)
				{
					++no_times_err_cond2;
					printf("\n deg(lambda) > 2*tt \n");
				}
			}
		}
		else
		{	// Verify that all ErrorPatterns with "r" Erasures & "e" Errors s.t. 2*e + r <= 2*tt were corrected.
			if ((no_eras + (2 * no_pure_errs) <= 2 * tt) && (decode_flag == 2 || decode_flag == 3))
			{	//  Errors + Erasures DECODER ERROR CONDITION (should not occur).
				printf("DECODER ERROR: #erasures = %d, #pure erros = %d but decode_flag = %d \n", no_eras, no_pure_errs, decode_flag);

				if (decode_flag == 2)
				{
					++no_times_err_cond1;
					printf("\n deg(lambda) unequal to #roots \n");
				}
				else if (decode_flag == 3)
				{
					++no_times_err_cond2;
					printf("\n deg(lambda) > 2*tt \n");
				}
			}
		}

   } // close while (++iter < no_cws)

	if (DecoderType)
		printf("Errors + Erasures RS(%d, %d) decoder: %d CWs decoded. Count(Error-Condition-1) = %d Count(Error-Condition-2) = %d \n", \
				nn, kk_short, no_cws, no_times_err_cond1, no_times_err_cond2);
#endif
}
