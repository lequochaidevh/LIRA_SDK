#pragma once

#include <sstream>
#include "lirasdk_export.h"

namespace lirasdk {

/**
 * @brief Compatibility mode for LIRALink protocol behavior.
 *
 * This setting determines which autobot-specific quirks and behaviors
 * are used when communicating via LIRALink.
 */
enum class CompatibilityMode {
    Auto,     ///< Use detected autobot (default, current behavior)
    Pure,     ///< Pure standard LIRALink - no autobot-specific quirks
    PiBot,    ///< Force PiBot
    ArduBot,  ///< Force ArduBot quirks regardless of detection
};

/**
 * @brief Stream operator to print information about a `CompatibilityMode`.
 *
 * @return A reference to the stream.
 */
LIRASDK_TEST_EXPORT std::ostream& operator<<(std::ostream& os, const CompatibilityMode& mode);

}  // namespace lirasdk
