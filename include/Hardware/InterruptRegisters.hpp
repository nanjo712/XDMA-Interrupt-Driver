#pragma once

#include <cstdint>

struct controlRegisters
{
    const volatile uint32_t status;
    const volatile uint32_t enable;
    volatile uint32_t status_set;
    volatile uint32_t status_clear;
    volatile uint32_t enable_set;
    volatile uint32_t enable_clear;
    const volatile uint32_t reserved[2];
};
