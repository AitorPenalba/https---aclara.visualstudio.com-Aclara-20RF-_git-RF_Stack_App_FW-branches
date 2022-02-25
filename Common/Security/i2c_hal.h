#ifndef IIC_HAL_H_
#define IIC_HAL_H_

#include "hal_data.h"

//extern i2c_master_event_t i2c_event;

uint8_t   i2c_send(uint8_t word_address, uint8_t count, uint8_t *buffer);
uint8_t   i2c_receive_response(uint8_t size, uint8_t *response);
uint8_t   i2c_resync(uint8_t size, uint8_t *response);
void      i2c_wakeUp(void);

//uint8_t   ecc108_open(void);
//void      ecc108_close(void);
void i2c_tranmsitCheck(void);
#endif
