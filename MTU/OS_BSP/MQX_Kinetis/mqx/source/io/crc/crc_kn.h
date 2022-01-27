#ifndef __CRC_KN_H
#define __CRC_KN_H 1

/**HEADER********************************************************************
*
* Copyright (c) 2008 Freescale Semiconductor;
* All Rights Reserved
*
***************************************************************************
*
* THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESSED OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************
*
* $FileName: crc_kn.h$
* $Version : 3.8.3.0$
* $Date    : Sep-27-2012$
*
* Comments:
*
*   The file contains definitions used in user program and/or in other
*   kernel modules to access CRC module
*
*END************************************************************************/

/*
*	Error code Definitions
*/
#define CRC_ERR_SUCCESS       0
#define CRC_ERR_CODE_BASE    (0x2000)
#define CRC_ERR_TOT_VAL      (CRC_ERR_CODE_BASE+1)
#define CRC_ERR_TOTR_VAL     (CRC_ERR_CODE_BASE+2)
#define CRC_ERR_FXOR_VAL     (CRC_ERR_CODE_BASE+3)
#define CRC_ERR_TCRC_VAL     (CRC_ERR_CODE_BASE+4)


/*----------------------------------------------------------------------*/
/*
**                    FUNCTION PROTOTYPES
*/

#ifdef __cplusplus
extern "C" {
#endif

uint32_t CRC_Config(uint32_t poly,uint32_t tot,uint32_t totr,uint32_t fxor,uint32_t tcrc);
uint32_t CRC_Cal_16(uint32_t seed,uint8_t *msg, uint32_t sizeBytes);
uint32_t CRC_Cal_32(uint32_t seed,uint8_t *msg, uint32_t sizeBytes);
uint32_t CRC_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __CRC_KN_H */

