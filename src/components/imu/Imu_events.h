/**
 * @ingroup imu
 * @file Imu_events.h
 * @author Duncan Hamill (dh2g16@soton.ac.uk/duncanrhamill@googlemail.com)
 * @brief Events associated with the Imu module.
 * @version 0.1
 * @date 2020-11-17
 * 
 * @copyright Copyright (c) 2020
 */

#ifndef H_IMU_EVENTS_H
#define H_IMU_EVENTS_H

/* -------------------------------------------------------------------------   
 * INCLUDES
 * ------------------------------------------------------------------------- */

/* Internal includes */
#include "system/event_manager/EventManager_public.h"
#include "system/event_manager/EventManager_blocks.h"

/* -------------------------------------------------------------------------   
 * EVENTS
 * ------------------------------------------------------------------------- */

/**
 * @brief New command event.
 * 
 * This event is raised when a new command is issued by a user to the Imu
 * module. It is used to identify when a new command is available to be
 * executed. The user should not raise this event themselves, instead use the
 * Imu_new_command() function.
 */
#define EVT_IMU_NEW_COMMAND ((Event)(EVENTMANAGER_BLOCK_IMU | 1))

/**
 * @brief IMU gyroscope read command completed successfully.
 */
#define EVT_IMU_READ_GYRO_SUCCESS ((Event)(EVENTMANAGER_BLOCK_IMU | 2))

/**
 * @brief IMU gyroscope read command failed. 
 */
#define EVT_IMU_READ_GYRO_FAILURE ((Event)(EVENTMANAGER_BLOCK_IMU | 3))

/**
 * @brief IMU magnetometer read command completed successfully.
 */
#define EVT_IMU_READ_MAGNE_SUCCESS ((Event)(EVENTMANAGER_BLOCK_IMU | 4))

/**
 * @brief IMU magnetometer read command failed.
 */
#define EVT_IMU_READ_MAGNE_FAILURE ((Event)(EVENTMANAGER_BLOCK_IMU | 5))

/**
 * @brief IMU temeprature read command completed successfully.
 */
#define EVT_IMU_READ_TEMP_SUCCESS ((Event)(EVENTMANAGER_BLOCK_IMU | 6))

/**
 * @brief IMU temperature read command failed.
 */
#define EVT_IMU_READ_TEMP_FAILURE ((Event)(EVENTMANAGER_BLOCK_IMU | 7))

/**
 * @brief Event used to signal a state change in the IMU. This is done so that
 * the new state will run it's first function in the next cycle without the
 * system going to sleep.
 */
#define EVT_IMU_STATE_CHANGE ((Event)(EVENTMANAGER_BLOCK_IMU | 8))

#endif /* H_IMU_EVENTS_H */