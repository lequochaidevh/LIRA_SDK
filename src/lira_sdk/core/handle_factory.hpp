#pragma once

#include <cstdint>
#include <atomic>
#include "lirasdk.hpp"  // Ensure it links to your Handle definition

namespace lirasdk {

// Define the template placeholder expected by Handle's friend declaration
template <typename... Args>
class FakeHandle {};

template <typename... Args>
class HandleFactory {
 public:
    HandleFactory()  = default;
    ~HandleFactory() = default;

    // Non-copyable for atomic thread safety
    HandleFactory(const HandleFactory&) = delete;
    HandleFactory& operator=(const HandleFactory&) = delete;

    /**
     * @brief Generates a strictly unique, thread-safe Handle object.
     * Bypasses Handle's private constructor restriction via friend status.
     */
    Handle<Args...> make_handle() {
        // Increment the atomic counter safely across threads
        uint64_t next_id = _current_id++;

        // Construct the handle explicitly using C++ list-initialization
        // to match aggregate structure permissions granted to friend classes
        return Handle<Args...>{next_id};
    }

 private:
    // Starts tracking handles from index 1 (0 represents an invalid Handle)
    std::atomic<uint64_t> _current_id{1};
};

}  // namespace lirasdk
