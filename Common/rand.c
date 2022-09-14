/******************************************************************************
 *
 * Filename: rand.c
 *
 * Global Designator:
 *
 * Contents: Mersenne Twister RNG and an integer only implementation
 *          Also aclara_randu, aclara_randf for generating random numbers in a range
 *          of integers or floats, respectively
 *
 *
 ******************************************************************************
 * Copyright (c) 2015 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/
/* ***************************************************************************** */
/* Copyright:      Francois Panneton and Pierre L'Ecuyer, University of Montreal */
/*                 Makoto Matsumoto, Hiroshima University                        */
/* Notice:         This code can be used freely for personal, academic,          */
/*                 or non-commercial purposes. For commercial purposes,          */
/*                 please contact P. L'Ecuyer at: lecuyer@iro.UMontreal.ca       */
/* ***************************************************************************** */

/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <math.h>
#include <stdlib.h>
#include "project.h"
#if ( RTOS_SELECTION == MQX_RTOS )
#include <mqx.h>
#endif
#include "rand.h"

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/*lint -esym(750,W,R,P,M1,M2,M3,MAT0POS,MAT0NEG,MAT3NEG,MAT4NEG,V0,VM1,VM2,VM3,VRm1,VRm2,newV0,newV1,newVRm1,FACT)   */
#define W 32
#define R 16
#define P 0
#define M1 13
#define M2 9
#define M3 5

#define MAT0POS(t,v) (v^(v>>t))
#define MAT0NEG(t,v) (v^(v<<(-(t))))
#define MAT3NEG(t,v) (v<<(-(t)))
#define MAT4NEG(t,b,v) (v ^ ((v<<(-(t))) & b))

#define V0            STATE[state_i                   ]
#define VM1           STATE[(state_i+M1) & 0x0000000fU]
#define VM2           STATE[(state_i+M2) & 0x0000000fU]
#define VM3           STATE[(state_i+M3) & 0x0000000fU]
#define VRm1          STATE[(state_i+15) & 0x0000000fU]
#define VRm2          STATE[(state_i+14) & 0x0000000fU]
#define newV0         STATE[(state_i+15) & 0x0000000fU]
#define newV1         STATE[state_i                 ]
#define newVRm1       STATE[(state_i+14) & 0x0000000fU]

#define FACT 2.32830643653869628906e-10
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

#define WELL_DOUBLE_RAND_IMPLEMENTATION 0

#if WELL_DOUBLE_RAND_IMPLEMENTATION
static __no_init  uint32_t STATE[R];
static __no_init  uint32_t init[R];
static            uint32_t state_i = 0;
static            uint32_t z0, z1, z2;
#else
static uint32_t state[R] = { // The initial state has to strike a 50/50 balance of 1's and 0's.
   0x3EC3D424,               // Those numbers came from the internal state of a long run of rand().
   0xF83857DC,               // The state could be initialized from the security chip RNG when available.
   0xB070735B,
   0xE3DBB8C2,
   0xDC2DB7DE,
   0xC19C7EDF,
   0xEDD1D5BB,
   0x6DE445C1,
   0x935E6054,
   0xE38285C2,
   0x04B054C5,
   0xBCA16537,
   0x5D870ADF,
   0x5C86D6F9,
   0xA4786F32,
   0x771327BD };
static uint32_t index     = 0;            /* init should also reset this to 0 */
static bool     srandInit = (bool)false;  /* Srand was called */
#endif


/* ****************************************************************************************************************** */
/* LOCAL FUNCTION PROTOTYPES */

/* ****************************************************************************************************************** */
/* GLOBAL FUNCTION DEFINITIONS */
#if WELL_DOUBLE_RAND_IMPLEMENTATION

static void InitWELLRNG512a (uint32_t *init)
{
   int j;
   state_i = 0;
   for (j = 0; j < R; j++)
   {
     STATE[j] = init[j];
   }
}

