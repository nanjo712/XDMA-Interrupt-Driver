#pragma once

#include <cstdint>

namespace Interrupt
{
    class IController
    {
       public:
        struct Index
        {
            uint32_t bank;
            uint32_t bit;
        };

        virtual ~IController() = default;

        virtual Index getBankIndex(uint32_t interruptNumber) const = 0;
        virtual void enable(uint32_t interruptNumber) = 0;
        virtual void disable(uint32_t interruptNumber) = 0;
        virtual bool isEnabled(uint32_t interruptNumber) const = 0;
        virtual uint64_t getMailbox(uint32_t interruptNumber) const = 0;
        virtual void setPending(uint32_t interruptNumber) = 0;
        virtual void clearPending(uint32_t interruptNumber) = 0;
        virtual uint32_t getPendingInterrupt(uint32_t bank) const = 0;
        virtual void cleanPendingBank(uint32_t bank, uint32_t pending) = 0;
    };
}  // namespace Interrupt
