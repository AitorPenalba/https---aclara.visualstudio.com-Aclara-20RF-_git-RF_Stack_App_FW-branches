; vim: ts=8 sw=8
/*HEADER**********************************************************************
* filename boot.S
*
* Adapted from Freescale Semiconductor, Inc. boot.S (MQX) and IAR's cstartup_M.s
*
*END************************************************************************/


        MODULE  ?cstartup

        PUBLIC  __iar_program_start
        EXTERN  __vector_table
        EXTWEAK __iar_init_core
        EXTWEAK __iar_init_vfp
        EXTWEAK __iar_data_init3

        SECTION .text:CODE:REORDER(4)
        THUMB


/*FUNCTION*-------------------------------------------------------------------

 Function Name    : __iar_program_start
 Returned Value   :
 Comments         : startup sequence

 END*-----------------------------------------------------------------------*/

NVIC_ICER0 EQU  0xE000E180 
NVIC_ICPR0 EQU  0xE000E280 


__iar_program_start:

#if MQX_AUX_CORE
        msr MSP, r0
        isb #15
#endif

        /* Disable interrupts and clear pending flags */
        ldr r0, =NVIC_ICER0
        ldr r1, =NVIC_ICPR0
        ldr r2, =0xFFFFFFFF
        mov r3, #8

_boot_loop
        cbz r3, _boot_loop_end
        str r2, [r0], #4        /* NVIC_ICERx - clear enable IRQ register */
        str r2, [r1], #4        /* NVIC_ICPRx - clear pending IRQ register */
        sub r3, r3, #1
        b _boot_loop

_boot_loop_end

        /* Prepare process stack pointer */
        mrs r0, MSP
        msr PSP, r0

        /* Switch to proccess stack (PSP) */
        mrs r0, CONTROL
        orr r0, r0, #2
        msr CONTROL, r0
        isb #15

#if MQXCFG_ENABLE_FP && PSP_HAS_FPU
        /* CPACR is located at address 0xE000ED88 */
        LDR.W   R0, =0xE000ED88
        /* Read CPACR */
        LDR     R1, [R0]
        /* Set bits 20-23 to enable CP10 and CP11 coprocessors */
        ORR     R1, R1, #(0xF << 20)
        /* Write back the modified value to the CPACR */
        STR     R1, [R0]
        /* turn off fpu register stacking in exception entry */

        ldr r0, =0xE000EF34     /* FPCCR */
        mov r1, #0
        str r1, [r0]
#endif
        extern  WDOG_Disable
        bl      WDOG_Disable

        /* Perform toolchain startup routines */
        extern  __iar_init_vfp
        extern  __iar_data_init3

        bl      __iar_init_vfp
        bl      __iar_data_init3

        movs    r0,#0


        EXTERN  main
        b       main

        /* Make sure __vector_table is loaded by the linker by forcing a reference to it. */
        REQUIRE __vector_table
        END
        ALIGN 4
        END
