/*
 * (C) 2025, Cornell University
 * All rights reserved.
 *
 * Description: grass layer initialization
 * Spawn the first process, GPID_PROCESS (pid=1).
 */

#include "egos.h"
#include "syscall.h"
#include "process.h"

static void sys_proc_read(uint block_no, char* dst) {
    earth->disk_read(SYS_PROC_EXEC_START + block_no, 1, dst);
}

// void sys_send_my(int receiver, char* msg, uint size) {
//     INFO("sys_send_my");
//     sys_send(receiver, msg, size);
// }

void grass_entry() {
    SUCCESS("Enter the grass layer");

    /* Initialize the grass interface functions */
    grass->proc_free      = proc_free;
    grass->proc_alloc     = proc_alloc;
    grass->proc_set_ready = proc_set_ready;
    grass->sys_send       = sys_send;
    grass->sys_recv       = sys_recv;
    /* Student's code goes here (System Call | Multicore & Locks). */

    /* Initialize the grass interface functions for process sleep
     * and displaying multicore process information. */
    grass->sys_sleep = proc_sleep;    
    /* Student's code ends here. */

    /* Load the first system server GPID_PROCESS */
    INFO("Load kernel process #%d: sys_process", GPID_PROCESS);
    elf_load(GPID_PROCESS, sys_proc_read, 0, 0);
    proc_set_running(proc_alloc());
    earth->mmu_switch(GPID_PROCESS);
    earth->mmu_flush_cache();

    /* Jump to the entry of process GPID_PROCESS */
    uint mstatus, M_MODE = 3, U_MODE = 0;
    // INFO("earth->translation = %d", earth->translation);
    // uint GRASS_MODE = (earth->translation == SOFT_TLB) ? M_MODE : U_MODE;
    uint GRASS_MODE = M_MODE;
    asm("csrr %0, mstatus" : "=r"(mstatus));
    mstatus = (mstatus & ~(3 << 11)) | (GRASS_MODE << 11);
    asm("csrw mstatus, %0" ::"r"(mstatus));

    asm("csrw mepc, %0" ::"r"(APPS_ENTRY));
    asm("mv a0, %0" ::"r"(APPS_ARG));
    asm("mv a1, %0" ::"r"(&boot_lock));
    // INFO("GPID_PROCESS ret, mstatus : %d", mstatus);
    asm("mret");
}
