#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace Interrupt
{
    class ScopedFd
    {
       public:
        explicit ScopedFd(int fd);
        ~ScopedFd();

        ScopedFd(const ScopedFd&) = delete;
        ScopedFd(ScopedFd&& other);

        int get() const;
        int release();

       private:
        int fd_;
    };

    using InterruptNodeList = std::vector<std::pair<ScopedFd, uint32_t>>;
}  // namespace Interrupt