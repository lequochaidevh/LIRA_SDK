#include <cstdint>
#include <string>
#include <vector>

#include "lirasdk_export.h"

// Taken from https://stackoverflow.com/a/13935718/8548472

namespace lirasdk {

/**
 * @brief Encode raw bytes to a base64 string
 *
 * @param raw Raw bytes
 *
 * @return Base64 string
 */
LIRASDK_PUBLIC std::string base64_encode(std::vector<uint8_t> &raw);

/**
 * @brief Decode a base64 string into raw bytes
 *
 * @param str Base64 string
 *
 * @return Raw bytes
 */
LIRASDK_PUBLIC std::vector<uint8_t> base64_decode(const std::string &str);

}  // namespace lirasdk
