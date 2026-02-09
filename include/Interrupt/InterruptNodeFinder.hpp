#pragma once

#include <string>

#include "ScopedFd.hpp"

namespace Interrupt
{
    class InterruptNodeFinder
    {
       public:
        static InterruptNodeList find(const std::string& device_prefix);
    };
}  // namespace Interrupt
