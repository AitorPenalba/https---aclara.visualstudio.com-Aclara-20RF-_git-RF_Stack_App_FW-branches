/***********************************************************************************************************************
 *
 * Filename: gpio.h
 *
 * Contents:
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2010-2017 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/
#ifndef GPIO_H
#define GPIO_H

/* ****************************************************************************************************************** */
/* INCLUDE FILES */

#include <stdlib.h>
#include <mqx.h>
#include <bsp.h>

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef struct{
   uint32_t     port;                 /* GPIO_PORT_A   GPIO_PORT_B   GPIO_PORT_C  GPIO_PORT_D  GPIO_PORT_E  GPIO_PORT_F */
   uint32_t     pin;                  /* GPIO_PIN1   GPIO_PIN2 ....  GPIO_PIN31 */
   LWGPIO_DIR   lwgpio_dir;           /* LWGPIO_DIR_INPUT  LWGPIO_DIR_OUTPUT */
   LWGPIO_VALUE lwgpio_value;         /* Initial output value LWGPIO_VALUE_LOW  LWGPIO_VALUE_HIGH */
   uint8_t      pu_pd_od;             /* LWGPIO_ATTR_PULL_UP  LWGPIO_ATTR_PULL_DOWN  LWGPIO_ATTR_OPEN_DRAIN */
   bool         isInterrupt;          /* true if this pin is an interrupt */
   uint8_t      interrupt_type;       /* LWGPIO_INT_MODE_NONE LWGPIO_INT_MODE_RISING LWGPIO_INT_MODE_FALLING LWGPIO_INT_MODE_HIGH LWGPIO_INT_MODE_LOW */
   uint8_t      priority_level;       /*  0-15? */
   void         (*callback)(void);    /* Interrupt function to be called */
} GPIO_INIT_t;

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* GLOBAL VARIABLES */

/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

void GPIO_init( const GPIO_INIT_t *settings, LWGPIO_STRUCT *gpio );
#endif
