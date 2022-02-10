/***********************************************************************************************************************
 * Copyright [2020-2021] Renesas Electronics Corporation and/or its affiliates.  All Rights Reserved.
 *
 * This software and documentation are supplied by Renesas Electronics America Inc. and may only be used with products
 * of Renesas Electronics Corp. and its affiliates ("Renesas").  No other uses are authorized.  Renesas products are
 * sold pursuant to Renesas terms and conditions of sale.  Purchasers are solely responsible for the selection and use
 * of Renesas products and Renesas assumes no liability.  No license, express or implied, to any intellectual property
 * right is granted by Renesas. This software is protected under all applicable laws, including copyright laws. Renesas
 * reserves the right to change or discontinue this software and/or this documentation. THE SOFTWARE AND DOCUMENTATION
 * IS DELIVERED TO YOU "AS IS," AND RENESAS MAKES NO REPRESENTATIONS OR WARRANTIES, AND TO THE FULLEST EXTENT
 * PERMISSIBLE UNDER APPLICABLE LAW, DISCLAIMS ALL WARRANTIES, WHETHER EXPLICITLY OR IMPLICITLY, INCLUDING WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT, WITH RESPECT TO THE SOFTWARE OR
 * DOCUMENTATION.  RENESAS SHALL HAVE NO LIABILITY ARISING OUT OF ANY SECURITY VULNERABILITY OR BREACH.  TO THE MAXIMUM
 * EXTENT PERMITTED BY LAW, IN NO EVENT WILL RENESAS BE LIABLE TO YOU IN CONNECTION WITH THE SOFTWARE OR DOCUMENTATION
 * (OR ANY PERSON OR ENTITY CLAIMING RIGHTS DERIVED FROM YOU) FOR ANY LOSS, DAMAGES, OR CLAIMS WHATSOEVER, INCLUDING,
 * WITHOUT LIMITATION, ANY DIRECT, CONSEQUENTIAL, SPECIAL, INDIRECT, PUNITIVE, OR INCIDENTAL DAMAGES; ANY LOST PROFITS,
 * OTHER ECONOMIC DAMAGE, PROPERTY DAMAGE, OR PERSONAL INJURY; AND EVEN IF RENESAS HAS BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH LOSS, DAMAGES, CLAIMS OR COSTS.
 **********************************************************************************************************************/

#include "blinky_thread.h"
#include "project.h"
#include <stdio.h>

extern bsp_leds_t g_bsp_leds;
#if 1
extern icu_instance_ctrl_t g_sw1_irq1_ctrl;
extern icu_instance_ctrl_t pf_meter_ctrl;
static uint8_t button_pressed = false;

SemaphoreHandle_t new_Sem;
#endif

/**********************************************************************************************************************
 * @brief       This function initializes and enables the ICU module for User Push Button Switch.
 * @param[IN]   None
 * @retval      FSP_SUCCESS                  Upon successful open and enable ICU module
 * @retval      Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
fsp_err_t user_switch_init(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Open external IRQ/ICU module */
    err = R_ICU_ExternalIrqOpen(&g_sw1_irq1_ctrl, &g_sw1_irq1_cfg);
    if(FSP_SUCCESS == err)
    {
    	/* Enable ICU module */
       printf("\nOpen IRQ");
    	err = R_ICU_ExternalIrqEnable(&g_sw1_irq1_ctrl);
      if(FSP_SUCCESS != err)
         printf("\nFailed:IRQEnable");
    }
    return err;
}

fsp_err_t pf_meter_isr_init(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Open external IRQ/ICU module */
    err = R_ICU_ExternalIrqOpen(&pf_meter_ctrl, &pf_meter_cfg);
    if(FSP_SUCCESS == err)
    {
    	/* Enable ICU module */
       printf("\nOpen PF Meter IRQ");
    	err = R_ICU_ExternalIrqEnable(&pf_meter_ctrl);
      if(FSP_SUCCESS != err)
         printf("\nFailed:IRQEnable");
    }
    return err;
}

/* Blinky Thread entry function */
void blinky_thread_entry (void * pvParameters)
{
    FSP_PARAMETER_NOT_USED(pvParameters);
#if 1
    fsp_err_t err = FSP_SUCCESS;

    vSemaphoreCreateBinary(new_Sem);
        /* Initialize External IRQ driver/user push-button. User Push buttons are used for user input event */
//    err = user_switch_init();
    /* Handle error */
    if(FSP_SUCCESS != err)
    {
       printf("\r\n User Push Button IRQ  Initialization Failed \r\n");
    }

    err = pf_meter_isr_init();
    /* Handle error */
    if(FSP_SUCCESS != err)
    {
       printf("\r\n PF METER IRQ  Initialization Failed \r\n");
    }
#endif
    /* LED type structure */
    bsp_leds_t leds = g_bsp_leds;

    /* If this board has no LEDs then trap here */
    if (0 == leds.led_count)
    {
        while (1)
        {
            ;                          // There are no LEDs on this board
        }
    }

    /* Holds level to set for pins */
    bsp_io_level_t pin_level = BSP_IO_LEVEL_LOW;
    if( pdPASS != xSemaphoreTake(new_Sem, portMAX_DELAY) )
    {
       printf("\n Failed to Take the Sem");
    }
    else
    {
       printf("\nSuccess");

    while (1)
    {
        /* Enable access to the PFS registers. If using r_ioport module then register protection is automatically
         * handled. This code uses BSP IO functions to show how it is used.
         */

        R_BSP_PinAccessEnable();

        /* Update all board LEDs */
        for (uint32_t i = 0; i < leds.led_count; i++)
        {
            /* Get pin to toggle */
            uint32_t pin = leds.p_leds[i];

            /* Write to this pin */
            R_BSP_PinWrite((bsp_io_port_pin_t) pin, pin_level);
        }

        /* Protect PFS registers */
        R_BSP_PinAccessDisable();

        /* Toggle level for next write */
        if (BSP_IO_LEVEL_LOW == pin_level)
        {
            pin_level = BSP_IO_LEVEL_HIGH;
        }
        else
        {
            pin_level = BSP_IO_LEVEL_LOW;
        }
        if(button_pressed)
        {
           printf("Button Pressed");
           button_pressed = false;
        }

        vTaskDelay(configTICK_RATE_HZ);
    }
    }
}

void sw1_irq_Handler(external_irq_callback_args_t * p_args)
{
   (void)p_args;
   button_pressed = true;
   xSemaphoreGiveFromISR(new_Sem, NULL);
}
void isr_brownOut(external_irq_callback_args_t * p_args)
{
   (void)p_args;
   button_pressed = true;
   printf("ISR_PF_METER");
   xSemaphoreGiveFromISR(new_Sem, NULL);

}
