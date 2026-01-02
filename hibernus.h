#ifndef __HIBERNATION_H__
#define __HIBERNATION_H__

#include <stdint.h>
#include <string.h>

// indicating the maximum size of each FRAM snapshot, change if needed
#define STACK_SIZE                      0x00A0 // 160 bytes stack
#define DATA_SIZE                       0x0800 // at most 2KB
#define BSS_SIZE                        0x0800 // at most 2KB

// Function Declarations

/**
 * @brief inits the hibernus library, it restore the snapshot if present
 *
 * @note this function disables watchdog, if needed enable it again.
 */
void hibernus_init(void);

/**
 * @brief takes a snapshot of the memory (just copies RAM and regs in FRAM)
 *
 * @note static registers setups are not taken in the snapshot as it is more efficient ot set them at startup
 * so you will need to set the HW every time (i.e. clock setup and I2C setup)
 */
void hibernate(void);

/**
 * @brief restore the previously taken snapshot
 */
void restore(void);

/**
 * @brief takes a snapshot and sends EOT to CapDyn
 *
 * @param gpio P4 EOT pin
 *
 * @note by default this uses P4, if needed it could be VERY easily changed to 
 * use an arbitrary port
 */
void send_EOT_and_shutdown(uint8_t gpio); 

#endif /* __HIBERNATION_H__ */


