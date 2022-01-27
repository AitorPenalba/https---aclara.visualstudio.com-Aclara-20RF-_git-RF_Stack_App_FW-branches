// <editor-fold defaultstate="collapsed" desc="File Header">
/***********************************************************************************************************************
 *
 * Filename: hmc_mtr_lp.c
 *
 * Global Designator: HMC_LP_
 *
 * Contents: Reads LP data from the host meter.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2013 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * $Log$
 *
 *  Created 04/18/13     KAD  Created
 *
 ***********************************************************************************************************************
 ***********************************************************************************************************************
 * Revision History:
 * v1.0 - Initial Release
 **********************************************************************************************************************/
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Included Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#define HMC_LP_GLOBAL
#include "hmc_mtr_lp.h"
#undef HMC_LP_GLOBAL

#include "hmc_prot.h"
#include "filenames.h"
#include "byteswap.h"
#include "object_list.h"

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */

#define MAX_TBL_READ_BYTES       ((uint8_t)40)       /* Maximum table size to read from meter. */
#define MAX_LP_CHANNELS          ((uint8_t)20)       /* The KV2 can handle 20 channels per set */
#define MAX_LP_SETS              ((uint8_t)4)        /* The KV2 supports 1 set */
#define LP_FILE_UPDATE_RATE_SEC  ((uint32_t)86400)   /* Once per day */
#define LP_RETRIES               ((uint8_t)1)
#define FILE_ST12_SIZE           ((uint16_t)(sizeof(stdTbl12_t) * (MAX_LP_SETS * MAX_LP_CHANNELS)))

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="File Variables - Static">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */

static FileHandle_t        lpFileHndl_ = {0};   /* Contains the file handle information. */
static tReadPartial        tblParams_;    /* Table parameters to read */
static HMC_PROTO_file_t    fileLpInfo_;   /* Contains file information for writing in the protocol layer. */
static bool             bLpEnabled = true;
static stdTbl61_t          actualLpTbl_ = {0};  /* Actual Load Profile Table */
static bool             bLpReady = false;

static HMC_PROTO_TableCmd  cmdProto_[] =      /* Table Command for protocol layer. */
{
   { HMC_PROTO_MEM_RAM, sizeof(tblParams_), (uint8_t far *) & tblParams_ },
   { (enum HMC_PROTO_MEM_TYPE)((uint8_t)HMC_PROTO_MEM_FILE | HMC_PROTO_MEM_WRITE), sizeof(fileLpInfo_), (uint8_t far *)&fileLpInfo_ },
   {HMC_PROTO_MEM_NULL }
};

uint8_t xxxFileData[150];


// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */

typedef struct
{
   stdTbl62_t        lpControlTbl;                          /* Load Profile Control Table */
   stdTbl12_t        sumCfg[MAX_LP_SETS][MAX_LP_CHANNELS];  /* Store configuration for the max LP channels */
   stdTbl15_t        cnstTbl[MAX_LP_SETS][MAX_LP_CHANNELS]; /* Store configuration for the max LP channels */
}lpFileData_t;                                              /* File for the load profile module. */

typedef enum
{
   eLP_START_STATE = (uint8_t)0,
   eLP_ST61 = eLP_START_STATE,   /* Reading the meter's Actual Load Profile Table from STD Table 61 */
   eLP_ST62,                     /* Populates ST62 This is a multi-step process because of the size of ST62. */
   eLP_ST12,                     /* Populates ST12 in NV from the meter's ST12.  ST62 contains the source offset.  */
   eLP_IDLE                      /* Complete, nothing to do */
}lpState_t;

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Constant Definitions">
/* ****************************************************************************************************************** */
/* CONSTANTS */

// <editor-fold defaultstate="collapsed" desc="lpFileAttr_ - File Attributes">
static const FileAttributes_t lpFileAttr_ = /* The attributes for the file. */
{
   0 /* This module doesn't use the checksum. */
}; // </editor-fold>

