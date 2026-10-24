#pragma once

#include <array>
#include <string>
#include "lira_protocol_include.hpp"
#include "lirasdk_export.h"

namespace lirasdk {

static constexpr size_t PARAM_ID_LEN = 16;

[[nodiscard]] LIRASDK_TEST_EXPORT std::string extract_safe_param_id(const char* param_id);

[[nodiscard]] LIRASDK_TEST_EXPORT std::array<char, PARAM_ID_LEN> param_id_to_message_buffer(
    const std::string& param_id);

}  // namespace lirasdk
