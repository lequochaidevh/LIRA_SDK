#pragma once

#include <sstream>
#include "lira_type.hpp"
#include "lirasdk_export.h"

namespace lirasdk {

/**
 * @brief Vehicle type
 */
enum class Vehicle {
    Unknown,            // Vehicle unknown
    Generic,            // Generic micro air vehicle
    QuadRobot,          // Four wheel robot
    GenericMultirotor,  // Generic multirotor
};

/**
 * @brief Stream operator to print information about a `Vehicle`.
 *
 * @return A reference to the stream.
 */
LIRASDK_PUBLIC std::ostream& operator<<(std::ostream& os, const Vehicle& vehicle);

/**
 * @brief Convert a `LiraType` to a `Vehicle`.
 *
 * @return The corresponding `Vehicle`, or `Vehicle::Unknown` if the type is not a vehicle.
 */
LIRASDK_PUBLIC Vehicle to_vehicle_from_lira_type(LiraType type);

}  // namespace lirasdk