// <editor-fold defaultstate="collapsed" desc="cmdProto_ & hmcTbl_ - This is for the RAM Commands">
static const HMC_PROTO_Table hmcTbl_[] =
{
   &cmdProto_[0],
   NULL
}; // </editor-fold>

// <editor-fold defaultstate="collapsed" desc="STD Table #61 - Actual Load Profile Table">
static const tReadFull Table_ActualLpTbl_[] =
{
   CMD_TBL_RD_FULL,
   BIG_ENDIAN_16BIT(STD_TBL_ACTUAL_LOAD_PROFILE_LIMITING), /* ST.Tbl 61 */  /*lint !e572 !e778 !e835 */
};

static const HMC_PROTO_TableCmd Cmd_ActualLpTbl_[] =
{
   {HMC_PROTO_MEM_PGM, (uint8_t)sizeof(Table_ActualLpTbl_), (uint8_t far *)&Table_ActualLpTbl_[0] },
   {(enum HMC_PROTO_MEM_TYPE)((uint16_t)HMC_PROTO_MEM_RAM | (uint16_t)HMC_PROTO_MEM_WRITE), sizeof(actualLpTbl_), (uint8_t *)&actualLpTbl_},
   {HMC_PROTO_MEM_NULL }
}; // </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Tables_lpActLpTbl - List of tables that are read from the host meter.">
static const HMC_PROTO_Table Tables_lpActLpTbl[] =
{
   &Cmd_ActualLpTbl_[0],         /* Read ST61 */
   NULL
}; // </editor-fold>

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Local Function Prototypes">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */

// </editor-fold>

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

/* ****************************************************************************************************************** */
/* Init Applet API */

// <editor-fold defaultstate="collapsed" desc="uint8_t HMC_LP_initApp(uint8_t cmd, void *pData)">
/***********************************************************************************************************************
 *
 * Function name: HMC_LP_initApp
 *
 * Purpose: Reads LP configuration data after a power up or after LP configuration changes.  Reads requested LP data.
 *
 * Arguments: uint8_t cmd, void *pData
 *
 * Returns: uint8_t Results of request
 *
 **********************************************************************************************************************/
