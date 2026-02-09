#pragma once

#include <array>
#include <asio.hpp>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <shared_mutex>
#include <string>

#include "IController.hpp"

namespace Interrupt
{
    class ListenerTest;
    
    using Handler = std::function<void(uint64_t)>;
    class Listener : public std::enable_shared_from_this<Listener>
    {
        friend class ListenerTest;
       private:
        IController& controller;
        asio::io_context& io_context;
        mutable std::shared_mutex mutex;
        std::array<std::shared_ptr<Handler>, 512>
            handlers;  // 512 interrupt handler should be enough for most system

        struct BankListener
        {
            asio::posix::stream_descriptor descriptor;
            uint32_t bank;
            uint32_t irq_buf;

            BankListener(asio::io_context& ctx, int fd, uint32_t bank)
                : descriptor(ctx, fd), bank(bank), irq_buf(0)
            {
            }
        };

        std::vector<std::unique_ptr<BankListener>> bankListeners;

        void start(BankListener& listener);
        void processInterrupt(uint32_t bank) const;
        void init();

        Listener(std::string& device_prefix, IController& controller,
                 asio::io_context& io_context);

       public:
        static std::shared_ptr<Listener> create(std::string& device_prefix,
                                                IController& controller,
                                                asio::io_context& io_context);
        ~Listener();

        void registerHandler(uint32_t interruptNumber, Handler handler);
        void unregisterHandler(uint32_t interruptNumber);

        std::future<uint64_t> waitForInterrupt(uint32_t interruptNumber);
    };
}  // namespace Interrupt