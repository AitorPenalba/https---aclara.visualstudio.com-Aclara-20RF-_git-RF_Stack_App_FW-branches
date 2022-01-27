/******************************************************************************
 *
 * Filename: aclara_rand.h
 *
 * Global Designator:
 *
 * Contents:
 *
 ******************************************************************************
 * Copyright (c) 2015 ACLARA.  All rights reserved.
 * This program may not be reproduced, in whole or in part, in any form or by
 * any means whatsoever without the written permission of:
 *    ACLARA, ST. LOUIS, MISSOURI USA
 *****************************************************************************/

#ifndef _RAND_H
#define _RAND_H

#ifdef RAND_MAX
#undef RAND_MAX
#endif
#define RAND_MAX 0xFFFFFFFF

void     aclara_srand(uint32_t seed);                       /* Seed the random number generator */
int32_t  aclara_rand(void);                                 /* uint32_t from 0 to RAND_MAX      */
float    aclara_randf( float minVal, float maxVal );        /* float from minVal to maxVal      */
uint32_t aclara_randu( uint32_t minVal, uint32_t maxVal );  /* uint32_t from minVal to maxVal   */

#endif

