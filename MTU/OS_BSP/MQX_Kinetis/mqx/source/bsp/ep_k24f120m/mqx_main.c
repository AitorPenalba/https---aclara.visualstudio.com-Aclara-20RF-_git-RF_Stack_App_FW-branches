/*HEADER**********************************************************************
*
* Copyright 2014 Freescale Semiconductor, Inc.
*
* This software is owned or controlled by Freescale Semiconductor.
* Use of this software is governed by the Freescale MQX RTOS License
* distributed with this Material.
* See the MQX_RTOS_LICENSE file distributed for more details.
*
* Brief License Summary:
* This software is provided in source form for you to use free of charge,
* but it is not open source software. You are allowed to use this software
* but you cannot redistribute it or derivative works of it in source form.
* The software may be used only in connection with a product containing
* a Freescale microprocessor, microcontroller, or digital signal processor.
* See license agreement file for full license terms including other restrictions.
*****************************************************************************
*
* Comments:
*
*   This file contains the main C language entrypoint, starting up the MQX
*
*
*END************************************************************************/

#include "mqx.h"
#include "bsp.h"

extern uint8_t PWRLG_LastGasp(void);
extern void    PWRLG_Startup(void);

/*FUNCTION*-------------------------------------------------------------------
*
* Function Name    : main
* Returned Value   : should return "status"
* Comments         :
*   Starts MQX running
*
*END*----------------------------------------------------------------------*/
int main
   (
      void
   )
{ /* Body */

   extern const MQX_INITIALIZATION_STRUCT MQX_init_struct;
   extern const MQX_INITIALIZATION_STRUCT MQX_init_struct_last_gasp;

   PWRLG_Startup();

   if (PWRLG_LastGasp())
   {
      /* Start MQX for last gasp */
      _mqx( (MQX_INITIALIZATION_STRUCT_PTR) &MQX_init_struct_last_gasp );
   }
   else
   {
      /* Start MQX */
      _mqx( (MQX_INITIALIZATION_STRUCT_PTR) &MQX_init_struct );
   }
   return 0;

} /* Endbody */
