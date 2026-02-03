#pragma once

#include <array>
#include <asio.hpp>
#include <functional>

#include "Controller.hpp"

namespace Interrupt
{
    using Handler = std::function<void(uint64_t)>;
    class Listener
    {
       private:
        Controller& controller;
        asio::io_context& io_context;
        std::array<Handler, 512>
            handlers;  // 512 interrupt handler should be enough for most system

       public:
        Listener(Controller& controller, asio::io_context& io_context);

        void registerHandler(uint32_t interruptNumber, Handler handler);
        void unregisterHandler(uint32_t interruptNumber);
    };
}  // namespace Interrupt