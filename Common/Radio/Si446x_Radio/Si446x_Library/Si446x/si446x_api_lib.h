/*!
 * File:
 *  si446x_api_lib.h
 *
 * Description:
 *  This file contains the Si446x API library.
 *
 * Silicon Laboratories Confidential
 * Copyright 2011 Silicon Laboratories, Inc.
 */

#ifndef _SI446X_API_LIB_H_
#define _SI446X_API_LIB_H_

#include "../compiler_types.h"
#include "si446x_cmd.h"

//extern SEGMENT_VARIABLE( Si446xCmd, union si446x_cmd_reply_union, SEG_XDATA );
//extern union si446x_cmd_reply_union Si446xCmd;
//extern SEGMENT_VARIABLE( Pro2Cmd[16], U8, SEG_XDATA );
//extern uint8_t Pro2Cmd[16];

#define SI466X_FIFO_SIZE 64

#define RADIO_DRIVER_EXTENDED_SUPPORT 1 // MKD
#define RADIO_DRIVER_FULL_SUPPORT 1 // MKD

enum
{
    SI446X_SUCCESS,
    SI446X_NO_PATCH,
    SI446X_CTS_TIMEOUT,
    SI446X_PATCH_FAIL,
    SI446X_COMMAND_ERROR
};

/* Minimal driver support functions */
void si446x_reset(void);
void si446x_power_up(uint8_t radioNum, U8 BOOT_OPTIONS, U8 XTAL_OPTIONS, U32 XO_FREQ);

U8 si446x_configuration_init(uint8_t radioNum, const U8* pSetPropCmd, U16 partInfo);
U8 si446x_apply_patch(void);
U8 si446x_part_info(uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd);

U8 si446x_start_tx(uint8_t radioNum, U8 CHANNEL, U8 CONDITION, U16 TX_LEN, uint32_t txtime);
U8 si446x_start_rx(uint8_t radioNum, U8 CHANNEL, U8 CONDITION, U16 RX_LEN, U8 NEXT_STATE1, U8 NEXT_STATE2, U8 NEXT_STATE3);

U8 si446x_get_int_status(uint8_t radioNum, U8 PH_CLR_PEND, U8 MODEM_CLR_PEND, U8 CHIP_CLR_PEND, union si446x_cmd_reply_union *Si446xCmd);
U8 si446x_gpio_pin_cfg(uint8_t radioNum, U8 GPIO0, U8 GPIO1, U8 GPIO2, U8 GPIO3, U8 NIRQ, U8 SDO, U8 GEN_CONFIG, union si446x_cmd_reply_union *Si446xCmd);

U8 si446x_set_property(uint8_t radioNum, U8 GROUP, U8 NUM_PROPS, U8 START_PROP, ... );
U8 si446x_change_state(uint8_t radioNum, U8 NEXT_STATE1);

#ifdef RADIO_DRIVER_EXTENDED_SUPPORT
  /* Extended driver support functions */
  U8   si446x_nop(uint8_t radioNum);

  U8   si446x_fifo_info(uint8_t radioNum, U8 FIFO, union si446x_cmd_reply_union *Si446xCmd);
  void si446x_write_tx_fifo( uint8_t radioNum, U8 numBytes, U8* pData );
  void si446x_read_rx_fifo( uint8_t radioNum, U8 numBytes, U8* pRxData );

  U8   si446x_get_property(uint8_t radioNum, U8 GROUP, U8 NUM_PROPS, U8 START_PROP, union si446x_cmd_reply_union *Si446xCmd);

  #ifdef RADIO_DRIVER_FULL_SUPPORT
    /* Full driver support functions */
    U8   si446x_func_info(uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd);

    void si446x_frr_a_read(uint8_t radioNum, U8 respByteCount, union si446x_cmd_reply_union *Si446xCmd);
    void si446x_frr_b_read(uint8_t radioNum, U8 respByteCount, union si446x_cmd_reply_union *Si446xCmd);
    void si446x_frr_c_read(uint8_t radioNum, U8 respByteCount, union si446x_cmd_reply_union *Si446xCmd);
    void si446x_frr_d_read(uint8_t radioNum, U8 respByteCount, union si446x_cmd_reply_union *Si446xCmd);

    U8   si446x_get_adc_reading(uint8_t radioNum, U8 ADC_EN, U8 UDTIME, union si446x_cmd_reply_union *Si446xCmd);
    U8   si446x_get_packet_info(uint8_t radioNum, U8 FIELD_NUMBER_MASK, U16 LEN, S16 DIFF_LEN, union si446x_cmd_reply_union *Si446xCmd );
    U8   si446x_get_ph_status(uint8_t radioNum, U8 PH_CLR_PEND, union si446x_cmd_reply_union *Si446xCmd);
    U8   si446x_get_modem_status(uint8_t radioNum, U8 MODEM_CLR_PEND, union si446x_cmd_reply_union *Si446xCmd);
    U8   si446x_get_chip_status(uint8_t radioNum, U8 CHIP_CLR_PEND, union si446x_cmd_reply_union *Si446xCmd );

    U8   si446x_ircal(uint8_t radioNum, U8 SEARCHING_STEP_SIZE, U8 SEARCHING_RSSI_AVG, U8 RX_CHAIN_SETTING1, U8 RX_CHAIN_SETTING2);
    U8   si446x_ircal_manual(uint8_t radioNum, U8 IRCAL_AMP, U8 IRCAL_PH, union si446x_cmd_reply_union *Si446xCmd);
    void si446x_protocol_cfg(U8 PROTOCOL);

    U8   si446x_request_device_state(uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd);

    U8   si446x_tx_hop(uint8_t radioNum, U8 INTE, U8 FRAC2, U8 FRAC1, U8 FRAC0, U8 VCO_CNT1, U8 VCO_CNT0, U8 PLL_SETTLE_TIME1, U8 PLL_SETTLE_TIME0);
    U8   si446x_rx_hop(uint8_t radioNum, U8 INTE, U8 FRAC2, U8 FRAC1, U8 FRAC0, U8 VCO_CNT1, U8 VCO_CNT0);

    U8   si446x_start_tx_fast( uint8_t radioNum );
    U8   si446x_start_rx_fast( uint8_t radioNum );

    U8   si446x_get_int_status_fast_clear( uint8_t radioNum );
    U8   si446x_get_int_status_fast_clear_read( uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd );

    U8   si446x_gpio_pin_cfg_fast( uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd );

    U8   si446x_get_ph_status_fast_clear( uint8_t radioNum );
    U8   si446x_get_ph_status_fast_clear_read( uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd );

    U8   si446x_get_modem_status_fast_clear( uint8_t radioNum );
    U8   si446x_get_modem_status_fast_clear_read( uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd );

    U8   si446x_get_chip_status_fast_clear( uint8_t radioNum );
    U8   si446x_get_chip_status_fast_clear_read( uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd );

    U8   si446x_fifo_info_fast_reset(uint8_t radioNum, U8 FIFO);
    U8   si446x_fifo_info_fast_read(uint8_t radioNum, union si446x_cmd_reply_union *Si446xCmd);

  #endif
#endif


#endif //_SI446X_API_LIB_H_
