// <editor-fold defaultstate="collapsed" desc="File Header Information">
/* ****************************************************************************************************************** */
/***********************************************************************************************************************
 *
 * Filename:   exe_list.c
 *
 * Global Designator: EL_
 *
 * Contents: Runs a series of operations as defined by a NULL terminated list of type specFunct_t.
 *
 ***********************************************************************************************************************
 * A product of
 * Aclara Technologies LLC
 * Confidential and Proprietary
 * Copyright 2012 Aclara.  All Rights Reserved.
 *
 * PROPRIETARY NOTICE
 * The information contained in this document is private to Aclara Technologies LLC an Ohio limited liability company
 * (Aclara).  This information may not be published, reproduced, or otherwise disseminated without the express written
 * authorization of Aclara.  Any software or firmware described in this document is furnished under a license and may be
 * used or copied only in accordance with the terms of such license.
 ***********************************************************************************************************************
 *
 * @author Karl Davlin
 *
 * $Log$ KAD Created June 4, 2012
 *
 ***********************************************************************************************************************
 * Revision History:
 * v0.1 - KAD 06/04/2012 - Initial Release
 *
 * @version    0.1
 * #since      2012-06-04
 **********************************************************************************************************************/
 // </editor-fold>

// <editor-fold defaultstate="collapsed" desc="PSECTs">
/* PSECTS */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Include Files">
/* ****************************************************************************************************************** */
/* INCLUDE FILES */
#include <stdint.h>
#include <stdbool.h>
#define EL_GLOBALS
#include "exe_list.h"
#undef  EL_GLOBALS

// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Macro Definitions">
/* ****************************************************************************************************************** */
/* MACRO DEFINITIONS */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Type Definitions">
/* ****************************************************************************************************************** */
/* TYPE DEFINITIONS */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Constants">
/* ****************************************************************************************************************** */
/* CONSTANTS */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Local Variables (File Static)">
/* ****************************************************************************************************************** */
/* FILE VARIABLE DEFINITIONS */
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="Local Function Prototypes">
/* ****************************************************************************************************************** */
/* FUNCTION PROTOTYPES */
// </editor-fold>

/* ****************************************************************************************************************** */
/* FUNCTION DEFINITIONS */

// <editor-fold defaultstate="collapsed" desc="bool EL_doList(initCmd_t eCmd, exeList_t *pEntry)">
/***********************************************************************************************************************
 *
 * Function Name: EL_doList
 *
 * Purpose: Operates on a list type exeList_t.  The command (initCmd_t eCmd) determines if the list is a run-time
 *          initialize or copy of the source to destination.
 *
 * Arguments: initCmd_t eCmd, exeList_t *pEntry
 *
 * Returns: returnStatus_t - eSUCCESS or eFAILURE
 *
 * Side Effects: Port pins change as well as other special function registers
 *
 * Reentrant Code: Yes
 *
 **********************************************************************************************************************/
returnStatus_t EL_doList(initCmd_t eCmd, exeList_t const *pEntry)
{
   returnStatus_t retVal = eSUCCESS;   /* Returns a failure if any one failure. */
   bool endOfList;                  /* End of List indication for the do-while loop. */
   uint16_t  destVal;                    /* Contains the destination value. */
   do
   {
      endOfList = true;
      if ((NULL != pEntry->pDest) && (NULL != pEntry->pSrc))  /* Make sure the src and dst pointers are valid. */
      {
         endOfList = false;
         destVal = *pEntry->pDest;  /* Read the Destination Value. */
         switch(eCmd)
         {
#ifndef __BOOTLOADER
            case eINIT_RTI_CLR:  /* Is the command a clear command? */
            {
               if (0 != (destVal & (*(uint16_t *)pEntry->pSrc))) /* If any bits are set, that means the SFR is corrupt. */
               {
                  retVal = eFAILURE; /* Set the error flag. */
               }
               destVal &= ~*(uint16_t *)pEntry->pSrc;   /* Clear any bits that were set. */
               break;
            }
            case eINIT_RTI_SET:  /* Is the command a Set comand? */
            {
               /* Are any bits set that shouldn't be? */
               if (*(uint16_t *)pEntry->pSrc != (destVal & *(uint16_t *)pEntry->pSrc))
               {
                  retVal = eFAILURE; /* Set the error flag. */
               }
               destVal |= *(uint16_t *)pEntry->pSrc; /* set any bits that were cleared. */
               break;
            }
#endif
            case eINIT_WR_REG: /* Is the command a write comand? */
            {
               destVal = *(uint16_t *)pEntry->pSrc;  /* Else, normal command, set the register value. */
               break;
            }
            default: /* Can't get here!  The 'if' before the do loop prevents it. */
            {
               break;
            }
         }
         *pEntry->pDest = destVal;  /* Set the destination to its new value. */
      }
      if (NULL != pEntry->Fct)   /* Is the function pointer valid (not null)? */
      {
         endOfList = false;
         if (eFAILURE == pEntry->Fct()) /* Call the indirect function, only change retVal on Failure */
         {
            retVal = eFAILURE; /* The function returned a failure. */
         }
      }
      pEntry++;   /* Increment ptr to operate on the next item in the list. */
   }while(!endOfList);
   return(retVal);
}
/* ****************************************************************************************************************** */
// </editor-fold>

/* ****************************************************************************************************************** */