static double WELLRNG512a (void)
{
  z0    = VRm1;
  z1    = MAT0NEG (-16,V0)    ^ MAT0NEG (-15, VM1);
  z2    = MAT0POS (11, VM2)  ;
  newV1 = z1                  ^ z2;
  newV0 = MAT0NEG (-2,z0)     ^ MAT0NEG(-18,z1)    ^ MAT3NEG(-28,z2) ^ MAT4NEG(-5,0xda442d24U,newV1) ;
  state_i = (state_i + 15) & 0x0000000fU;
  return ((double) STATE[state_i]) * FACT;
}

/******************************************************************************
 *
 * Function name: aclara_srand
 *
 * Purpose: initializes RNG with a seed
 *
 * Arguments: seed - Initial seed
 *
 * Returns: nothing
 *
 * Notes:
 *
 *****************************************************************************/
void aclara_srand(uint32_t seed)
{
   // This is faster than mutex lock since the call is quick
   OS_INT_disable( );
   InitWELLRNG512a( init );
   OS_INT_enable( );
}
/******************************************************************************
 *
 * Function name: aclara_rand
 *
 * Purpose: Generates a random number on [0,0xffffffff] interval
 *
 * Arguments: nothing
 *
 * Returns: 32 bit random number
 *
 * Notes:
 *
 *****************************************************************************/
int32_t aclara_rand(void)
{
   uint32_t y;
   double r;

   // This is faster than mutex lock since the call is quick
   OS_INT_disable( );

   r =  WELLRNG512a();
   r *= (double)(1<<32);
   y = (uint32_t)r;

   OS_INT_enable( );

   return (int32_t)y;
}
#else       /* Integer implementation of WELL, courtesy Chris Lomont: http://www.lomont.org/  */
/******************************************************************************
 *
 * Function name: aclara_srand
 *
 * Purpose: Seed the random number generator.
 *
 * Arguments: 32 bit seed
 *
 * Returns: nothing
 *
 * Notes:
 *
 * TODO: Use the security chip to seed this array.
 *
 *****************************************************************************/
void aclara_srand( uint32_t seed )
{
   for ( uint32_t i = 0; i < R; i++ )
   {
      // Perturb current states with seed
      state[ i ] ^= seed;
   }
   srandInit = (bool)true;
}

/******************************************************************************
 *
 * Function name: aclara_rand
 *
 * Purpose: Generates a random number on [0,0xffffffff] interval
 *
 * Arguments: nothing
 *
 * Returns: 32 bit random number
 *
 * Notes:
 *
 *****************************************************************************/
int32_t aclara_rand(void)
{
   uint32_t a;
   uint32_t b;
   uint32_t c;
   uint32_t d;

   // This is faster than mutex lock since the call is quick
   OS_INT_disable( );

   // Perturb original states if needed
   if ( ! srandInit ) {
      // Try to create some entropy by using temperature sensors.
      // It seems to be working fine overall.
      // Even the same board generates different seed values every time it is run.
#if ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84001_REV_A )   || \
      ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84020_1_REV_A ) || \
      ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84114_1_REV_A ) || \
      ( HAL_TARGET_HARDWARE == HAL_TARGET_Y99852_1_REV_A ) || \
      ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84030_1_REV_A ) || \
      ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84580_x_REV_A ) || \
      ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84580_x_REV_B ) )
      float temp1, temp2;
      uint32_t utemp1, utemp2;

#if ( MCU_SELECTED == NXP_K24 )
      temp1 = ADC_Get_uP_Temperature(TEMP_IN_DEG_F);
#elif ( MCU_SELECTED == RA6E1 )
      temp1 = ADC_Get_4V0_Voltage();
#endif
      temp2 = ADC_Get_SC_Voltage();

      utemp1 = *(uint32_t*)&temp1;  /*lint !e740 just want bit pattern from temp1   */
      utemp2 = *(uint32_t*)&temp2;  /*lint !e740 just want bit pattern from temp2   */

      aclara_srand(((utemp1 & 0xFFFF) << 16) | ((utemp2 & 0xFFFF)));
