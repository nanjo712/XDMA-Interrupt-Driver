#include "Interrupt/ScopedFd.hpp"

#include <unistd.h>

namespace Interrupt
{
    ScopedFd::ScopedFd(int fd) : fd_(fd) {}

    ScopedFd::~ScopedFd()
    {
        if (fd_ >= 0)
        {
            ::close(fd_);
        }
    }

    ScopedFd::ScopedFd(ScopedFd&& other) : fd_(other.fd_) { other.fd_ = -1; }

    int ScopedFd::get() const { return fd_; }

    int ScopedFd::release()
    {
        int temp = fd_;
        fd_ = -1;
        return temp;
    }
}  // namespace Interrupt