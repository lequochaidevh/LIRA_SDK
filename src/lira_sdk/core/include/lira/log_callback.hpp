#pragma once

#include <string>
#include <functional>

#include "lirasdk_export.h"

namespace lirasdk::log {

// defined numeric values can be useful for filtering message by severity
enum class Level : int { Debug = 0, Info = 1, Warn = 2, Err = 3 };

/** @brief User-defined callback for logging. Returning true from this callback
 * prevents default lirasdk`s logging to stderr. Returning false keeps it.
 */
using Callback = std::function<bool(Level level, const std::string& message, const std::string& file, int line)>;

LIRASDK_PUBLIC Callback& get_callback();
LIRASDK_PUBLIC void      subscribe(const Callback& callback);

}  // namespace lirasdk::log