uint8_t HMC_LP_initApp(uint8_t cmd, void *pData)
{
   static bool       bReadFromMeter_;           /* true = Read from host meter, false = Do nothing. */
   static lpState_t  state_;                    /* State of this module */
   static uint16_t   fileDestOffset_;
   static uint8_t    set_;                      /* LP Set currently being worked on.  There can be 4 sets. */
   static uint8_t    cnt_ = 0;
   static uint8_t    bytesRead_ = 0;
   static uint32_t   tableOffset_ = 0;
   static uint8_t    retries_ = LP_RETRIES;     /* Number of times this module will retry */

   enum HMC_APP_API_RPLY retVal = HMC_APP_API_RPLY_IDLE;

   switch (cmd)
   {
      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_INIT">
      case HMC_APP_API_CMD_INIT:
      {
         FileStatus_t fileStatus;                         /* Contains the file status */

         if ( eSUCCESS != FIO_fopen(&lpFileHndl_,         /* File Handle so that this module can access the file. */
                           ePART_SEARCH_BY_TIMING,        /* Search for the best partition according to the timing. */
                           (uint16_t)eFN_LPDATA,          /* File ID (filename) */
                           (lCnt)sizeof(lpFileData_t),    /* Size of the data in the file. */
                           &lpFileAttr_,                  /* File attributes to use. */
                           LP_FILE_UPDATE_RATE_SEC,
                           &fileStatus) )
         {
            NOP(); //Todo:  Fail!!!!
         }
         state_ = eLP_START_STATE;  /* Start by reading Actual Load Profile Table */
         set_ = 0;
         bReadFromMeter_ = true;    /* Need to read data from the host meter */
         bLpEnabled = true;
         /* Fall through to Activate */
      } // FALLTHROUGH
      // </editor-fold>

      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_ACTIVATE">
      case HMC_APP_API_CMD_ACTIVATE:
      {
         if (bLpEnabled)
         {
            retries_ = LP_RETRIES;
            bReadFromMeter_ = true;
            state_ = eLP_START_STATE;
            fileDestOffset_ = 0;
         }
         break;
      }// </editor-fold>

      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_STATUS">
      case HMC_APP_API_CMD_STATUS:
      {
         if ( bLpEnabled && bReadFromMeter_ )
         {
            retVal = HMC_APP_API_RPLY_READY_COM;
         }
         break;
      }// </editor-fold>

      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_PROCESS">
      case HMC_APP_API_CMD_PROCESS:
      {
         tblProp_t tblProp;
         if (eSUCCESS == HMC_MTRINFO_tableExits(eSTD_TBL, STD_TBL_ACTUAL_LOAD_PROFILE_LIMITING, &tblProp))
         {
            if (tblProp.exists)
            {
               retVal = HMC_APP_API_RPLY_READY_COM;
               /* Determine what table needs to be read by the state_ variable. */
               switch ( state_ )
               {
                  case eLP_ST61:      /* Read Std Tables pointed to by Tables_lpInfo_ */
                  {
                     (void)memset(&actualLpTbl_, 0, sizeof(actualLpTbl_));
                     ((HMC_COM_INFO *)pData)->pCommandTable = (uint8_t far *)Tables_lpActLpTbl;
                     break;
                  }
                  case eLP_ST62:    /* Gather all data from ST62.  Read only the 'sets' that are present. */
                  {
                     bytesRead_ = cnt_;

                     if (bytesRead_ > MAX_TBL_READ_BYTES)
                     {
                        bytesRead_ = MAX_TBL_READ_BYTES;
                     }

                     /* Configure ANSI Partial Table Read parameters */
                     (void)memcpy(&tblParams_.ucOffset[0], &tableOffset_, sizeof(tblParams_.ucOffset));
                     Byte_Swap(&tblParams_.ucOffset[0], sizeof(tblParams_.ucOffset));
                     tblParams_.ucServiceCode = CMD_TBL_RD_PARTIAL;
                     tblParams_.uiTbleID = BIG_ENDIAN_16BIT(STD_TBL_LOAD_PROFILE_CONTROL);
                     tblParams_.uiCount = BIG_ENDIAN_16BIT(bytesRead_);                 /*lint !e572 */
                     cmdProto_[1].ucLen = (uint8_t)bytesRead_;

                     /* Set up the location to write the data after the data is read. */
                     fileLpInfo_.pFileHandle = &lpFileHndl_;
                     fileLpInfo_.offset = fileDestOffset_;

                     ((HMC_COM_INFO *)pData)->pCommandTable = (uint8_t far *)&hmcTbl_[0];

                     break;
                  }
                  case eLP_ST12:    /* Gather all data from ST12 that is pointed to by ST62. */
                  {
                     loadProfileCtrl_t lpProfileCtrl;

                     if (eSUCCESS == HMC_LD_readSetxChx(set_, cnt_, &lpProfileCtrl))
                     {
                        /* Configure ANSI Partial Table Read parameters */
                        uint32_t   tableOffset = lpProfileCtrl.lpSelSet.lpSrcSel * sizeof(stdTbl12_t);
                        (void)memcpy(&tblParams_.ucOffset[0], &tableOffset, sizeof(tblParams_.ucOffset));
                        Byte_Swap(&tblParams_.ucOffset[0], sizeof(tblParams_.ucOffset));
                        tblParams_.ucServiceCode = CMD_TBL_RD_PARTIAL;
                        tblParams_.uiTbleID = BIG_ENDIAN_16BIT(STD_TBL_UNITS_OF_MEASURE_ENTRY);
                        tblParams_.uiCount = BIG_ENDIAN_16BIT(sizeof(stdTbl12_t));                 /*lint !e572 */

                        /* Set up the location to write the data after the data is read. */
                        fileLpInfo_.pFileHandle = &lpFileHndl_;
                        fileLpInfo_.offset = fileDestOffset_;

                        ((HMC_COM_INFO *)pData)->pCommandTable = (uint8_t far *) & hmcTbl_[0];
                     }
                     break;
                  }
                  case eLP_IDLE:    /* Nothing to do */
                  {
                     break;
                  }
                  default: /* Error Condition, start over */
                  {
                     state_ = eLP_ST61;
                     ((HMC_COM_INFO *) pData)->pCommandTable = (uint8_t far *)Tables_lpActLpTbl;
                  }
               }
            }
            else
            {
               bLpEnabled = false;
            }
         }
         break;
      }// </editor-fold>

      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_MSG_COMPLETE">
      case HMC_APP_API_CMD_MSG_COMPLETE:
      {
         switch ( state_ )
         {
            case eLP_ST61: /* The results were saved to NV.  Set up to perform the next set of reads. */
            {
               /* Prepare for the next step, reading ST 62. */
               state_++;      /* Increment State */
               tableOffset_ = 0;
               fileDestOffset_ = (uint16_t)offsetof(lpFileData_t, lpControlTbl);  /* Start at the correct file offset */
               /* The following is calculated by using table 62 definition.   See the ANSI C12 spec.  The variable _cnt
                * will be used to count the number of bytes to read from ST62. */
               for (cnt_ = 0, set_= 0; set_ < LP_NUM_OF_SETS; set_++)
               {
                  if (0 != actualLpTbl_.lpDataSet[set_].nbrChnsSet)
                  {
                     cnt_ += (1 + (uint8_t)(actualLpTbl_.lpDataSet[set_].nbrChnsSet * 7));
                  }
               }
               break;
            }
            case eLP_ST62:
            {
               fileDestOffset_ += bytesRead_;
               tableOffset_ += bytesRead_;
               cnt_ -= bytesRead_;
               if (0 == cnt_)
               {
                  state_++;
                  set_ = 0;
                  fileDestOffset_ = (uint16_t)offsetof(lpFileData_t, sumCfg);
               }
               break;
            }
            case eLP_ST12:
            {
               fileDestOffset_ += sizeof(stdTbl12_t);
               if (++cnt_ >= MAX_LP_CHANNELS)
               {
                  cnt_ = 0;
                  if (++set_ >= LP_NUM_OF_SETS)
                  {
                     state_++;
                  }
               }
               break;
            }
            case eLP_IDLE:
            {
               bReadFromMeter_ = false;
               bLpReady = true;
               break;
            }
            default:
            {
               state_ = eLP_ST61;  /* Don't know how we got here, but what else is corrupt?  Recollect all data */
            }
         }
         if ( bReadFromMeter_ )
         {
            retVal = HMC_APP_API_RPLY_READY_COM;
         }
         break;
      }// </editor-fold>

      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_TBL_ERROR">
      case HMC_APP_API_CMD_TBL_ERROR:
      {
         bReadFromMeter_ = false;
         break;
      }
      // </editor-fold>

      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_ABORT">
      case HMC_APP_API_CMD_ABORT:
      {
         if (0 == --retries_)
         {
            bReadFromMeter_ = false;
            break;
         }
         break;
      }
      // </editor-fold>

      // <editor-fold defaultstate="collapsed" desc="HMC_APP_API_CMD_ERROR">
      case HMC_APP_API_CMD_ERROR:
      {
         if (0 == --retries_)
         {
            bReadFromMeter_ = false;
            break;
         }
         break;
      }// </editor-fold>

      // <editor-fold defaultstate="collapsed" desc="default">
      default: // No other commands are supported in this applet
      {
         retVal = HMC_APP_API_RPLY_UNSUPPORTED_CMD;
         break;
      }// </editor-fold>
   }
   return ((uint8_t)retVal);
}
/* ****************************************************************************************************************** */
// </editor-fold>

