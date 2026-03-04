#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

#include <iostream>
#include <string>

// 基于你之前的配置：
// Vivado AXI 基地址 0x44A00000, Chisel 模块偏移 0x1000
// 软件相对于 /dev/xdma0_user 的偏移量是 0x1000
#define BASE_ADDR 0x1000
#define REG_STATUS_SET (BASE_ADDR + 0x08)
#define REG_ENABLE_SET (BASE_ADDR + 0x10)

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "用法: " << argv[0] << " <中断ID (0-31)>" << std::endl;
        return -1;
    }

    // 1. 解析中断 ID
    int irq_id = std::stoi(argv[1]);
    if (irq_id < 0 || irq_id > 31)
    {
        std::cerr << "错误: 中断 ID 超出范围 (0-31)" << std::endl;
        return -1;
    }

    // 2. 计算位掩码 (Bitmask)
    // 例如 ID=1, mask = 0x00000002
    uint32_t mask = (1u << irq_id);

    // 3. 打开设备节点
    int fd = open("/dev/xdma0_user", O_RDWR | O_SYNC);
    if (fd < 0)
    {
        perror("无法打开 /dev/xdma0_user，请确认驱动已加载并具有 sudo 权限");
        return -1;
    }

    // 4. 先使能该中断位 (ENABLE_SET)
    // 确保 (status & enable) 逻辑可以通向 events0
    if (pwrite(fd, &mask, sizeof(mask), REG_ENABLE_SET) != sizeof(mask))
    {
        perror("写入 ENABLE_SET 失败");
        close(fd);
        return -1;
    }

    // 5. 触发该中断位 (STATUS_SET)
    if (pwrite(fd, &mask, sizeof(mask), REG_STATUS_SET) != sizeof(mask))
    {
        perror("写入 STATUS_SET 失败");
        close(fd);
        return -1;
    }

    std::hex(std::cout);
    std::cout << "成功触发中断 ID: " << std::dec << irq_id << " (位掩码: 0x"
              << std::hex << mask << ")" << std::endl;

    close(fd);
    return 0;
}