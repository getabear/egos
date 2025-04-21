/*
 * (C) 2025, Cornell University
 * All rights reserved.
 *
 * Description: Kernel ≈ 2 handlers
 *   intr_entry() handles timer, keyboard, and other interrupts
 *   excp_entry() handles system calls and faults (e.g., invalid memory access)
 */

#include "egos.h"
#include "syscall.h"
#include "process.h"
#include "servers.h"
#include <string.h>

uint core_in_kernel;
uint core_to_proc_idx[NCORES + 1];
/* QEMU has cores with ID #1 .. #NCORES */
/* Arty has cores with ID #0 .. #NCORES-1 */

struct process proc_set[MAX_NPROCESS + 1];
/* proc_set[0..MAX_NPROCESS-1] are actual processes */
/* proc_set[MAX_NPROCESS] is a place holder for idle cores */

#define curr_proc_idx core_to_proc_idx[core_in_kernel]
#define curr_pid      proc_set[curr_proc_idx].pid
#define curr_status   proc_set[curr_proc_idx].status
#define curr_saved    proc_set[curr_proc_idx].saved_registers
#define CORE_IDLE     (curr_proc_idx == MAX_NPROCESS)
void core_set_idle(uint core) { core_to_proc_idx[core] = MAX_NPROCESS; }

static void intr_entry(uint);
static void excp_entry(uint);

void kernel_entry(uint mcause) {
    /* With the kernel lock, only one core can enter this point at any time */
    asm("csrr %0, mhartid" : "=r"(core_in_kernel));

    /* Save the process context */
    asm("csrr %0, mepc" : "=r"(proc_set[curr_proc_idx].mepc));
    memcpy(curr_saved, SAVED_REGISTER_ADDR, SAVED_REGISTER_SIZE);

    (mcause & (1 << 31)) ? intr_entry(mcause & 0x3FF) : excp_entry(mcause);

    /* Restore the process context */
    asm("csrw mepc, %0" ::"r"(proc_set[curr_proc_idx].mepc));
    memcpy(SAVED_REGISTER_ADDR, curr_saved, SAVED_REGISTER_SIZE);
    // INFO("pid = %d, return addr = 0x%x, mepc = 0x%x", 
    // curr_pid, proc_set[curr_proc_idx].saved_registers[27], proc_set[curr_proc_idx].mepc);
}

#define INTR_ID_TIMER   7
#define EXCP_ID_ECALL_U 8
#define EXCP_ID_ECALL_M 11
static void proc_yield();
static void proc_try_syscall(struct process* proc);
static void proc_try_send(struct process* sender);

static void excp_entry(uint id) {
    if (id >= EXCP_ID_ECALL_U && id <= EXCP_ID_ECALL_M) {
        /* Copy the system call arguments from user space to the kernel */
        uint syscall_paddr = earth->mmu_translate(curr_pid, SYSCALL_ARG);
        memcpy(&proc_set[curr_proc_idx].syscall, (void*)syscall_paddr, sizeof(struct syscall));
        proc_set[curr_proc_idx].mepc += 4;
        proc_set[curr_proc_idx].syscall.status = PENDING;
        proc_set_pending(curr_pid);
        proc_try_syscall(&proc_set[curr_proc_idx]);
        proc_yield();
        // INFO("cur pid = 0x%x", curr_pid);
        return;
    }
    /* Student's code goes here (System Call & Protection). */

    /* Kill the process if curr_pid is a user application. */
    else{
        uint fault_addr = 0;
        uint page_table = 0;
        asm volatile("csrr %0, mtval" : "=r"(fault_addr));
        asm("csrr %0, satp" :"=r"(page_table));
        page_table = (page_table & 0xFFFFF) << 12;
        CRITICAL("process pid = %d: error code = %d, fault addr = 0x%x, mepc = 0x%x, page_table = 0x%x, exit with code -1!!!",
                curr_pid, id, fault_addr, proc_set[curr_proc_idx].mepc, page_table);
        // 此处一定要调用内核空间的函数，用户空间函数可能同一个地址对应的不同的物理地址
        struct proc_request req;
        req.type = PROC_EXIT;
        proc_set[curr_proc_idx].syscall.receiver = GPID_PROCESS;        // 指定接收者
        memcpy(proc_set[curr_proc_idx].syscall.content, &req, sizeof(req));
        proc_set_pending(curr_pid);
        proc_try_send(&proc_set[curr_proc_idx]);            // 发送消息给接收者
        proc_yield();                                       // 唤醒接收者


        /*  下面代码有问题， 两次进入异常会使得curr_pid进程上下文丢失 */
        // struct syscall *syscall_paddr = (struct syscall *)earth->mmu_translate(curr_pid, SYSCALL_ARG);
        // INFO("syscall_paddr = 0x%x", syscall_paddr);
        // syscall_paddr->receiver = GPID_PROCESS;
        // syscall_paddr->type = SYS_SEND;
        // memcpy(syscall_paddr->content, &req, sizeof(req));
        // asm("ecall"); 
// 照理说虽然会导致curr_pid进程上下文丢失，但是不会影响进程GPID_PROCESS恢复执行，为什么会出错呢？
// 因为当前我们是通过系统调用eacll进入的，处于机器模式（11），使用ecall时，硬件会自动记录当前的CPU模式，以便返回时恢复
// 再次使用ecall时，机器处于机器模式，因此返回时会处于机器模式，机器模式没有虚拟地址，返回时直接访问用户空间0x80400000，从而造成错误。
// 因此在返回时将mstatus寄存器的状态改为用户模式即可让上年的代码无问题。
// [INFO] process pid = 7: error code = 15, fault addr = 0x0, mepc = 0x8040058c, page_table = 0x80830000, exit with code -1!!!
// [INFO] proc_set saddr = 0x8000ed48, proc_set eaddr = 0x80065c18
// [INFO] process pid = 1: error code = 1, fault addr = 0x0, mepc = 0x0, page_table = 0x80808000, exit with code -1!!!
// [INFO] proc_set saddr = 0x8000ed48, proc_set eaddr = 0x80065c18
// [INFO] process pid = 7: error code = 12, fault addr = 0x80003104, mepc = 0x80003104, page_table = 0x80830000, exit with code -1!!!
// [INFO] proc_set saddr = 0x8000ed48, proc_set eaddr = 0x80065c18

        return;
    }
    /* Student's code ends here. */
    FATAL("excp_entry: kernel got exception %d", id);
}

