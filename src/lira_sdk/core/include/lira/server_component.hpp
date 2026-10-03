#pragma once

#include <cstdint>
#include <memory>

#include "lirasdk_export.h"

namespace lirasdk {

class LirasdkImpl;
class ServerComponentImpl;
class ServerPluginImplBase;

/**
 * @brief This class represents a component, used to initialize a server plugin.
 */
class LIRASDK_PUBLIC ServerComponent {
 public:
    /**
     * @private Constructor, used internally
     *
     * This constructor is not (and should not be) directly called by application code.
     */
    ServerComponent(LirasdkImpl& lirasdk_impl, uint8_t component_id, uint8_t lira_type);

    /**
     * @brief Destructor.
     */
    ~ServerComponent() = default;

    /**
     * @brief LIRALink component ID of this component
     */
    uint8_t component_id() const;

    /**
     * @brief Set system status of this LIRALink entity.
     *
     * The default system status is LIRA_STATE_UNINIT.
     *
     * @param system_status system status.
     */
    void set_system_status(uint8_t system_status);

 private:
    std::shared_ptr<ServerComponentImpl> _impl;

    friend LirasdkImpl;
    friend ServerPluginImplBase;
};

}  // namespace lirasdk
