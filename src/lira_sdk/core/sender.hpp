#pragma once

#include "compatibility_mode.hpp"
#include "lira_protocol_include.hpp"
#include "lira_protocol_address.hpp"
#include <asio/io_context.hpp>
#include <cstdint>
#include <functional>

namespace lirasdk {

class Sender {
 public:
    Sender()                                                    = default;
    virtual ~Sender()                                           = default;
    virtual bool send_message(lira_protocol_message_t& message) = 0;
    virtual bool queue_message(
        std::function<lira_protocol_message_t(LiraProtocolAddress lira_protocol_address, uint8_t channel)> fun) = 0;
    [[nodiscard]] virtual uint8_t           get_own_system_id() const                                           = 0;
    [[nodiscard]] virtual uint8_t           get_own_component_id() const                                        = 0;
    [[nodiscard]] virtual CompatibilityMode compatibility_mode() const                                          = 0;
    [[nodiscard]] virtual asio::io_context& io_context()                                                        = 0;
};

}  // namespace lirasdk
