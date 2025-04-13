/*
 * (C) 2025, Cornell University
 * All rights reserved.
 *
 * Description: helper functions for managing processes
 */

#include "egos.h"
#include "syscall.h"
#include "process.h"

extern struct process proc_set[MAX_NPROCESS];

static void proc_set_status(int pid, enum proc_status status) {
    for (uint i = 0; i < MAX_NPROCESS; i++)
        if (proc_set[i].pid == pid) proc_set[i].status = status;
}

void proc_set_ready(int pid) { proc_set_status(pid, PROC_READY); }
void proc_set_running(int pid) { proc_set_status(pid, PROC_RUNNING); }
void proc_set_runnable(int pid) { proc_set_status(pid, PROC_RUNNABLE); }
void proc_set_pending(int pid) { proc_set_status(pid, PROC_PENDING_SYSCALL); }

void proc_print(int pid){
        struct process tmp;
        for(int i = 0; i < MAX_NPROCESS; i++){
            if(proc_set[i].pid == pid){
                tmp = proc_set[i];
            }
        }
        ulonglong now = mtime_get();
        ulonglong around_time = (now - tmp.create_time);
        tmp.cpu_time +=  now - tmp.last_time;
        // INFO("end time %llu, create time: %llu, turnaround time %llu", now, tmp.create_time, around_time);
        INFO("proc %d died after %d yields, turnaround time: %llu.%llu, response time: %llu.%llu, cpu time: %llu.%llu", pid, tmp.schedule_num,
        around_time / 1000000, around_time / 1000 % 100, tmp.response_time / 1000000, tmp.response_time / 10000 % 100, tmp.cpu_time / 1000000, tmp.cpu_time / 10000 % 100);
}


int proc_alloc() {
    /* Student's code goes here (Preemptive Scheduler | System Call). */
    
    /* Collect information (e.g., spawning time) for the new process
     * and initialize the MLFQ data structures. Initialize fields of
     * struct process added for process sleep. */

    static uint curr_pid = 0;
    for (uint i = 0; i < MAX_NPROCESS; i++)
        if (proc_set[i].status == PROC_UNUSED) {
            proc_set[i].pid    = ++curr_pid;
            proc_set[i].status = PROC_LOADING;
            proc_set[i].create_time = mtime_get();
            // INFO("create_time:%llu", proc_set[i].create_time);
            proc_set[i].last_time = 0;
            proc_set[i].cpu_time = 0;
            proc_set[i].response_time = 0;
            proc_set[i].schedule_num = 0;
            return curr_pid;
        }

    /* Student's code ends here. */
    FATAL("proc_alloc: reach the limit of %d processes", MAX_NPROCESS);
}

void proc_free(int pid) {
    /* Student's code goes here (Preemptive Scheduler). */
    
    /* Collect information (e.g., termination time) for process pid,
     * and print out scheduling metrics. Cleanup MLFQ data structures. */

    if (pid != GPID_ALL) {
        proc_print(pid);
        earth->mmu_free(pid);
        proc_set_status(pid, PROC_UNUSED);
        return;
    }

    /* Free all user applications */
    for (uint i = 0; i < MAX_NPROCESS; i++)
        if (proc_set[i].pid >= GPID_USER_START &&
            proc_set[i].status != PROC_UNUSED) {
            earth->mmu_free(proc_set[i].pid);
            proc_set[i].status = PROC_UNUSED;
        }

    /* Student's code ends here. */
}
