#include <gtest/gtest.h>
#include "Interrupt/Controller.hpp"
#include "Hardware/InterruptRegisters.hpp"
#include <vector>
#include <cstring>

// Helper class to expose protected members for testing
class TestableController : public Interrupt::Controller {
public:
    using Interrupt::Controller::Controller;
    using Interrupt::Controller::setMailbox;
};

class ControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Calculate necessary size:
        // Offset (0x1000) + Registers (Banks * sizeof(controlRegisters)) + Mailboxes (Banks * 32 * sizeof(uint64_t))
        // Let's allocate enough for 2 banks to be safe for multi-bank tests.
        // 2 banks registers: 2 * 32 bytes = 64 bytes
        // 2 banks mailboxes: 2 * 32 * 8 bytes = 512 bytes
        // Offset: 0x1000 = 4096 bytes
        // Total ~ 4700 bytes. Round up to 8192 for alignment and safety.
        memory.resize(8192, 0);
        baseAddress = reinterpret_cast<uintptr_t>(memory.data());
    }

    void TearDown() override {
        memory.clear();
    }

    std::vector<uint8_t> memory;
    uintptr_t baseAddress;
    static constexpr uint32_t OFFSET = 0x1000;
};

TEST_F(ControllerTest, Initialization) {
    Interrupt::Controller controller(baseAddress, OFFSET, 1);
    // Just ensuring it constructs without crashing
    SUCCEED();
}

TEST_F(ControllerTest, EnableInterrupt) {
    Interrupt::Controller controller(baseAddress, OFFSET, 1);
    
    // Enable interrupt 5
    controller.enable(5);

    // Verify memory
    // enable_set is at offset 0x10 (16 bytes) into controlRegisters
    // Address = base + 0x1000 + 0x10
    controlRegisters* regs = reinterpret_cast<controlRegisters*>(baseAddress + OFFSET);
    EXPECT_EQ(regs->enable_set, (1U << 5));
}

TEST_F(ControllerTest, DisableInterrupt) {
    Interrupt::Controller controller(baseAddress, OFFSET, 1);
    
    // Disable interrupt 10
    controller.disable(10);

    // Verify memory
    // enable_clear is at offset 0x14 (20 bytes) into controlRegisters
    controlRegisters* regs = reinterpret_cast<controlRegisters*>(baseAddress + OFFSET);
    EXPECT_EQ(regs->enable_clear, (1U << 10));
}

TEST_F(ControllerTest, IsEnabled) {
    Interrupt::Controller controller(baseAddress, OFFSET, 1);
    
    // Manually write to memory to simulate state
    controlRegisters* regs = reinterpret_cast<controlRegisters*>(baseAddress + OFFSET);
    // Cast away const volatile for test setup
    uint32_t* enableReg = const_cast<uint32_t*>(&regs->enable);
    *enableReg = (1U << 7);

    EXPECT_TRUE(controller.isEnabled(7));
    EXPECT_FALSE(controller.isEnabled(6));
}

TEST_F(ControllerTest, PendingOperations) {
    Interrupt::Controller controller(baseAddress, OFFSET, 1);
    
    // Set Pending
    controller.setPending(3);
    controlRegisters* regs = reinterpret_cast<controlRegisters*>(baseAddress + OFFSET);
    EXPECT_EQ(regs->status_set, (1U << 3));

    // Clear Pending
    controller.clearPending(3);
    EXPECT_EQ(regs->status_clear, (1U << 3));
}

TEST_F(ControllerTest, GetPendingInterrupt) {
    Interrupt::Controller controller(baseAddress, OFFSET, 1);
    
    controlRegisters* regs = reinterpret_cast<controlRegisters*>(baseAddress + OFFSET);
    uint32_t* statusReg = const_cast<uint32_t*>(&regs->status);
    *statusReg = 0xDEADBEEF;

    EXPECT_EQ(controller.getPendingInterrupt(0), 0xDEADBEEF);
}

TEST_F(ControllerTest, MailboxOperations) {
    TestableController controller(baseAddress, OFFSET, 1);
    
    // Calculate mailbox address for interrupt 2
    // Base + Offset + sizeof(controlRegisters) * 1 + 2 * 8
    // But let's use the controller to set it and verify retrieval
    
    uint64_t testValue = 0x1234567890ABCDEF;
    controller.setMailbox(2, testValue);
    
    EXPECT_EQ(controller.getMailbox(2), testValue);

    // Verify memory manually
    uintptr_t mailboxStart = baseAddress + OFFSET + sizeof(controlRegisters);
    uint64_t* mailboxPtr = reinterpret_cast<uint64_t*>(mailboxStart + 2 * sizeof(uint64_t));
    EXPECT_EQ(*mailboxPtr, testValue);
}

TEST_F(ControllerTest, MultiBankSupport) {
    // 2 Banks
    Interrupt::Controller controller(baseAddress, OFFSET, 2);

    // Test Bank 0 (Interrupt 31)
    controller.enable(31);
    controlRegisters* regs0 = reinterpret_cast<controlRegisters*>(baseAddress + OFFSET);
    EXPECT_EQ(regs0->enable_set, (1U << 31));

    // Test Bank 1 (Interrupt 32 -> Bank 1, Bit 0)
    controller.enable(32);
    controlRegisters* regs1 = reinterpret_cast<controlRegisters*>(baseAddress + OFFSET + sizeof(controlRegisters));
    EXPECT_EQ(regs1->enable_set, (1U << 0));

    // Test Bank 1 (Interrupt 63 -> Bank 1, Bit 31)
    controller.disable(63);
    EXPECT_EQ(regs1->enable_clear, (1U << 31));
}

TEST_F(ControllerTest, MultiBankMailbox) {
    // 2 Banks
    TestableController controller(baseAddress, OFFSET, 2);

    // Bank 0 Mailbox
    controller.setMailbox(5, 0xAAAA);
    EXPECT_EQ(controller.getMailbox(5), 0xAAAA);

    // Bank 1 Mailbox (Interrupt 35 -> Bank 1, Index 35 in flat array? No, wait)
    // The implementation of Controller says:
    // mailboxes[i] = reinterpret_cast<uint64_t*>(mailboxBase + i * sizeof(uint64_t));
    // And getMailbox(interruptNumber) returns *mailboxes[interruptNumber]
    // The loop in constructor I fixed:
    // for (uint32_t i = 0; i < totalBanks * 32; i++) ...
    // So interrupt 35 should simply be index 35.
    
    controller.setMailbox(35, 0xBBBB);
    EXPECT_EQ(controller.getMailbox(35), 0xBBBB);

    // Verify memory location for Bank 1 Mailbox
    // Mailbox Base starts after ALL register banks.
    // mailboxBase = base + offset + totalBanks * sizeof(controlRegisters)
    // Here totalBanks = 2.
    uintptr_t mailboxBase = baseAddress + OFFSET + 2 * sizeof(controlRegisters);
    
    // Interrupt 35 is the 36th mailbox (0-indexed)
    uint64_t* mailboxPtr = reinterpret_cast<uint64_t*>(mailboxBase + 35 * sizeof(uint64_t));
    EXPECT_EQ(*mailboxPtr, 0xBBBB);
}
