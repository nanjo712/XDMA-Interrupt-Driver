#include <sys/mman.h>

#include <asio.hpp>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <thread>

#include "Interrupt/Controller.hpp"
#include "Interrupt/InterruptNodeFinder.hpp"
#include "Interrupt/Listener.hpp"

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "usage: " << argv[0] << "<MAP_SIZE> <OFFSET>" << std::endl;
        return -1;
    }
    size_t MAP_SIZE = std::stoul(argv[1], nullptr, 0);
    size_t OFFSET = std::stoul(argv[2], nullptr, 0);
    int fd = open("/dev/xdma0_user", O_RDWR | O_SYNC);
    if (fd < 0)
    {
        perror("无法打开 /dev/xdma0_user");
        return -1;
    }
    void* map_base =
        mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map_base == MAP_FAILED)
    {
        perror("mmap 映射失败");
        close(fd);
        return -1;
    }

    asio::io_context io_context;
    Interrupt::Controller controller((uintptr_t)map_base, OFFSET, 1);
    auto fds = Interrupt::InterruptNodeFinder::find("/dev/xdma0_events_", 1);
    std::cout << "Found " << fds.size() << " interrupt nodes." << std::endl;
    auto listener =
        Interrupt::Listener::create(std::move(fds), controller, io_context);
    std::cout << "Registering handler for interrupt 0..." << std::endl;
    listener->registerHandler(0,
                              [](uint64_t data)
                              {
                                  std::cout << "[Mailbox " << 0 << "] 数据: 0x"
                                            << std::hex << std::setw(16)
                                            << std::setfill('0') << data
                                            << std::dec << std::endl;
                              });
    std::cout << "Waiting for interrupts..." << std::endl;
    std::thread wait_for_interrupts(
        [&listener]()
        {
            auto fut = listener->waitForInterrupt(1);
            uint64_t data = fut.get();
            std::cout << "[waitForInterrupt] 收到中断 1，数据: 0x" << std::hex
                      << std::setw(16) << std::setfill('0') << data << std::dec
                      << std::endl;
        });
    io_context.run();
    wait_for_interrupts.join();
    return 0;
}