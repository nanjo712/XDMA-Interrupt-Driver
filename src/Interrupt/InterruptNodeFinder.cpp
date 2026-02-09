#include "Interrupt/InterruptNodeFinder.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <string>

namespace Interrupt
{
    InterruptNodeList InterruptNodeFinder::find(
        const std::string& device_prefix)
    {
        InterruptNodeList fds;
        for (uint32_t i = 0; i < 16; ++i)
        {
            std::string path = device_prefix + std::to_string(i);
            int fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
            if (fd >= 0)
            {
                fds.emplace_back(ScopedFd(fd), i);
            }
            else
            {
                break;
            }
        }
        return fds;
    }
}  // namespace Interrupt
