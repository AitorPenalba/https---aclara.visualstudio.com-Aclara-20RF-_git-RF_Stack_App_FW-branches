
/***********************************************************************************************************************

   Filename: DBG_CommandLine.h

   Global Designator:

   Contents:

 ***********************************************************************************************************************
   A product of
   Aclara Technologies LLC
   Confidential and Proprietary
   Copyright 2012-2021 Aclara.  All Rights Reserved.

   PROPRIETARY NOTICE
   The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
   (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
   authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may
   be used or copied only in accordance with the terms of such license.
 **********************************************************************************************************************/
#ifndef DBG_CommandLine_H
#define DBG_CommandLine_H

/* INCLUDE FILES */

/* #DEFINE DEFINITIONS */

/* MACRO DEFINITIONS */

/* TYPE DEFINITIONS */

/* CONSTANTS */

/* FILE VARIABLE DEFINITIONS */

/* FUNCTION PROTOTYPES */

void DBG_CommandLineTask ( taskParameter );
uint32_t DBG_CommandLine_Help ( uint32_t argc, char *argv[] );
#if ( TM_CRC_UNIT_TEST == 1 )
uint32_t DBG_CommandLine_CrcCalculate( uint32_t argc, char *argv[] );
#endif

#if ( TM_TIME_COMPOUND_TEST == 1 )
uint32_t DBG_CommandLine_TimeElapsed( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_TimeNanoSec( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_TimeMicroSec( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_TimeMilliSec( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_TimeSec( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_TimeMin( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_TimeHour( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_TimeTicks( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_TimeFuture( uint32_t argc, char *argv[] );
#endif

#if ( TM_OS_EVENT_TEST == 1)
uint32_t DBG_CommandLine_wdTest ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_OS_EventSet( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_OS_EventCreateWait( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_OS_EventTaskDelete( uint32_t argc, char *argv[] );
#endif

#if ( TM_INTERNAL_FLASH_TEST == 1)
uint32_t DBG_CommandLine_IntFlash_OpenPartition( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_IntFlash_ReadPartition( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_IntFlash_WritePartition( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_IntFlash_ErasePartition( uint32_t argc, char *argv[] );
#if ( MCU_SELECTED == RA6E1 )
uint32_t DBG_CommandLine_IntFlash_BlankCheckPartition( uint32_t argc, char *argv[] );
#endif
uint32_t DBG_CommandLine_IntFlash_ClosePartition( uint32_t argc, char *argv[] );
#endif

#ifdef TM_BL_TEST_COMMANDS
uint32_t DBG_CommandLine_BL_Test_Write_DFW_Image( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_BL_Test_Write_BL_Info( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_BL_Test_Clear_BL_Info( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_BL_Test_Erase_BL_Info( uint32_t argc, char *argv[] );
#endif

#if ( TM_LINKED_LIST == 1)
uint32_t DBG_CommandLine_OS_LinkedList_Create( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_OS_LinkedList_Remove( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_OS_LinkedList_Enqueue( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_OS_LinkedList_Dequeue( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_OS_LinkedList_Insert( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_OS_LinkedList_Next( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_OS_LinkedList_Head( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_OS_LinkedList_AddElements( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_OS_LinkedList_NumElements( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_OS_LinkedList_Dump( uint32_t argc, char *argv[] );
#endif
#if ( TM_RTC_UNIT_TEST == 1 )
uint32_t DBG_CommandLine_RTC_UnitTest( uint32_t argc, char *argv[] );
#endif
#if ( DAC_CODE_CONFIG == 1 )
uint32_t DBG_CommandLine_DAC_SetDacStep ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_setPwrSel ( uint32_t argc, char *argv[] );

uint32_t DBG_CommandLine_DAC_Code ( uint32_t argc, char *argv[] );
#endif
#ifdef CompileSwitch_H
#define ENABLE_DAC_TEST_FUNCTIONS 0
#if ENABLE_DAC_TEST_FUNCTIONS
uint32_t DBG_CommandLine_DAC ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_DAC_Ampl ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_DAC_Cycl ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_DAC_Freq ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_AmpEnable ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_RelayEnable ( uint32_t argc, char *argv[] );
#endif /* ENABLE_DAC_TEST_FUNCTIONS */
#endif /* CompileSwitch_H */
uint32_t DBG_CommandLine_Buffers( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_PrintFiles  ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_DumpFiles  ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_ManualTemperature ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_GetHWInfo ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_HmcCmd ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_HmcCurrent ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_HmcHist ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_HmcDemand ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_HmcEng ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_HmcDemandCoin ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_DsTimeDv( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_IdCfg ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_IdCfgR ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_IdRd ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_IdTimeDv( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_IdParams ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_NvRead ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_Partition ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_clocktst( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_time ( uint32_t argc, char *argv[] );
#if ( EP == 1 ) && ( TEST_TDMA == 1 )
uint32_t DBG_CommandLine_Delay ( uint32_t argc, char *argv[] );
#endif
uint32_t DBG_CommandLine_rtcTime ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_rtcCap ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_LED ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_getDstHash ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_setTimezoneOffset ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_getTimezoneOffset ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_setDstEnable ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_getDstEnable ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_setDstOffset ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_getDstOffset ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_setDstStartRule ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_getDstStartRule ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_setDstStopRule ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_getDstStopRule ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_getDstParams ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_virgin ( uint32_t argc, char *argv[] );
#if (EP == 1) && ( ACLARA_LC == 0 ) && ( ACLARA_DA == 0 )
uint32_t DBG_CommandLine_OnDemandRead ( uint32_t argc, char *argv[] );
#endif
#if (MCU_SELECTED == RA6E1) && (TM_VERIFY_BOOT_UPDATE_RA6E1 == 1)
uint32_t DBG_CommandLine_BootUpdateVerify ( uint32_t argc, char *argv[] );
#endif
uint32_t DBG_CommandLine_Versions ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_Counters ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_DFW_Status ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_DfwTimeDv ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_MacAddr( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_TimeSync( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_MacPingCmd(uint32_t argc, char *argv[]);

uint32_t DBG_CommandLine_SmStart( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_SmStop( uint32_t argc, char *argv[] );
#if ( BOB_IS_LAZY == 1 )
uint32_t DBG_ResetAllStats ( uint32_t argc, char *argv[] );
uint32_t DBG_ShowAllStats  ( uint32_t argc, char *argv[] );
#endif
uint32_t DBG_CommandLine_NwkGet(   uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_NwkReset( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_NwkSet(   uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_NwkStart( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_NwkStop(  uint32_t argc, char *argv[] );

uint32_t DBG_CommandLine_MacReset( uint32_t argc, char *argv[] );
#if 0
uint32_t DBG_CommandLine_MacFlush( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_MacGet( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_MacPurge( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_MacSet( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_MacStart( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_MacStop( uint32_t argc, char *argv[] );
#endif
uint32_t DBG_CommandLine_MacTime( uint32_t argc, char *argv[] );

uint32_t DBG_CommandLine_PhyStart( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_PhyStop(  uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_PhyReset( uint32_t argc, char *argv[] );

uint32_t DBG_CommandLine_MacTxPause ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_MacTxUnPause ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_MacTimeCmd ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_MacStats ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_MacConfig ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_PhyStats ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_PhyConfig ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_NwkStats ( uint32_t argc, char *argv[] );
#if (USE_DTLS == 1)
uint32_t DBG_CommandLine_DtlsStats ( uint32_t argc, char *argv[] );
#endif


uint32_t DBG_CommandLine_PWR_BoostTest ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_PWR_SuperCap ( uint32_t argc, char *argv[] );

#ifdef CompileSwitch_H
uint32_t DBG_CommandLine_AfcEnable ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_AfcRSSIThreshold ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_AfcTemperatureRange ( uint32_t argc, char *argv[] );

uint32_t DBG_CommandLine_SyncStats ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_Thres ( uint32_t argc, char *argv[] );

uint32_t DBG_CommandLine_CpuLoadEnable( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_CpuLoadSmart( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_CpuLoadDisable( uint32_t argc, char *argv[] );

uint32_t DBG_CommandLine_SendAppMsg( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_SendHeadEndMsg ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_InsertAppMsg ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_InsertMacMsg ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_RegTimeouts ( uint32_t argc, char *argv[] );
#if ( EP == 1 )
uint32_t DBG_CommandLine_RegState ( uint32_t argc, char *argv[] );
#endif
uint8_t  DBG_CommandLine_chksum( char *msg );
uint32_t DBG_CommandLine_tunnel ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_crc( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_SetFreq( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_GetFreq( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_TxChannels( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_RxChannels( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_RxDetection ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_RxFraming ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_RxMode ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_StackUsage ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_TaskSummary ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_TXMode ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_Power ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_RadioStatus ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_DMDDump ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_SchDmdRst ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_DMDTO ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_Buffers( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_FreeRAM ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_Reboot ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_RSSI ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_RSSIJumpThreshold ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_CCA ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_CCAAdaptiveThreshold ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_CCAAdaptiveThresholdEnable ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_CCAOffset ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_FrontEndGain ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_NoiseEstimate ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_NoiseEstimateCount ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_NoiseEstimateRate ( uint32_t argc, char *argv[] );
#endif // TODO: RA6E1 Bob: enable this command even without compile_switch.h
uint32_t DBG_CommandLine_NoiseBand ( uint32_t argc, char *argv[] );
#if ( NOISE_HIST_ENABLED == 1 )
uint32_t DBG_CommandLine_NoiseHistogram ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_NoiseBurst     ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_BurstHistogram ( uint32_t argc, char *argv[] );
#endif
uint32_t DBG_CommandLine_ds( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_DebugDisable ( uint32_t argc, char *argv[] );
#ifdef CompileSwitch_H
#if 0
uint32_t DBG_CommandLine_FlashSecurity(uint32_t argc, char *argv[]);
uint32_t DBG_CommandLine_DebugEnable ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_GetPhyMaxTxPayload( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_SendMetadata( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_crc16 ( uint32_t argc, char *argv[] );
#endif

uint32_t DBG_CommandLine_NvTest ( uint32_t argc, char *argv[] );
#if ( SIMULATE_POWER_DOWN == 1 )
uint32_t DBG_CommandLine_NvEraseTest ( uint8_t sectors, bool nvLocation );
uint32_t DBG_CommandLine_SimulatePowerDown ( uint32_t argc, char *argv[] );
OS_TICK_Struct DBG_CommandLine_GetStartTick( void );
bool     DBG_CommandLine_isPowerDownSimulationFeatureEnabled(void);
void     DBG_CommandLine_isPowerDownSimulationFeatureDisable(void);
#endif
uint32_t DBG_CommandLine_sectest ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_secConfig ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_NetworkId ( uint32_t argc, char *argv[] );
#if ( FAKE_TRAFFIC == 1 )
uint32_t DBG_CommandLine_ipRfDutyCycle ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_ipBhaulGenCount ( uint32_t argc, char *argv[] );
#endif
uint32_t DBG_CommandLine_RxTimeout( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_RelHighCnt( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_RelMedCnt( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_RelLowCnt( uint32_t argc, char *argv[] );
#if ( DCU == 1 )
uint32_t DBG_CommandLine_TxPacketDelay( uint32_t argc, char *argv[] );
#if( HAL_TARGET_HARDWARE == HAL_TARGET_XCVR_9985_REV_A )
uint32_t DBG_CommandLine_PaCoeff( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_PaDAC( uint32_t argc, char *argv[] );
#if ( ENGINEERING_BUILD == 1 )
uint32_t DBG_CommandLine_PaCoeff( uint32_t argc, char *argv[] );
#endif   //( ENGINEERING_BUILD == 1 )
uint32_t DBG_CommandLine_VSWR( uint32_t argc, char *argv[] );
#endif //9985T
#endif //DCU
uint32_t DBG_CommandLine_PacketTimeout( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_EVLADD( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_EVLQ( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_EVLCMD( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_EVLSEND( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_EVLGETLOG( uint32_t argc, char *argv[] );
uint32_t DBG_CommandLine_EVLINIT( uint32_t argc, char *argv[] );
#if (USE_DTLS == 1)
uint32_t DBG_CommandLine_Dtls( uint32_t argc, char *argv[] );
#endif
#if ( USE_MTLS == 1 )
uint32_t DBG_CommandLine_mtlsStats ( uint32_t argc, char *argv[] );
#endif
#if  ( ( MAC_LINK_PARAMETERS == 1 ) && ( DCU == 1 ) )
uint32_t DBG_CommandLine_MAC_LinkParameters ( uint32_t argc, char *argv[] );
#endif
#if ( TM_ROUTE_UNKNOWN_MFG_CMDS_TO_DBG == 1 ) /* This makes DBG commands accessible from MFG port */
void DBG_CommandLine_InvokeDebugCommandFromManufacturingPort ( char * pString );
#endif

#endif /* CompileSwitch_H */

/* FUNCTION DEFINITIONS */

#endif /* this must be the last line of the file */