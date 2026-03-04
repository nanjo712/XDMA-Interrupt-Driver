#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/socket.h>

#include <asio.hpp>
#include <memory>

#include "Interrupt/IController.hpp"
#include "Interrupt/Listener.hpp"
#include "Interrupt/ScopedFd.hpp"

using namespace testing;

namespace Interrupt
{

    class MockController : public IController
    {
       public:
        MOCK_METHOD(Index, getBankIndex, (uint32_t), (const, override));
        MOCK_METHOD(void, enable, (uint32_t), (override));
        MOCK_METHOD(void, disable, (uint32_t), (override));
        MOCK_METHOD(bool, isEnabled, (uint32_t), (const, override));
        MOCK_METHOD(uint64_t, getMailbox, (uint32_t), (const, override));
        MOCK_METHOD(void, setPending, (uint32_t), (override));
        MOCK_METHOD(void, clearPending, (uint32_t), (override));
        MOCK_METHOD(uint32_t, getPendingInterrupt, (uint32_t),
                    (const, override));
        MOCK_METHOD(void, cleanPendingBank, (uint32_t, uint32_t), (override));
    };

    class ListenerTest : public Test
    {
       protected:
        void SetUp() override
        {
            int raw_fds[2] = {-1, -1};
            if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, raw_fds) != 0)
            {
                perror("socketpair");
                return;
            }

            int flags = fcntl(raw_fds[0], F_GETFL, 0);
            fcntl(raw_fds[0], F_SETFL, flags | O_NONBLOCK);

            fds[0] = std::make_unique<ScopedFd>(raw_fds[0]);
            fds[1] = std::make_unique<ScopedFd>(raw_fds[1]);
        }

        void TearDown() override {}

        std::shared_ptr<Listener> createListener(int bank_id = 0)
        {
            InterruptNodeList nodes;
            if (fds[0])
            {
                nodes.emplace_back(std::move(*fds[0]), bank_id);
            }
            return Listener::create(std::move(nodes), controller, io_context);
        }

        void triggerHardwareInterrupt(uint32_t val = 1)
        {
            if (fds[1] && fds[1]->get() >= 0)
            {
                ssize_t n = write(fds[1]->get(), &val, sizeof(val));
                (void)n;
            }
        }

        asio::io_context io_context;
        NiceMock<MockController> controller;
        std::array<std::unique_ptr<ScopedFd>, 2> fds;
    };

    TEST_F(ListenerTest, RegisterAndProcessInterrupt)
    {
        EXPECT_CALL(controller, getPendingInterrupt(0))
            .WillOnce(Return(1 << 5));

        EXPECT_CALL(controller, getMailbox(5)).WillOnce(Return(0xCAFEBABE));

        EXPECT_CALL(controller, cleanPendingBank(0, 1 << 5)).Times(1);

        auto listener = createListener();

        bool handlerCalled = false;
        listener->registerHandler(5,
                                  [&](uint64_t data)
                                  {
                                      handlerCalled = true;
                                      EXPECT_EQ(data, 0xCAFEBABE);
                                  });

        triggerHardwareInterrupt(0);

        io_context.run_for(std::chrono::milliseconds(100));

        EXPECT_TRUE(handlerCalled);
    }

    TEST_F(ListenerTest, WaitForInterrupt)
    {
        EXPECT_CALL(controller, getPendingInterrupt(0))
            .WillOnce(Return(1 << 10));

        EXPECT_CALL(controller, getMailbox(10)).WillOnce(Return(12345));

        EXPECT_CALL(controller, cleanPendingBank(0, 1 << 10)).Times(1);

        auto listener = createListener();

        auto future = listener->waitForInterrupt(10);

        triggerHardwareInterrupt(0);

        io_context.run_for(std::chrono::milliseconds(100));

        ASSERT_EQ(future.wait_for(std::chrono::seconds(0)),
                  std::future_status::ready);
        EXPECT_EQ(future.get(), 12345);
    }

    TEST_F(ListenerTest, UnregisterHandler)
    {
        EXPECT_CALL(controller, getPendingInterrupt(0))
            .WillOnce(Return(1 << 5));
        EXPECT_CALL(controller, getMailbox(5)).WillOnce(Return(0));
        EXPECT_CALL(controller, cleanPendingBank(0, 1 << 5)).Times(1);

        auto listener = createListener();

        bool handlerCalled = false;
        listener->registerHandler(5, [&](uint64_t) { handlerCalled = true; });
        listener->unregisterHandler(5);

        triggerHardwareInterrupt(0);
        io_context.run_for(std::chrono::milliseconds(100));

        EXPECT_FALSE(handlerCalled);
    }

}  // namespace Interrupt
