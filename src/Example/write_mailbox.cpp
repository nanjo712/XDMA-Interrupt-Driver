#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

#include <iostream>

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "用法: ./write64 <偏移地址> <64位十六进制值>" << std::endl;
        std::cout << "示例: sudo ./write64 0x1020 0xDEADBEEFCAFEBABE"
                  << std::endl;
        return -1;
    }

    uint32_t offset = std::stoul(argv[1], nullptr, 16);
    uint64_t value = std::stoull(argv[2], nullptr, 16);

    int fd = open("/dev/xdma0_user", O_RDWR | O_SYNC);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }

    // 映射 64KB 空间
    void* map_base =
        mmap(NULL, 65536, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map_base == MAP_FAILED)
    {
        perror("mmap");
        return -1;
    }

    // 执行 64 位原子写入
    volatile uint64_t* reg_ptr =
        (volatile uint64_t*)((uint8_t*)map_base + offset);
    *reg_ptr = value;

    std::cout << "成功向 [0x" << std::hex << offset << "] 写入 64 位值: 0x"
              << value << std::dec << std::endl;

    munmap(map_base, 65536);
    close(fd);
    return 0;
}