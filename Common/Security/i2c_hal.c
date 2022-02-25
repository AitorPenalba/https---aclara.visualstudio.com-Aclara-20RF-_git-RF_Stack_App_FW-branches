#ifndef I2C_HAL_H_
#define I2C_HAL_H_

#include "ecc108_physical.h"
#include "i2c_hal.h"

#include "security/ecc108_lib_return_codes.h"
#include "security/ecc108_comm_marshaling.h"

uint8_t sendVal[255]; // Considering the max value of 255 bytes
uint8_t receiveVal[75];

volatile i2c_master_event_t i2c_event = I2C_MASTER_EVENT_ABORTED;

void iic_eventCallback(i2c_master_callback_args_t * p_args)
{
    if (NULL != p_args)
    {
        /* capture callback event for validating the i2c transfer event*/
        i2c_event = p_args->event;
    }
}

void i2c_tranmsitCheck(void)
{
   uint8_t err;
   uint8_t sendVal = 3;
   while(1){
          uint8_t timeout_ms = 25;
   err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, &sendVal, 1, false);
    while ((I2C_MASTER_EVENT_TX_COMPLETE != i2c_event) && timeout_ms)
    {
        R_BSP_SoftwareDelay(500U, BSP_DELAY_UNITS_MICROSECONDS);
        timeout_ms--;;
    }
    R_BSP_SoftwareDelay(10000U, BSP_DELAY_UNITS_MICROSECONDS);
   }

}
uint8_t i2c_send(uint8_t word_address, uint8_t count, uint8_t *buffer)
{
    fsp_err_t err = FSP_SUCCESS;
    uint8_t timeout_ms = 25;
    memset(sendVal, 0, sizeof(sendVal));
    uint8_t sendCount = count + 1;
    sendVal[0] = word_address;
    for(uint8_t i = 1; i < sendCount; i++ )
    {
        sendVal[i] = buffer[i-1];
    }

//    while(1) {
    timeout_ms = 25;
    err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, sendVal, sendCount, true);
    while ((I2C_MASTER_EVENT_TX_COMPLETE != i2c_event) && timeout_ms)
    {
        R_BSP_SoftwareDelay(500U, BSP_DELAY_UNITS_MICROSECONDS);
        timeout_ms--;;
    }
//    }
    return (uint8_t) err;
}

uint8_t i2c_receive_response(uint8_t size, uint8_t *response)
{
    fsp_err_t err = FSP_SUCCESS;
    uint8_t count;
    uint8_t timeout_ms = 50;
//    memset(receiveVal, 0, sizeof(receiveVal));
    err = R_IIC_MASTER_Read(&g_i2c_master0_ctrl, response, size, false);
    if (err == FSP_SUCCESS)
    {
        while ((I2C_MASTER_EVENT_RX_COMPLETE != i2c_event) && timeout_ms)
        {
            R_BSP_SoftwareDelay(500U, BSP_DELAY_UNITS_MICROSECONDS);
            timeout_ms--;;
        }

        if(I2C_MASTER_EVENT_RX_COMPLETE == i2c_event)
        {
//            count = receiveVal[ECC108_BUFFER_POS_COUNT];
//            memcpy(response, receiveVal, count);
        }
        else
        {
            err = (uint8_t) ECC108_RX_NO_RESPONSE;
        }

    }

    return (uint8_t) err;
}

uint8_t i2c_resync(uint8_t size, uint8_t *response)
{
    uint8_t ret_code;
    ret_code = ecc108p_wakeup();
    return ret_code;
}

void i2c_wakeUp(void)
{
    R_BSP_PinCfg( BSP_IO_PORT_04_PIN_01, ((uint32_t) IOPORT_CFG_DRIVE_HIGH
            | (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t) IOPORT_CFG_PORT_OUTPUT_LOW) );
    R_BSP_PinAccessEnable();

    R_BSP_PinWrite ( BSP_IO_PORT_04_PIN_01, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(70U, BSP_DELAY_UNITS_MICROSECONDS);
    R_BSP_PinWrite ( BSP_IO_PORT_04_PIN_01, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(1000U, BSP_DELAY_UNITS_MICROSECONDS);

    R_BSP_PinCfg( BSP_IO_PORT_04_PIN_01, ((uint32_t) IOPORT_CFG_DRIVE_MID | (uint32_t) IOPORT_CFG_PERIPHERAL_PIN
            | (uint32_t) IOPORT_CFG_PULLUP_ENABLE | (uint32_t) IOPORT_PERIPHERAL_IIC) );
}

/***********************************************************************************************************************
   Function Name: ecc108_open

   Purpose: Open the i2c port

   Arguments: none

   Returns: Status of the open
***********************************************************************************************************************/
//uint8_t ecc108_open(void)
//{
////   uint8_t wakeup_response[ECC108_RSP_SIZE_MIN];
//   fsp_err_t err = R_IIC_MASTER_Open(&g_i2c_master0_ctrl, &g_i2c_master0_cfg);
//   if (FSP_SUCCESS == err)
//   {
////       (void)ecc108c_wakeup( wakeup_response );   // This wakeup call here results in not communicating with the security chip as there will be another wakeup at every APIs
//   }
//
//   return (uint8_t)err;
//}

/***********************************************************************************************************************
   Function Name: ecc108_close

   Purpose: Close the i2c port

   Arguments: none

   Returns: none
***********************************************************************************************************************/
//void ecc108_close(void)
//{
//    (void) ecc108p_sleep();
//    (void) R_IIC_MASTER_Close(&g_i2c_master0_ctrl);
//}

#endif
