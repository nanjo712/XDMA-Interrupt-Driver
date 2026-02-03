#include "Interrupt/Controller.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstdint>

#include "Hardware/InterruptRegisters.hpp"

namespace Interrupt
{
    Controller::Controller(uintptr_t base, uint32_t offset, uint32_t totalBanks)
        : registers(totalBanks), mailboxes(totalBanks * 32)
    {
        uintptr_t mailboxBase =
            base + offset + totalBanks * sizeof(controlRegisters);
        for (uint32_t i = 0; i < totalBanks; i++)
        {
            registers[i] = reinterpret_cast<controlRegisters*>(
                base + offset + i * sizeof(controlRegisters));
            mailboxes[i] =
                reinterpret_cast<uint64_t*>(mailboxBase + i * sizeof(uint64_t));
        }
    }

    Controller::Index Controller::getBankIndex(uint32_t interruptNumber) const
    {
        return Index{
            .bank = (uint32_t)interruptNumber / 32,
            .bit = (uint32_t)interruptNumber % 32,
        };
    }

    void Controller::enable(uint32_t interruptNumber)
    {
        Index index = getBankIndex(interruptNumber);
        registers[index.bank]->enable_set = 1 << index.bit;
    }

    void Controller::disable(uint32_t interruptNumber)
    {
        Index index = getBankIndex(interruptNumber);
        registers[index.bank]->enable_clear = 1 << index.bit;
    }

    bool Controller::isEnabled(uint32_t interruptNumber) const
    {
        Index index = getBankIndex(interruptNumber);
        return registers[index.bank]->enable & (1 << index.bit);
    }

    uint64_t Controller::getMailbox(uint32_t interruptNumber) const
    {
        return *mailboxes[interruptNumber];
    }

    void Controller::setMailbox(uint32_t interruptNumber, uint64_t value)
    {
        const_cast<uint64_t&>(*mailboxes[interruptNumber]) = value;
    }

    void Controller::setPending(uint32_t interruptNumber)
    {
        Index index = getBankIndex(interruptNumber);
        registers[index.bank]->status_set = 1 << index.bit;
    }

    void Controller::clearPending(uint32_t interruptNumber)
    {
        Index index = getBankIndex(interruptNumber);
        registers[index.bank]->status_clear = 1 << index.bit;
    }

    uint32_t Controller::getPendingInterrupt(uint32_t bank) const
    {
        return registers[bank]->status;
    }
}  // namespace Interrupt
