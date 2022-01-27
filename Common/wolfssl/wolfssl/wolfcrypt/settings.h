/* settings.h
 *
 * Aclara modified to allow selection of RTOS without changing the WolfSSL File folder structure
 *
 */

#if ( RTOS_SELECTION == MQX )
   #include <wolfssl/wolfcrypt/settings_MQX.h>
#elif ( RTOS_SELECTION == FREERTOS )
   #include <wolfssl/wolfcrypt/settings_FreeRTOS.h>
#else
   #error MUST select MQX or FreeRTOS
#endif
