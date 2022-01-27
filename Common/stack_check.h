/******************************************************************************
 *
 * Filename: stack_check.h
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

#ifndef _STACK_CHECK_H
#define _STACK_CHECK_H

#define STACK_WARNING   100  // Issue warning if there is less than xxx bytes left on stack

#define stack_check() stack_Check(STACK_WARNING, __func__, __LINE__)

void stack_check_init(_task_id taskID);
void stack_Check( uint32_t stackWarning, char const *func, unsigned int line );

#endif