/* ****************************************************************************************************************** */
/* External APIs */

// <editor-fold defaultstate="collapsed" desc="returnStatus_t HMC_LD_getLPTbl(stdTbl61_t **pStdTbl61)">
/***********************************************************************************************************************
 *
 * Function name: HMC_LD_getLPTbl
 *
 * Purpose: Sets a point to a pointer passed in to the local copy of ST61.  If the local copy of ST61 hasn't been read,
 *          and error is returned and the pointer is set to NULL.
 *
 * Arguments: stdTbl61_t **pStdTbl61 - Pointer to a Pointer to the local copy of ST61
 *
 * Returns: returnStatus_t - eSUCCESS or eFAILURE
 *
 **********************************************************************************************************************/
returnStatus_t HMC_LD_getLPTbl(stdTbl61_t **pStdTbl61)
{
   returnStatus_t retVal;

   if (bLpReady)
   {
      *pStdTbl61 = &actualLpTbl_;  /* Actual Load Profile Table */
      retVal = eSUCCESS;
   }
   else
   {
      *pStdTbl61 = NULL;
      retVal = eFAILURE;
   }
   return(retVal);
}// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="returnStatus_t HMC_LD_readSetxChx(uint8_t set, uint8_t ch, loadProfileCtrl_t *plpCtrl)">
/***********************************************************************************************************************
 *
 * Function name: HMC_LD_readSetxChx
 *
 * Purpose: Reads the channel information from set X and channel X.
 *
 * Arguments: uint8_t set - Set is 0 based.  If "set 1" is requested, this function expects 0 passed in
 *            uint8_t ch  - ch is 0 based.  If "ch 1" is requested, this function expects 0 passed in
 *            loadProfileCtrl_t *plpCtrl - Location to store the data.
 *
 * Returns: eSUCCESS or eFAILURE
 *
 **********************************************************************************************************************/
