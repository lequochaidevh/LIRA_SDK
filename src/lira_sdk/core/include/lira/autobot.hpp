#pragma once

#include <sstream>
#include "lirasdk_export.h"

namespace lirasdk {

/**
 * @brief AutoBot type
 */
enum class AutoBot {
    Unknown,     // AutoBot unknown
    Base,        // Base
    Simulation,  // Simulation
};

/**
 * @brief Stream operator to print information about an `AutoBot`.
 *
 * @return A reference to the stream.
 */
LIRASDK_PUBLIC std::ostream& operator<<(std::ostream& os, const AutoBot& autobot);

}  // namespace lirasdk
