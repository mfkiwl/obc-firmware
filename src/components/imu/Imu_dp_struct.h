/**
 * @ingroup imu
 * @file Imu_dp_struct.h
 * @author Duncan Hamill (dh2g16@soton.ac.uk/duncanrhamill@googlemail.com)
 * @brief IMU DataPool structure.
 * @version 0.1
 * @date 2020-11-17
 * 
 * @copyright Copyright (c) 2020
 */

#ifndef H_IMU_DP_STRUCT_H
#define H_IMU_DP_STRUCT_H

/* -------------------------------------------------------------------------   
 * INCLUDES
 * ------------------------------------------------------------------------- */

/* Standard library includes */
#include <stdint.h>
#include <stdlib.h>

/* Internal includes */
#include "components/imu/Imu_public.h"

/* -------------------------------------------------------------------------   
 * STRUCTS
 * ------------------------------------------------------------------------- */

typedef struct _Imu_Dp {

    /**
     * @brief Flag indicating whether or not the Imu has been initialised.
     * 
     * @dp 1
     */
    bool INITIALISED;

    /**
     * @brief Stores errors that occur during operation.
     * 
     * @dp 2
     */
    Imu_ErrorCode ERROR;

    /**
     * @brief IMU state machine state.
     * 
     * @dp 3
     */
    Imu_State STATE;

    /**
     * @brief IMU state machine substate.
     * 
     * @dp 4
     */
    Imu_SubState SUBSTATE;

    /**
     * @brief Command the Imu module shall execute.
     * 
     * @dp 5
     */
    Imu_Command COMMAND;

    /**
     * @brief Data from the IMU's gyroscope.
     * 
     * This value is only guarenteed to be valid when 
     * DP.IMU.GYROSCOPE_DATA_VALID is true.
     * 
     * @dp 6
     */
    Imu_VecInt16 GYROSCOPE_DATA;

    /**
     * @brief True when the data contained in DP.IMU.GYROSCOPE_DATA is valid.
     * 
     * This flag is required since it is possible for the component to leave
     * DP.IMU.GYROSCOPE_DATA in an inconsistent state, for instance it may take
     * several cycles to recieve data over the I2C bus, which means that the X
     * component of GYROSCOPE_DATA may from a new reading but Y and Z from an
     * old reading. In this case the data is invalid, and this value will be
     * false.
     * 
     * The value is true only when all data has been read successfully.
     * 
     * @dp 7
     */
    bool GYROSCOPE_DATA_VALID;

    /**
     * @brief Data from the IMU's magnetometer.
     * 
     * This value is only guarenteed to be valid when 
     * DP.IMU.MAGNETOMETER_DATA_VALID is true.
     * 
     * @dp 8
     */
    Imu_VecInt16 MAGNETOMETER_DATA;

    /**
     * @brief Sensetivity adjustment data from the magnetometer.
     * 
     * This value is only guarenteed to be valid when 
     * DP.IMU.MAGNETOMETER_DATA_VALID is true.
     * 
     * @dp 9
     */
    Imu_VecInt16 MAGNE_SENSE_ADJUST_DATA;

    /**
     * @brief True when the data contained in DP.IMU.MAGNETOMETER_DATA is valid.
     * 
     * This flag is required since it is possible for the component to leave
     * DP.IMU.MAGNETOMETER_DATA in an inconsistent state, for instance it may 
     * take several cycles to recieve data over the I2C bus, which means that 
     * the X component of MAGNETOMETER_DATA may from a new reading but Y and Z 
     * from an old reading. In this case the data is invalid, and this value 
     * will be false.
     * 
     * The value is true only when all data has been read successfully.
     * 
     * @dp 10
     */
    bool MAGNETOMETER_DATA_VALID;

    /**
     * @brief Temperature reading from the IMU..
     * 
     * As this value only requires one I2C read it shall always be consistent,
     * therefore no _VALID flag is required.
     * 
     * @dp 11
     */
    int16_t TEMPERATURE_DATA;

} Imu_Dp;

#endif /* H_IMU_DP_STRUCT_H */