#include "Interrupt/Listener.hpp"

#include <cerrno>
#include <cstdint>
#include <future>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace Interrupt
{
    Listener::Listener(InterruptNodeList fds, IController& controller,
                       asio::io_context& io_context)
        : controller(controller), io_context(io_context)
    {
        for (auto&& pair : fds)
        {
            bankListeners.emplace_back(std::make_unique<BankListener>(
                io_context, pair.first.release(), pair.second));
        }
    }

    void Listener::init()
    {
        for (auto& bl : bankListeners)
        {
            start(*bl);
        }
    }

    std::shared_ptr<Listener> Listener::create(InterruptNodeList fds,
                                               IController& controller,
                                               asio::io_context& io_context)
    {
        auto listener = std::shared_ptr<Listener>(
            new Listener(std::move(fds), controller, io_context));
        listener->init();
        return listener;
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
        auto self = shared_from_this();
        listener.descriptor.async_read_some(
            asio::buffer(&listener.irq_buf, 4),
            [this, &listener, self](auto& e, auto bytes_transferred)
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
        if (pending_bits == 0)
        {
            return;
        }
        struct Event
        {
            uint32_t irq_id;
            uint64_t mailbox_data;
        };
        Event events[32];
        uint32_t event_count = 0;
        for (uint32_t bit = 0; bit < 32; ++bit)
        {
            if (pending_bits & (1u << bit))
            {
                uint32_t irq_id = bank * 32 + bit;
                events[event_count++] =
                    Event{.irq_id = irq_id,
                          .mailbox_data = controller.getMailbox(irq_id)};
            }
        }
        controller.cleanPendingBank(bank, pending_bits);
        for (uint32_t i = 0; i < event_count; ++i)
        {
            uint32_t irq_id = events[i].irq_id;
            uint64_t mailbox_data = events[i].mailbox_data;
            std::shared_ptr<Handler> handler = nullptr;
            {
                std::shared_lock lock(mutex);
                if (irq_id < handlers.size() && handlers[irq_id])
                {
                    handler = handlers[irq_id];
                }
            }
            if (handler)
            {
                try
                {
                    (*handler)(mailbox_data);
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Exception in interrupt handler for " << irq_id
                              << ": " << e.what() << std::endl;
                }
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
        handlers[interruptNumber] = std::make_shared<Handler>(handler);
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

    std::future<uint64_t> Listener::waitForInterrupt(uint32_t interruptNumber)
    {
        auto promise = std::make_shared<std::promise<uint64_t>>();
        auto self = shared_from_this();
        registerHandler(interruptNumber,
                        [promise, self, interruptNumber](uint64_t data)
                        {
                            self->unregisterHandler(interruptNumber);
                            try
                            {
                                promise->set_value(data);
                            }
                            catch (const std::exception& e)
                            {
                                std::cerr
                                    << "Exception in interrupt handler for "
                                    << interruptNumber << ": " << e.what()
                                    << std::endl;
                            }
                        });
        return promise->get_future();
    }
}  // namespace Interrupt
