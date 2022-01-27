/***********************************************************************************************************************

   Filename:   gpio.c

   Global Designator: GPIO_

   Contents:

 ***********************************************************************************************************************
   A product of Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2017 - 2021 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
   used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/

/* INCLUDE FILES */
#include "project.h"
#include "gpio.h"
#include "DBG_SerialDebug.h"

/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

/* ****************************************************************************************************************** */
/* CONSTANTS */

/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

/* ****************************************************************************************************************** */
/* LOCAL FUNCTION PROTOTYPES */

static void portA_int_handler(void *pin);
static void portB_int_handler(void *pin);
static void portC_int_handler(void *pin);
static void portD_int_handler(void *pin);
static void portE_int_handler(void *pin);
static void (*isrCallback[5][32])(void);
static void register_gpio_file(uint32_t port, uint32_t pin, LWGPIO_STRUCT *gpio, void (*callback)(void));

/* ****************************************************************************************************************** */
/* GLOBAL FUNCTION DEFINITIONS */

/******************************************************************************

   Function Name: register_gpio_file

   Purpose: This function will map a user initialized gpio to its proper interrupt handler.

   Arguments:  port - GPIO_PORT_A - GPIO_PORT_E
               pin  - GPIO_PIN1 - GPIO_PIN31
               gpio - Pin settings
               callback - Function to call when an interrupt is detected

   Returns:    none

   Notes:

******************************************************************************/
static void register_gpio_file(uint32_t port, uint32_t pin, LWGPIO_STRUCT *gpio, void (*callback)(void))
{
   switch (port) {
      case GPIO_PORT_A:
         isrCallback[0][pin] = callback; // Save callback pointer
         (void)_int_install_isr(lwgpio_int_get_vector(gpio), portA_int_handler , gpio); // Set the correct interrupt routine for the gpio
         break;

      case GPIO_PORT_B:
         isrCallback[1U][pin] = callback; // Save callback pointer
         (void)_int_install_isr(lwgpio_int_get_vector(gpio), portB_int_handler , gpio); // Set the correct interrupt routine for the gpio
         break;

      case GPIO_PORT_C:
         isrCallback[2U][pin] = callback; // Save callback pointer
         (void)_int_install_isr(lwgpio_int_get_vector(gpio), portC_int_handler , gpio); // Set the correct interrupt routine for the gpio
         break;

      case GPIO_PORT_D:
         isrCallback[3U][pin] = callback; // Save callback pointer
         (void)_int_install_isr(lwgpio_int_get_vector(gpio), portD_int_handler , gpio); // Set the correct interrupt routine for the gpio
         break;

      case GPIO_PORT_E:
         isrCallback[4U][pin] = callback; // Save callback pointer
         (void)_int_install_isr(lwgpio_int_get_vector(gpio), portE_int_handler , gpio); // Set the correct interrupt routine for the gpio
         break;

      default:
         break;
   }
}

/******************************************************************************

   Function Name: int_handler

   Purpose: Process all ports interrupts

   Arguments:  portBasePtr - Port base address for port A to port E
               portNum     - port number 0-4

   Returns:    none

   Notes:

******************************************************************************/
static void int_handler(PORT_MemMapPtr portBasePtr, uint32_t portNum )
{
   uint32_t i; // Loop counter

   if ( portNum > 4U )
   {
      DBG_LW_printf( "\nInvalid port number %u received in GPIO handler!\nLooping until watch dog kicks in.\n", portNum );
      for (;;){}
   }

   for (i=0; i<32U; i++) {  // There are 32 pins max, 0-31
      if ( PORT_PCR_REG(portBasePtr, i) & PORT_PCR_ISF_MASK ) {  // Determine which gpio line was interrupted
         // Only process pins that are configured to generate an interrupt.
         // Pins that are configured for DMA will be processed/acknowlegded by the DMA engine.
         if ( PORT_PCR_REG(portBasePtr, i) & 0x00080000u ) {
            // Make sure a callback is defined.
            if ( isrCallback[portNum][i] == NULL )
            {
               DBG_LW_printf( "\nCallback for GPIO port %u, pin %u is NULL\n", portNum, i );
            }
            else
            {
               (*isrCallback[portNum][i])();                        // Call callback function for this pin
            }
            PORT_PCR_REG(portBasePtr, i) |= PORT_PCR_ISF_MASK;      // Clear interrupt
         }
      }
   }
}

