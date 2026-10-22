#pragma once

#include <cstdint>
#include <mutex>
#include "lirasdk_export.h"

namespace lirasdk {

class LIRASDK_TEST_EXPORT LiraProtocolChannels {
 public:
    // Defined in the .cpp so that a shared-library build has exactly one
    // channel allocator. lira_protocol_get_channel_status()'s m_lira_protocol_status array
    // is a single global provided by liblirasdk and shared with anything linking
    // it, so a test binary with its own allocator would hand out channel 0
    // again and race the library's own packing on that shared array.
    static LiraProtocolChannels& Instance();

    // delete copy and move constructors and assign operators
    LiraProtocolChannels(LiraProtocolChannels const&) = delete;             // Copy construct
    LiraProtocolChannels(LiraProtocolChannels&&)      = delete;             // Move construct
    LiraProtocolChannels& operator=(LiraProtocolChannels const&) = delete;  // Copy assign
    LiraProtocolChannels& operator=(LiraProtocolChannels&&) = delete;       // Move assign

    LiraProtocolChannels();
    ~LiraProtocolChannels() = default;

    /**
     * Check out a free channel and mark it as used.
     *
     * @param new_channel: the channel assigned to use
     * @return true if a free channel was available.
     */
    bool checkout_free_channel(uint8_t& new_channel);

    /**
     * Check a used channel back in and mark it as free again.
     *
     * @param used_channel: the channel that is returned
     */
    void checkin_used_channel(uint8_t used_channel);

    static constexpr uint8_t get_max_channels() { return MAX_CHANNELS; }

 private:
    // It is not clear what the limit of the number of channels is, except UINT8_MAX.
    static constexpr uint8_t MAX_CHANNELS = 32;

    bool       _channels_used[MAX_CHANNELS];
    std::mutex _channels_used_mutex;
};

}  // namespace lirasdk
