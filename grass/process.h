#pragma once

#include "elf.h"
#include "disk.h"

enum proc_status {
    PROC_UNUSED,
    PROC_LOADING, /* allocated and wait for loading elf binary */
    PROC_READY,   /* finished loading elf and wait for first running */
    PROC_RUNNING,
    PROC_RUNNABLE,
    PROC_PENDING_SYSCALL,
    PROC_SLEEP_SYSCALL
};

#define SAVED_REGISTER_NUM  32
#define SAVED_REGISTER_SIZE SAVED_REGISTER_NUM * 4
#define SAVED_REGISTER_ADDR (void*)(EGOS_STACK_TOP - SAVED_REGISTER_SIZE)

struct process {
    int pid;
    struct syscall syscall;
    enum proc_status status;
    uint mepc, saved_registers[SAVED_REGISTER_NUM];

    /* Student's code goes here (Preemptive Scheduler | System Call). */

    /* Add new fields for scheduling metrics, MLFQ, or process sleep. */
    // 构建创建时间， 响应时间（创建到调度的时间）， 在cpu上运行的时间, 上次运行的开始时间
    ulonglong create_time, response_time, cpu_time, last_time;
    int schedule_num;           // 调度的次数
    ulonglong wake_time;
    /* Student's code ends here. */
};

ulonglong mtime_get();
void core_set_idle(uint);

#define MAX_NPROCESS 16
int proc_alloc();
void proc_free(int);
void proc_set_ready(int);
void proc_set_running(int);
void proc_set_runnable(int);
void proc_set_pending(int);

void proc_coresinfo();
void proc_sleep(int pid, uint usec);