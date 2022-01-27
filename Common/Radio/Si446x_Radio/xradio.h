/*! @file radio.h
 * @brief This file is contains the public radio interface functions.
 *
 * @b COPYRIGHT
 * @n Silicon Laboratories Confidential
 * @n Copyright 2012 Silicon Laboratories, Inc.
 * @n http://www.silabs.com
 */

#ifndef XRADIO_H_
#define XRADIO_H_

#include <stdint.h>
#include <stdbool.h>

/*****************************************************************************
 *  Global Macros & Definitions
 *****************************************************************************/

/*****************************************************************************
 *  Global Typedefs & Enums
 *****************************************************************************/
typedef struct
{
    const uint8_t   *Radio_ConfigurationArray;
          uint8_t   Radio_ChannelNumber;
          uint8_t   Radio_PacketLength;
          uint8_t   Radio_State_After_Power_Up;
          uint16_t  Radio_Delay_Cnt_After_Reset;
} tRadioConfiguration;

/*****************************************************************************
 *  Global Variable Declarations
 *****************************************************************************/
extern const tRadioConfiguration RadioConfiguration_normal;
extern const tRadioConfiguration RadioConfiguration_STAR_2400;
extern const tRadioConfiguration RadioConfiguration_STAR_7200;
extern const tRadioConfiguration RadioConfiguration_normal_dev_700_IF_6_69_TCXO_29_491;
extern const tRadioConfiguration RadioConfiguration_normal_dev_700_IF_7_43_TCXO_29_491;
extern const tRadioConfiguration RadioConfiguration_normal_dev_800_IF_7_43_TCXO_29_491;
extern const tRadioConfiguration RadioConfiguration_normal_dev_800_IF_8_23_TCXO_29_491;
extern const tRadioConfiguration RadioConfiguration_normal_dev_600;
//extern const tRadioConfiguration RadioConfiguration_STAR_2400;
//extern const tRadioConfiguration RadioConfiguration_STAR_7200;
//extern const tRadioConfiguration RadioConfiguration_PN9;
//extern const tRadioConfiguration RadioConfiguration_cw;
//extern const tRadioConfiguration RadioConfiguration_2GFSK_BER;
//extern const tRadioConfiguration RadioConfiguration_4GFSK_BER;

/*****************************************************************************
 *  Global Function Declarations
 *****************************************************************************/
#ifdef RADIO_H_
void    vRadio_Init(RADIO_MODE_t mode);
#endif
void    RadioEvent_Int(uint8_t radioNum);
void    RadioEvent_Int(uint8_t radioNum);
void    vRadio_StartRX(uint8_t radioNum, uint16_t channel);
void    SetFreq(uint8_t radioNum, uint32_t freq);

#endif /* XRADIO_H_ */