/******************************************************************************

   Function Name: portA_int_handler

   Purpose: Process port A interrupts

   Arguments:  pin - ignored

   Returns:    one

   Notes:

******************************************************************************/
static void portA_int_handler(void *pin)
{
   (void)pin;
   int_handler(PORTA_BASE_PTR, 0); // Handle port A interrupts
}

/******************************************************************************

   Function Name: portB_int_handler

   Purpose: Process port B interrupts

   Arguments:  pin - ignored

   Returns:    none

   Notes:

******************************************************************************/
static void portB_int_handler(void *pin)
{
   (void)pin;
   int_handler(PORTB_BASE_PTR, 1U); // Handle port B interrupts
}

/******************************************************************************

   Function Name: portC_int_handler

   Purpose: Process port C interrupts

   Arguments:  pin - ignored

   Returns:    none

   Notes:

******************************************************************************/
static void portC_int_handler(void *pin)
{
   (void)pin;
   int_handler(PORTC_BASE_PTR, 2U); // Handle port C interrupts
}

/******************************************************************************

   Function Name: portD_int_handler

   Purpose: Process port D interrupts

   Arguments:  pin - ignored

   Returns:    none

   Notes:

******************************************************************************/
static void portD_int_handler(void *pin)
{
   (void)pin;
   int_handler(PORTD_BASE_PTR, 3U); // Handle port D interrupts
}

/******************************************************************************

   Function Name: portE_int_handler

   Purpose: Process port E interrupts

   Arguments:  pin - ignored

   Returns:    none

   Notes:

******************************************************************************/
static void portE_int_handler(void *pin)
{
   (void)pin;
   int_handler(PORTE_BASE_PTR, 4U); // Handle port E interrupts
}

/******************************************************************************

   Function Name: GPIO_init

   Purpose: Initializes a gpio port and pins

   Arguments:  settings - address of structure containing settings for gpio initialization
               gpio     - GPIO struct

   Returns:    none

   Notes:

******************************************************************************/
void GPIO_init( const GPIO_INIT_t *settings, LWGPIO_STRUCT *gpio )
{
   if(settings->lwgpio_dir == LWGPIO_DIR_INPUT) /*0 is input*/
   {
      (void)lwgpio_init(gpio, (settings->port | settings->pin), LWGPIO_DIR_INPUT, LWGPIO_VALUE_NOCHANGE);  /*this will initialize the gpio, and place the information for it in temp_gpio*/
      lwgpio_set_functionality(gpio, 1U);   /*sets the gpio mux to gpio setting*/
      (void)lwgpio_set_attribute(gpio, settings->pu_pd_od, LWGPIO_AVAL_ENABLE);  /* pull up, pull down, or open drain */

      if ( settings->isInterrupt )  /* these are interrupt GPIO settigns */
      {
         (void)lwgpio_int_init(gpio, settings->interrupt_type ); /* set the type of interrupt */

         register_gpio_file(settings->port, settings->pin, gpio, settings->callback);  /* this will map the gpio file to the proper interrupt routine handler, and initialize the event for the
                                                                                          IRQ */

         (void)_bsp_int_init(lwgpio_int_get_vector(gpio), settings->priority_level, 0, TRUE);  /* initialize the interrupt and set its priority.  */
         lwgpio_int_clear_flag(gpio); /* clear pending interrupt */
         lwgpio_int_enable(gpio, TRUE);  /* enable the interrupt for use*/
      }
   }
   else /*gpio.mode == output*/
   {
      (void)lwgpio_init(gpio, (settings->port | settings->pin), LWGPIO_DIR_OUTPUT, LWGPIO_VALUE_NOCHANGE);
      lwgpio_set_functionality(gpio, 1U);    /* sets the gpio mux to gpio setting */
      lwgpio_set_value(gpio, settings->lwgpio_value); /* set initial pin state */
   }
}