returnStatus_t HMC_LD_readSetxChx(uint8_t set, uint8_t ch, loadProfileCtrl_t *pLpCtrl)
{
   returnStatus_t retVal = eFAILURE;

   (void)FIO_fread(&lpFileHndl_, &xxxFileData[0], (uint16_t)offsetof(lpFileData_t,lpControlTbl), 127);

   if (set < LP_NUM_OF_SETS)  /* Error check:  Is set in range? */
   {  /* Does the set have associated data (is it active) and is the ch field in range? */
      if ((0 != actualLpTbl_.lpDataSet[set].nbrChnsSet) && (ch < actualLpTbl_.lpDataSet[set].nbrChnsSet))
      {
         /* Calculate the "set" offset.  This will be the starting point to reset the channel information. */
         uint16_t lpFileOffset = 0;
         uint8_t i;
         for (i = 0; i < set; i++)
         {
            if (0 != actualLpTbl_.lpDataSet[set].nbrChnsSet)
            {
               lpFileOffset += (1 + (actualLpTbl_.lpDataSet[i].nbrChnsSet * 7));
            }
         }
         /* Read the lp_sel_set field - This should be 3 bytes! */
         if (eSUCCESS == FIO_fread(&lpFileHndl_, (uint8_t *)&pLpCtrl->lpSelSet,
                                   lpFileOffset + (sizeof(pLpCtrl->lpSelSet) * ch),
                                   sizeof(pLpCtrl->lpSelSet)))
         {
            /* Adjust of the lpFileOffset.  The LP_SEL_SETx field is an array. */
            lpFileOffset += ((uint16_t)sizeof(pLpCtrl->lpSelSet) * actualLpTbl_.lpDataSet[set].nbrChnsSet);
            /* Read the INT_FMT_CDEx field.  */
            if (eSUCCESS == FIO_fread(&lpFileHndl_, &pLpCtrl->intFmtCde, lpFileOffset, sizeof(pLpCtrl->intFmtCde)))
            {
               /* Adjust of the lpFileOffset to point to the start of the SCALARS_SETx field. */
               lpFileOffset += sizeof(pLpCtrl->intFmtCde);
               /* Read the SCALARS_SETx field.  */
               if (eSUCCESS == FIO_fread(&lpFileHndl_, (uint8_t *)&pLpCtrl->scalarsSet,
                                         lpFileOffset + (sizeof(pLpCtrl->scalarsSet) * ch),
                                         sizeof(pLpCtrl->scalarsSet)))
               {
                  uint8_t bigEndian;
                  if (eSUCCESS == HMC_MTRINFO_endian(&bigEndian))
                  {
                     if (bigEndian)
                     {
                        Byte_Swap((uint8_t *)&pLpCtrl->scalarsSet, sizeof(pLpCtrl->scalarsSet));
                     }
                     /* Adjust of the lpFileOffset to point to the start of the DIVISORS_SETx field. */
                     lpFileOffset += (sizeof(pLpCtrl->scalarsSet) * LP_SET1_SIZE);
                     /* Read the DIVISORS_SETx field.  */
                     retVal = FIO_fread(&lpFileHndl_, (uint8_t *)&pLpCtrl->divisorsSet,
                                        lpFileOffset + (sizeof(pLpCtrl->divisorsSet) * ch),
                                        sizeof(pLpCtrl->divisorsSet));
                     if (bigEndian)
                     {
                        Byte_Swap((uint8_t *)&pLpCtrl->divisorsSet, sizeof(pLpCtrl->divisorsSet));
                     }
                  }
               }
            }
         }
      }
   }
   return(retVal);
}// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="returnStatus_t HMC_LP_getQuantityProp(uint8_t set, quantity_t qs, uint8_t *pCh, quantDesc_t *pProperties)">
/***********************************************************************************************************************
 *
 * Function name: HMC_LP_getQuantityProp
 *
 * Purpose: Checks if a requested quantity passed in matches an entity the meter has in Std Tbl #62
 *
 * Arguments: uint8_t set, quantity_t qs, lpQuantDesc_t *pProperties
 *
 * Returns: eSUCCESS or eFAILURE
 *
 **********************************************************************************************************************/
