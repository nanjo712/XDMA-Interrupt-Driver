#pragma once

#include <cstdint>
#include <vector>

#include "../Hardware/InterruptRegisters.hpp"

namespace Interrupt
{
    class Controller
    {
       private:
        std::vector<controlRegisters*> registers;
        std::vector<uint64_t const*> mailboxes;
        struct Index
        {
            uint32_t bank;
            uint32_t bit;
        };

       public:
        Controller(uintptr_t base, uint32_t offset = 0x1000,
                   uint32_t totalBanks = 1);
        Index getBankIndex(uint32_t interruptNumber);
        void enable(uint32_t interruptNumber);
        void disable(uint32_t interruptNumber);
        bool isEnabled(uint32_t interruptNumber);
        uint64_t getMailbox(uint32_t interruptNumber);
        void setPending(uint32_t interruptNumber);
        void clearPending(uint32_t interruptNumber);
        uint32_t getPendingInterrupt(uint32_t bank);

       protected:
        void setMailbox(uint32_t interruptNumber, uint64_t value);
    };
}  // namespace Interrupt