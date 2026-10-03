#pragma once

#include <cstdint>

/**
 * @brief Struct to represent a LIRALink address.
 */
struct LiralinkAddress {
    /**
     * @brief System ID, also called sysid.
     *
     * 32 bits wide to accommodate LIRALink's extended system IDs, which are
     * not supported yet.
     */
    uint32_t system_id;
    /**
     * @brief Component ID, also called compid.
     */
    uint8_t component_id;
};