#elif ( ( HAL_TARGET_HARDWARE == HAL_TARGET_Y84050_1_REV_A ) || \
        (HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )    )
      float temp1, temp2, temp3;
      uint32_t utemp1, utemp2, utemp3;

      temp1 = ADC_Get_uP_Temperature( TEMP_IN_DEG_F );
      temp2 = ADC_Get_PS_Temperature( TEMP_IN_DEG_F );
      temp3 = ADC_Get_PA_Temperature( TEMP_IN_DEG_F );

      utemp1 = *(uint32_t*)&temp1;  /*lint !e740 just want bit pattern from temp1   */
      utemp2 = *(uint32_t*)&temp2;  /*lint !e740 just want bit pattern from temp2   */
      utemp3 = *(uint32_t*)&temp3;  /*lint !e740 just want bit pattern from temp3   */

      aclara_srand(((utemp1 & 0xFFFF) << 16) | ((utemp2 & 0xFF) << 8) | (utemp3 & 0xFF));
#else
#error "Unsupported TARGET HARDWARE"
#endif
   }

   a = state[index];
   c = state[(index+13)&15];
   b = a^c^(a<<16)^(c<<15);
   c = state[(index+9)&15];
   c ^= (c>>11);
   a = state[index] = b^c;
   d = a^((a<<5)&0xDA442D24UL);
   index = (index + 15)&15;
   a = state[index];
   state[index] = a^b^d^(a<<2)^(b<<18)^(c<<28);

   OS_INT_enable( );

   return (int32_t)state[index];
}
#endif

/******************************************************************************
 *
 * Function name: aclara_randf
 *
 * Purpose: Generates a floating point random number on [minVal,maxVal] interval
 *
 * Arguments: minVal, maxVal
 *
 * Returns: floating point random number from minVal to maxVal
 *
 * Notes:
 *
 *****************************************************************************/

float aclara_randf( float minVal, float maxVal )
{
   float retVal;
   float randVal;

   // Swap minVal with maxVal if needed
   if ( maxVal < minVal ) {
      retVal = minVal;
      minVal = maxVal;
      maxVal = retVal;
   }

   randVal = (uint32_t)aclara_rand();                 /* Get a random integer */
   retVal  = randVal/RAND_MAX;                        /* Convert random integer to floating point [0,1]  */

   retVal *= maxVal - minVal;  /* Get range desired    */
   retVal += minVal;           /* Get offset desired   */
   return retVal;
}


/******************************************************************************
 *
 * Function name: aclara_randu
 *
 * Purpose: Generates a integer point random number on [minVal,maxVal] interval
 *
 * Arguments: minVal, maxVal
 *
 * Returns: integer random number from minVal to maxVal
 *
 * Notes:
 *   This use floating point with a range of 0.0 to 1.0
 *   The floating point range is increased to 0.0 to 2.0.
 *      values from 0.0 to 0.999... to return 0
 *      values from 1.0 to 1.999... to return 1
 *      value  >=   2.0             will be limited to max value.
 *****************************************************************************/

uint32_t aclara_randu( uint32_t minVal, uint32_t maxVal )
{
   float    retVal;
   uint32_t randVal;

   randVal = (uint32_t)aclara_rand();        /* Get a random integer ( 0 - 2^32-1 */

   retVal = ((float)randVal)/RAND_MAX;       /* Convert random integer to floating point [0.0, 1.0] */
   retVal = retVal * ((maxVal+1) - minVal);  /* Scale to the range specified */
   randVal =  (uint32_t)retVal;              /* Convert uint32_t */
   randVal += minVal;                        /* Offset the result by the minimum of the range */

   if(randVal > maxVal)                      /* Limit to max value */
   {
      randVal = maxVal;
   }
   return randVal;
}

