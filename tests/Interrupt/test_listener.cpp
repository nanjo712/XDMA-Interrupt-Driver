#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Interrupt/Listener.hpp"
#include "Interrupt/IController.hpp"
#include <sys/socket.h>
#include <fcntl.h>
#include <asio.hpp>

using namespace testing;

namespace Interrupt {

class MockController : public IController {
public:
    MOCK_METHOD(Index, getBankIndex, (uint32_t), (const, override));
    MOCK_METHOD(void, enable, (uint32_t), (override));
    MOCK_METHOD(void, disable, (uint32_t), (override));
    MOCK_METHOD(bool, isEnabled, (uint32_t), (const, override));
    MOCK_METHOD(uint64_t, getMailbox, (uint32_t), (const, override));
    MOCK_METHOD(void, setPending, (uint32_t), (override));
    MOCK_METHOD(void, clearPending, (uint32_t), (override));
    MOCK_METHOD(uint32_t, getPendingInterrupt, (uint32_t), (const, override));
};

class ListenerTest : public Test {
protected:
    void SetUp() override {
        if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fds) != 0) {
             perror("socketpair");
        }
        // fds[0] for listener, fds[1] for trigger
        
        // Make fds[0] non-blocking as Listener expects
        int flags = fcntl(fds[0], F_GETFL, 0);
        fcntl(fds[0], F_SETFL, flags | O_NONBLOCK);
    }

    void TearDown() override {
        if (fds[0] >= 0) close(fds[0]);
        if (fds[1] >= 0) close(fds[1]);
    }
    
    std::shared_ptr<Listener> createListener() {
        std::string dummy = "dummy_path_that_does_not_exist";
        // Constructor will fail to open files and result in empty bankListeners
        auto l = std::shared_ptr<Listener>(new Listener(dummy, controller, io_context));
        
        // Inject bank listener manually
        int listener_fd = fds[0];
        fds[0] = -1; // Transferred ownership to Listener
        
        auto bl = std::make_unique<Listener::BankListener>(io_context, listener_fd, 0);
        l->bankListeners.push_back(std::move(bl));
        
        l->init();
        return l;
    }
    
    void triggerHardwareInterrupt(int bank, uint32_t val = 1) {
        if (fds[1] >= 0) {
            write(fds[1], &val, sizeof(val));
        }
    }

    asio::io_context io_context;
    NiceMock<MockController> controller;
    int fds[2] = {-1, -1};
};

TEST_F(ListenerTest, RegisterAndProcessInterrupt) {
    EXPECT_CALL(controller, getPendingInterrupt(0))
        .WillOnce(Return(1 << 5)); 

    EXPECT_CALL(controller, getMailbox(5))
        .WillOnce(Return(0xCAFEBABE));
    
    EXPECT_CALL(controller, clearPending(5))
        .Times(1);

    auto listener = createListener();

    bool handlerCalled = false;
    listener->registerHandler(5, [&](uint64_t data) {
        handlerCalled = true;
        EXPECT_EQ(data, 0xCAFEBABE);
    });

    triggerHardwareInterrupt(0);

    io_context.run_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(handlerCalled);
}

TEST_F(ListenerTest, WaitForInterrupt) {
    EXPECT_CALL(controller, getPendingInterrupt(0))
        .WillOnce(Return(1 << 10)); 
    
    EXPECT_CALL(controller, getMailbox(10))
        .WillOnce(Return(12345));
    
    EXPECT_CALL(controller, clearPending(10)).Times(1);

    auto listener = createListener();

    auto future = listener->waitForInterrupt(10);
    
    triggerHardwareInterrupt(0);

    io_context.run_for(std::chrono::milliseconds(100));

    ASSERT_EQ(future.wait_for(std::chrono::seconds(0)), std::future_status::ready);
    EXPECT_EQ(future.get(), 12345);
}

TEST_F(ListenerTest, UnregisterHandler) {
    EXPECT_CALL(controller, getPendingInterrupt(0))
        .WillOnce(Return(1 << 5));
    EXPECT_CALL(controller, getMailbox(5))
        .WillOnce(Return(0));
    EXPECT_CALL(controller, clearPending(5)).Times(1);

    auto listener = createListener();

    bool handlerCalled = false;
    listener->registerHandler(5, [&](uint64_t) { handlerCalled = true; });
    listener->unregisterHandler(5);

    triggerHardwareInterrupt(0);
    io_context.run_for(std::chrono::milliseconds(100));

    EXPECT_FALSE(handlerCalled);
}

} // namespace Interrupt
