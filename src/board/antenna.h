/**
 * @defgroup antenna
 *
 * @brief Antenna Hardware Driver
 *
 * @details Spacecraft Antenna Hardware Driver.
 *
 * @ingroup drivers
 */

#ifndef __ANTENNA_H__
#define __ANTENNA_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @function
 * @ingroup antenna
 *
 * Attempt to deploy the antenna by 
 * switching on the MOSFET for a short period.
 *
 */
void Antenna_deploy(void);

/**
 * @function
 * @ingroup antenna
 *
 * Reads the state of the Antenna Deployment Switch.
 *
 * @returns boolean value of switch
 */
bool Antenna_read_deployment_switch(void);

#endif /*  __ANTENNA_H__ */