#include "Interrupt/Listener.hpp"

#include <cerrno>
#include <iostream>

#include "Interrupt/Controller.hpp"

namespace Interrupt
{
    Listener::Listener(Controller& controller, asio::io_context& io_context)
        : controller(controller), io_context(io_context)
    {
        for (uint32_t i = 0; i < 16; ++i)
        {
            std::string path = "/dev/xdma0_events_" + std::to_string(i);
            int fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);

            if (fd >= 0)
            {
                bankListeners.emplace_back(
                    std::make_unique<BankListener>(io_context, fd, i));
                start(*bankListeners.back());
            }
            else
            {
                break;
            }
        }
    }

    Listener::~Listener()
    {
        for (auto& bl : bankListeners)
        {
            if (bl->descriptor.is_open())
            {
                if (std::error_code ec; bl->descriptor.close(ec))
                {
                    std::cerr << "Error closing event node " << bl->bank << ": "
                              << ec.message() << std::endl;
                }
            }
        }
    }

    void Listener::start(BankListener& listener)
    {
        listener.descriptor.async_read_some(
            asio::buffer(&listener.irq_buf, 4),
            [this, &listener](auto& e, auto bytes_transferred)
            {
                if (!e)
                {
                    processInterrupt(listener.bank);
                    start(listener);
                }
                else if (e != asio::error::operation_aborted)
                {
                    std::cerr << "Error reading event node " << listener.bank
                              << ": " << e.message() << std::endl;
                }
            });
    }

    void Listener::processInterrupt(uint32_t bank) const
    {
        uint32_t pending_bits = controller.getPendingInterrupt(bank);
        for (uint32_t bit = 0; bit < 32; ++bit)
        {
            if (pending_bits & (1u << bit))
            {
                uint32_t irq_id = bank * 32 + bit;
                uint64_t mailbox_data = controller.getMailbox(irq_id);
                {
                    std::shared_lock lock(mutex);
                    if (irq_id < handlers.size() && handlers[irq_id])
                    {
                        try
                        {
                            handlers[irq_id](mailbox_data);
                        }
                        catch (const std::exception& e)
                        {
                            std::cerr << "Exception in interrupt handler for "
                                      << irq_id << ": " << e.what()
                                      << std::endl;
                        }
                    }
                }
                controller.clearPending(irq_id);
            }
        }
    }

    void Listener::registerHandler(uint32_t interruptNumber, Handler handler)
    {
        if (interruptNumber >= handlers.size())
        {
            throw std::out_of_range("Interrupt number out of bounds");
        }
        std::unique_lock lock(mutex);
        handlers[interruptNumber] = handler;
    }

    void Listener::unregisterHandler(uint32_t interruptNumber)
    {
        if (interruptNumber >= handlers.size())
        {
            throw std::out_of_range("Interrupt number out of bounds");
        }
        std::unique_lock lock(mutex);
        handlers[interruptNumber] = nullptr;
    }

}  // namespace Interrupt