#include "Interrupt/Listener.hpp"

#include "Interrupt/Controller.hpp"

namespace Interrupt
{
    Listener::Listener(Controller& controller, asio::io_context& io_context)
        : controller(controller), io_context(io_context)
    {
    }

    void Listener::registerHandler(uint32_t interruptNumber, Handler handler)
    {
        handlers[interruptNumber] = handler;
    }

    void Listener::unregisterHandler(uint32_t interruptNumber)
    {
        handlers[interruptNumber] = nullptr;
    }

}  // namespace Interrupt