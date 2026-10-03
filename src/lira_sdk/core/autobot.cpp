#include "autobot.hpp"

namespace lirasdk {

std::ostream& operator<<(std::ostream& str, const AutoBot& autobot) {
    switch (autobot) {
        case AutoBot::Base:
            return str << "Base";
        case AutoBot::Simulation:
            return str << "Simulation";
        default:
            return str << "Unknown";
    }
}

}  // namespace lirasdk
