#pragma once

#include <cstdint>
#include <sstream>
#include <string_view>
#include "lirasdk_export.h"

namespace lirasdk {

/**
 * @brief LIRALink type of a component (LIRA_TYPE).
 *
 * The values match the LIRALink LIRA_TYPE enum, so they can be used where LIRALink
 * expects a LIRA_TYPE.
 */
enum class LiraType : uint8_t {
    Generic           = 0,  /**< @brief Generic micro air vehicle */
    AntennaTracker    = 5,  /**< @brief Ground installation */
    Gcs               = 6,  /**< @brief Operator control unit / ground control station */
    OnboardController = 18, /**< @brief Onboard companion controller */
    Gimbal            = 26, /**< @brief Gimbal */
    Adsb              = 27, /**< @brief ADSB system */
    Camera            = 30, /**< @brief Camera */
    ChargingStation   = 31, /**< @brief Charging station */
    Servo             = 33, /**< @brief Servo */
    Decarotor         = 35, /**< @brief Decarotor */
    Battery           = 36, /**< @brief Battery */
    Log               = 38, /**< @brief Log */
    Imu               = 40, /**< @brief IMU */
    Gps               = 41, /**< @brief GPS */
    Radio             = 49, /**< @brief Radio */
};

/**
 * @brief Convert a `LiraType` to a string.
 *
 * @return A string representation of the `LiraType`.
 */
LIRASDK_PUBLIC std::string_view to_string(LiraType lira_type);

/**
 * @brief Stream operator to print information about a `LiraType`.
 *
 * @return A reference to the stream.
 */
LIRASDK_PUBLIC std::ostream& operator<<(std::ostream& os, const LiraType& lira_type);

}  // namespace lirasdk