static void intr_entry(uint id) {
    if (id == INTR_ID_TIMER) return proc_yield();

    /* Student's code goes here (Ethernet & TCP/IP). */

    /* Handle the Ethernet device interrupt. */

    /* Student's code ends here. */
    
    FATAL("excp_entry: kernel got interrupt %d", id);
}

static void proc_yield() {
    if (!CORE_IDLE && curr_status == PROC_RUNNING) proc_set_runnable(curr_pid);

    /* Student's code goes here (Preemptive Scheduler | System Call). */
    struct process *p = &proc_set[curr_proc_idx];
    // 得到当前时间
    ulonglong now = mtime_get();
    if(p->last_time != 0){
        // 记录cpu时间
        p->cpu_time +=  mtime_get() - p->last_time;
        // INFO("p->cputime : %d", p->cpu_time);
    }
   
    /* Replace the loop below to find the next process to schedule with MLFQ.
     * Do not schedule a process that should still be sleeping at this time. */

    /* Find the next process to run */
    int next_idx = MAX_NPROCESS;
    for (uint i = 1; i <= MAX_NPROCESS; i++) {
        struct process* p = &proc_set[(curr_proc_idx + i) % MAX_NPROCESS];
        if (p->status == PROC_PENDING_SYSCALL) proc_try_syscall(p);
        // 唤醒睡眠的进程
        // if(p->status == PROC_SLEEP_SYSCALL && now >= p->wake_time){
        //     p->status = PROC_RUNNABLE;
        // }

        if (p->status == PROC_READY || p->status == PROC_RUNNABLE) {
            next_idx = (curr_proc_idx + i) % MAX_NPROCESS;
            break;
        }
    }
    p = &proc_set[curr_proc_idx];
    p->last_time = now;
    // 下个被调度的程序和当前的不一样
    if(next_idx != curr_proc_idx){
        p->schedule_num += 1;
    }
    // 如果第一次被调用，记录响应时间
    if(p->response_time == 0){
        p->response_time = now - p->create_time;
    }


    /* Measure and record scheduling metrics for the current process before it
     * yields; Measure and record scheduling metrics for the next process. */

    /* Student's code ends here. */
    
    curr_proc_idx = next_idx;
    earth->timer_reset(core_in_kernel);
   
    if (CORE_IDLE) {
        /* Student's code goes here (System Call | Multicore & Locks) */

        /* Release the kernel lock; Enable interrupts by modifying mstatus;
         * Wait for a timer interrupt with the wfi instruction. */
         char *status[] = { "PROC_UNUSED",
                            "PROC_LOADING", /* allocated and wait for loading elf binary */
                            "PROC_READY",   /* finished loading elf and wait for first running */
                            "PROC_RUNNING",
                            "PROC_RUNNABLE",
                            "PROC_PENDING_SYSCALL",
                            "PROC_SLEEP_SYSCALL"};
        for(int i = 0; i < MAX_NPROCESS; i++){
            if(proc_set[i].status != PROC_UNUSED){
                INFO("pid = %d, status = %s", proc_set[i].pid, status[proc_set[i].status]);
            }
        }
        /* Student's code ends here. */
        FATAL("proc_yield: no process to run on core %d", core_in_kernel);
    }
    earth->mmu_switch(curr_pid);
    earth->mmu_flush_cache();

    /* Student's code goes here (Protection | Multicore & Locks). */

    /* Modify mstatus.MPP to enter machine or user mode after mret. */
    // if(curr_pid < GPID_USER_START){
    //     asm("csrs mstatus, %0"::"r"(3<<11)); // 将mstatus.MPP设置为11(机器模式)
    // }else{
    //     asm("csrc mstatus, %0"::"r"(3<<11)); // 将mstatus.MPP设置为00(用户模式)
    // }
    if(earth->translation == PAGE_TABLE){
         asm("csrc mstatus, %0"::"r"(3<<11)); // 将mstatus.MPP设置为00(用户模式)
    }
        
    /* Setup the entry point for a newly created process */
    if (curr_status == PROC_READY) {
        /* Set argc, argv and initial program counter */
        curr_saved[0]                = APPS_ARG;
        curr_saved[1]                = APPS_ARG + 4;
        proc_set[curr_proc_idx].mepc = APPS_ENTRY;
    }

    
    proc_set_running(curr_pid);
}

