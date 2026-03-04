#include <asio.hpp>
#include <iostream>

#include "Interrupt/Controller.hpp"
#include "Interrupt/InterruptNodeFinder.hpp"
#include "Interrupt/Listener.hpp"

int main()
{
    asio::io_context io_context;
    Interrupt::Controller controller(0x10000000, 0x1000, 1);
    auto fds = Interrupt::InterruptNodeFinder::find("/dev/xdma0_events_");
    std::cout << "Found " << fds.size() << " interrupt nodes." << std::endl;
    auto listener =
        Interrupt::Listener::create(std::move(fds), controller, io_context);
    listener->registerHandler(
        0,
        [](uint64_t data)
        {
            std::cout << "Interrupt 0 received with mailbox data: " << data
                      << std::endl;
        });
    auto future = listener->waitForInterrupt(1);
    std::cout << "Waiting for interrupt 1..." << std::endl;
    std::cout << "Interrupt 1 received with mailbox data: " << future.get()
              << std::endl;
    io_context.run();
    return 0;
}