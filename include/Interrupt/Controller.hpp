#pragma once

#include <cstdint>
#include <vector>

#include "../Hardware/InterruptRegisters.hpp"
#include "IController.hpp"

namespace Interrupt
{
    class Controller : public IController
    {
       private:
        std::vector<controlRegisters*> registers;
        std::vector<uint64_t*> mailboxes;

       public:
        Controller(uintptr_t base, uint32_t offset, uint32_t totalBanks);
        Index getBankIndex(uint32_t interruptNumber) const override;
        void enable(uint32_t interruptNumber) override;
        void disable(uint32_t interruptNumber) override;
        bool isEnabled(uint32_t interruptNumber) const override;
        uint64_t getMailbox(uint32_t interruptNumber) const override;
        void setMailbox(uint32_t interruptNumber, uint64_t value) override;
        void setPending(uint32_t interruptNumber) override;
        void clearPending(uint32_t interruptNumber) override;
        uint32_t getPendingInterrupt(uint32_t bank) const override;
        void cleanPendingBank(uint32_t bank, uint32_t pending) override;
    };
}  // namespace Interrupt
