/*
 * (C) 2025, Cornell University
 * All rights reserved.
 *
 * Description: functions for interrupts
 * Initialize the trap entry, enable interrupts, and reset the timer
 */

#include "egos.h"

#define MTIME_BASE    (CLINT_BASE + 0xBFF8)
#define MTIMECMP_BASE (CLINT_BASE + 0x4000)
#define QUANTUM       (earth->platform == QEMU ? 500000UL : 50000000UL)

ulonglong mtime_get() {
    uint low, high;
    do {
        high = REGW(MTIME_BASE, 4);
        low  = REGW(MTIME_BASE, 0);
    } while (REGW(MTIME_BASE, 4) != high);

    return (((ulonglong)high) << 32) | low;
}

static void mtimecmp_set(ulonglong time, uint core_id) {
    REGW(MTIMECMP_BASE, core_id * 8 + 4) = 0xFFFFFFFF;
    REGW(MTIMECMP_BASE, core_id * 8 + 0) = (uint)time;
    REGW(MTIMECMP_BASE, core_id * 8 + 4) = (uint)(time >> 32);
}

static void timer_reset(uint core_id) {
    mtimecmp_set(mtime_get() + QUANTUM, core_id);
}

void intr_init(uint core_id) {
    /* Initialize the timer */
    earth->timer_reset = timer_reset;
    mtimecmp_set(0x0FFFFFFFFFFFFFFFUL, core_id);

    /* Setup the interrupt/exception handling entry */
    void trap_entry(); /* (see grass/kernel.s) */
    asm("csrw mtvec, %0" ::"r"(trap_entry));
    INFO("Use direct mode and put the address of the trap_entry into mtvec");

    /* Enable timer interrupt */
    asm("csrw mip, %0" ::"r"(0));
    asm("csrs mie, %0" ::"r"(0x80));
    asm("csrs mstatus, %0" ::"r"(0x88));
}
