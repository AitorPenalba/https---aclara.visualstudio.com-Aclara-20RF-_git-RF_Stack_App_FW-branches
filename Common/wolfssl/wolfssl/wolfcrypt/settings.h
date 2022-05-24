/* settings.h
 *
 * Aclara modified to allow selection of RTOS without changing the WolfSSL File folder structure
 *
 */

/* TODO: RA6: Move the following to Appropriate module */
#define BARE_METAL                        0
#ifdef __mqx_h__
   #define MQX_RTOS                       1
#endif
#define FREE_RTOS                         2

#define RTOS_SELECTION                    FREE_RTOS /* 0 = Bare Metal, 1 = MQX , 2 = FreeRTOS */

#if ( RTOS_SELECTION == MQX_RTOS )
   #include <wolfssl/wolfcrypt/settings_MQX.h>
#elif ( RTOS_SELECTION == FREE_RTOS )
   #include <wolfssl/wolfcrypt/settings_FreeRTOS.h>
#else
   #error MUST select MQX or FreeRTOS
#endif
