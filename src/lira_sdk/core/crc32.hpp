#pragma once

#include <cstdint>
#include "lirasdk_export.h"

namespace lirasdk {

// For more information about the CRC algorithm used, check the comment in the
// source file.

class LIRASDK_TEST_EXPORT Crc32 {
 public:
    uint32_t add(const uint8_t* src, uint32_t len);

    [[nodiscard]] uint32_t get() const { return _val; }

 private:
    uint32_t _val{0};
};

}  // namespace lirasdk