returnStatus_t HMC_LP_getQuantityProp(uint8_t set, quantity_t qs, uint8_t *pCh, quantDesc_t *pProperties)
{
   returnStatus_t    retVal = eFAILURE;
   quantitySig_t     *pSig;
   tblProp_t         stdTbl13Prop;

   stdTbl13Prop.exists = 0;

   if (eSUCCESS == OL_Search(OL_eHMC_QUANT_SIG, (ELEMENT_ID)qs, (void **)&pSig)) /* Does quantity requested exist? */
   {
      (void)HMC_MTRINFO_tableExits(eSTD_TBL, STD_TBL_DEMAND_CONTROL, &stdTbl13Prop);
      uint16_t            stdTbl12Idx;
      stdTbl12_t        stdTbl12;
      uint16_t            stdTbl12Offset;
      niFormatEnum_t    niFormat1;
      uint8_t             niDataSize1;
      niFormatEnum_t    niFormat2;
      uint8_t             niDataSize2;

      for (stdTbl12Idx = 0, stdTbl12Offset = (uint16_t)offsetof(lpFileData_t, sumCfg) + (sizeof(stdTbl12_t)*set);
           stdTbl12Idx < FILE_ST12_SIZE;
           stdTbl12Idx++, stdTbl12Offset += sizeof(stdTbl12_t))
      {
         (void)FIO_fread(&lpFileHndl_, (uint8_t *)&stdTbl12, stdTbl12Offset, sizeof(stdTbl12));
         if ((pSig->sig.tbl12.idCode == stdTbl12.idCode) && (pSig->sig.tbl12.timeBase == stdTbl12.timeBase))
         {
            /* Check the Qx Accountability Values, Net Flow Accountability, segmentation & harmonic */
            if (((pSig->sig.tbl12.q1Accountability == stdTbl12.q1Accountability)      || pSig->sig.wild.q1) &&
                ((pSig->sig.tbl12.q2Accountability == stdTbl12.q2Accountability)      || pSig->sig.wild.q2) &&
                ((pSig->sig.tbl12.q3Accountability == stdTbl12.q3Accountability)      || pSig->sig.wild.q3) &&
                ((pSig->sig.tbl12.q4Accountability == stdTbl12.q4Accountability)      || pSig->sig.wild.q4) &&
                ((pSig->sig.tbl12.netFlowAccountability == stdTbl12.netFlowAccountability) || pSig->sig.wild.netFlowAcnt) &&
                ((pSig->sig.tbl12.segmentation     == stdTbl12.segmentation)          || pSig->sig.wild.segmenatation)&&
                ((pSig->sig.tbl12.harmonic         == stdTbl12.harmonic)              || pSig->sig.wild.harmonic) )
            {
               (void)HMC_MTRINFO_niFormat1(&niFormat1, &niDataSize1);
               (void)HMC_MTRINFO_niFormat2(&niFormat2, &niDataSize2);
               retVal = FIO_fread(&lpFileHndl_,
                                  (uint8_t *)&pProperties->multiplier15,
                                  (uint16_t)offsetof(lpFileData_t, cnstTbl) + (sizeof(stdTbl15_t) * stdTbl12Idx),
                                  sizeof(pProperties->multiplier15));
               pProperties->multiplier12 = stdTbl12.multiplier;
               pProperties->dataFmt = niFormat1;
               uint8_t endian;
               (void)HMC_MTRINFO_endian(&endian);
               pProperties->endian = endian;
               *pCh = (uint8_t)stdTbl12Idx;
               break;
            }
         }
      }
   }
   else
   {
      (void)memset(pProperties, 0, sizeof(quantDesc_t));
   }
   return(retVal);
}// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="returnStatus_t HMC_LP_getFmtSize(uint8_t *pSize)">
/***********************************************************************************************************************
 *
 * Function name: HMC_LP_getFmtSize
 *
 * Purpose: Checks if a requested quantity passed in matches an entity the meter has in Std Tbl #62
 *
 * Arguments: uint8_t set, quantity_t qs, lpQuantDesc_t *pProperties
 *
 * Returns: eSUCCESS or eFAILURE
 *
 **********************************************************************************************************************/
returnStatus_t HMC_LP_getFmtSize(uint8_t *pSize)
{
   returnStatus_t    retVal = eSUCCESS;

   if (actualLpTbl_.invUnit8Flag || actualLpTbl_.invUnit8Flag)
   {
      *pSize = 1; /* size of uint8_t */
   }
   else if (actualLpTbl_.invUnit16Flag || actualLpTbl_.invInt16Flag)
   {
      *pSize = 2; /* size of uint16_t */
   }
   else if (actualLpTbl_.invUnit32Flag || actualLpTbl_.invInt32Flag)
   {
      *pSize = 4; /* size of uint32_t */
   }
   else if (actualLpTbl_.invNiFmt1Flag)
   {
      niFormatEnum_t fmt1;
      (void)HMC_MTRINFO_niFormat1(&fmt1, pSize);
   }
   else if (actualLpTbl_.invNiFmt2Flag)
   {
      niFormatEnum_t fmt2;
      (void)HMC_MTRINFO_niFormat2(&fmt2, pSize);
   }
   else
   {
      retVal = eFAILURE;
   }

   return(retVal);
}// </editor-fold>