static void proc_try_send(struct process* sender) {
    for (uint i = 0; i < MAX_NPROCESS; i++) {
        struct process* dst = &proc_set[i];
        if (dst->pid == sender->syscall.receiver &&
            dst->status != PROC_UNUSED) {
            /* Return if dst is not receiving or not taking msg from sender. */
            if (!(dst->syscall.type == SYS_RECV &&
                  dst->syscall.status == PENDING)){
                    return;
                }

            if (!(dst->syscall.sender == GPID_ALL ||
                  dst->syscall.sender == sender->pid)){
                    return;
                }
            // INFO("proc_try_send, recv pid = %d, sender->pid = %d", dst->pid, sender->pid);              
            dst->syscall.status = DONE;
            dst->syscall.sender = sender->pid;
            /* Copy the system call arguments within the kernel PCB. */
            memcpy(dst->syscall.content, sender->syscall.content,
                   SYSCALL_MSG_LEN);
            
            return;
        }
    }
    FATAL("proc_try_send: unknown receiver pid=%d", sender->syscall.receiver);
}

static void proc_try_recv(struct process* receiver) {
    if (receiver->syscall.status == PENDING) return;

    /* Copy the system call results from the kernel back to user space. */
    uint syscall_paddr = earth->mmu_translate(receiver->pid, SYSCALL_ARG);
    memcpy((void*)syscall_paddr, &receiver->syscall, sizeof(struct syscall));
    // INFO("proc_try_recv, recv pid = %d, sender->pid = %d, syscall_paddr = 0x%x", receiver->pid, receiver->syscall.sender, syscall_paddr); 

    /* Set the receiver and sender back to RUNNABLE. */
    // INFO("proc_try_recv runable, pid1 = %d, pid2 = %d", receiver->pid, receiver->syscall.sender);
    proc_set_runnable(receiver->pid);
    proc_set_runnable(receiver->syscall.sender);
}

static void proc_try_syscall(struct process* proc) {
    switch (proc->syscall.type) {
    case SYS_RECV:
        proc_try_recv(proc);
        break;
    case SYS_SEND:
        proc_try_send(proc);
        break;
    default:
        FATAL("proc_try_syscall: unknown syscall type=%d", proc->syscall.type);
    }
}

void proc_sleep(int pid, uint usec) {
    /* Student's code goes here (System Call & Protection). */

    /* Update the struct process of process pid for process sleep. */
    INFO("proc_sleep pid: %d, usec : %d", pid, usec);
    ulonglong now = mtime_get();

    // 设置唤醒时间
    for(int i = 0; i < MAX_NPROCESS; i++){
        if(proc_set[i].pid == pid){
            proc_set[i].wake_time = now + usec;
            proc_set[i].status = PROC_SLEEP_SYSCALL;
            return;
        }
    }
    // proc_yield();
    /* Student's code ends here. */
}

void proc_coresinfo() {
    /* Student's code goes here (Multicore & Locks). */

    /* Print the pid of the process running on each core; Add this
     * function into the grass interface so that shell can invoke it. */

    /* Student's code ends here. */
}
