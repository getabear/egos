/*
 * (C) 2025, Cornell University
 * All rights reserved.
 *
 * Description: functions for memory management unit (MMU)
 * This MMU code contains a simple memory allocation/free mechanism
 * and two memory translation mechanisms: software TLB and page table.
 * It also contains a hardware-specific cache flushing function.
 */

#include "egos.h"
#include "disk.h"
#include "servers.h"
#include <string.h>

/* Memory allocation and free */
#define APPS_PAGES_CNT (RAM_END - APPS_PAGES_BASE) / PAGE_SIZE
struct page_info {
    int use;       /* Is this page free or allocated? */
    int pid;       /* Which process owns this page? */
    uint vpage_no; /* Which virtual page in this process maps to this physial
                      page? */
} page_info_table[APPS_PAGES_CNT];

uint mmu_alloc() {
    for (uint i = 0; i < APPS_PAGES_CNT; i++)
        if (!page_info_table[i].use) {
            page_info_table[i].use = 1;
            return i;
        }
    FATAL("mmu_alloc: no more free memory");
}



/* Software TLB Translation */
void soft_tlb_map(int pid, uint vpage_no, uint ppage_id) {
    page_info_table[ppage_id].pid      = pid;
    page_info_table[ppage_id].vpage_no = vpage_no;
}

void soft_tlb_switch(int pid) {
    static int curr_vm_pid = -1;
    if (pid == curr_vm_pid) return;

    /* Unmap curr_vm_pid from the user address space */
    for (uint i = 0; i < APPS_PAGES_CNT; i++)
        if (page_info_table[i].use && page_info_table[i].pid == curr_vm_pid)
            memcpy(PAGE_ID_TO_ADDR(i),
                   PAGE_NO_TO_ADDR(page_info_table[i].vpage_no), PAGE_SIZE);

    /* Map pid to the user address space */
    for (uint i = 0; i < APPS_PAGES_CNT; i++)
        if (page_info_table[i].use && page_info_table[i].pid == pid)
            memcpy(PAGE_NO_TO_ADDR(page_info_table[i].vpage_no),
                   PAGE_ID_TO_ADDR(i), PAGE_SIZE);

    curr_vm_pid = pid;
}

uint soft_tlb_translate(int pid, uint vaddr) {
    soft_tlb_switch(pid);
    return vaddr;
}

/* Page Table Translation
 *
 * The code below creates an identity mapping using RISC-V Sv32;
 * Read section4.3 of RISC-V manual (references/riscv-privileged-v1.10.pdf)
 *
 * mmu_map() and mmu_switch() using page tables is given to students
 * as a course project. After this project, every process should have
 * its own set of page tables. mmu_map() will modify entries in these
 * tables and mmu_switch() will modify satp (page table base register)
 */

#define USER_RWX     (0xC0 | 0x1F)
#define MAX_NPROCESS 256
static uint* root;
static uint* leaf;
static uint* pid_to_pagetable_base[MAX_NPROCESS];
/* Assume at most MAX_NPROCESS unique processes for simplicity */

void mmu_free(int pid) {
    for (uint i = 0; i < APPS_PAGES_CNT; i++)
        if (page_info_table[i].use && page_info_table[i].pid == pid)
            memset(&page_info_table[i], 0, sizeof(struct page_info));
    pid_to_pagetable_base[pid] = NULL;
}

void setup_identity_region(int pid, uint addr, uint npages, uint flag) {
    // INFO("setup_identity_region start");
    uint vpn1 = addr >> 22;

    if (root[vpn1] & 0x1) {
        /* Leaf has been allocated */
        leaf = (void*)((root[vpn1] << 2) & 0xFFFFF000);
    } else {
        /* Leaf has not been allocated */
        uint ppage_id                 = earth->mmu_alloc();
        leaf                          = (void*)PAGE_ID_TO_ADDR(ppage_id);
        // INFO("setup_identity_region leaf: 0x%x", leaf);
        page_info_table[ppage_id].pid = pid;
        memset(leaf, 0, PAGE_SIZE);
        // 为什么右移两位，因为页表记录的地址是从第10 - 31 位，共 22 位。
        // 实际我们只需要记录20位的地址，有两位多余了，所以右移动两位(>>12 <<10) 相当于右移两位
        root[vpn1] = ((uint)leaf >> 2) | 0x1;
    }
    // root[vpn1] |= flag;
    /* Setup the entries in the leaf page table */
    uint vpn0 = (addr >> 12) & 0x3FF;
    for (uint i = 0; i < npages; i++){
        leaf[vpn0 + i] = ((addr + i * PAGE_SIZE) >> 2) | flag;
    }
}

void pagetable_identity_map(int pid) {
    /* Allocate the root page table and set the page table base (satp) */
    uint ppage_id = earth->mmu_alloc();
    root          = (void*)PAGE_ID_TO_ADDR(ppage_id);
    memset(root, 0, PAGE_SIZE);
    page_info_table[ppage_id].pid = pid;
    pid_to_pagetable_base[pid]    = root;

    /* Allocate the leaf page tables */
    for (uint i = RAM_START; i < RAM_END; i += PAGE_SIZE * 1024)
    {
        setup_identity_region(pid, i, 1024, USER_RWX);    /* RAM   */
    }
    setup_identity_region(pid, CLINT_BASE, 16, USER_RWX); /* CLINT */
    setup_identity_region(pid, UART_BASE, 1, USER_RWX);   /* UART  */
    setup_identity_region(pid, SPI_BASE, 1, USER_RWX);    /* SPI   */

    if (earth->platform == ARTY) {
        setup_identity_region(pid, BOARD_FLASH_ROM, 1024, USER_RWX); /* ROM */
        setup_identity_region(pid, ETHMAC_CSR_BASE, 1, USER_RWX);
        setup_identity_region(pid, ETHMAC_TX_BUFFER, 1, USER_RWX);
        setup_identity_region(pid, ETHMAC_RX_BUFFER, 1, USER_RWX);
    } else {
        /* Student's code goes here (Ethernet & TCP/IP). */

        /* Create page tables for the GEM I/O region of the sifive_u machine */
        /* Reference:
         * https://github.com/qemu/qemu/blob/stable-9.0/hw/riscv/sifive_u.c#L86
         */

        /* Student's code ends here. */
    }
}
uint copy_kernel_page(uint* app_root_page, uint base_addr){
    // app_root_page 一个配置项 4M
    uint vpn1 = base_addr >> 22;
    if(!(app_root_page[vpn1] & 0x1)){
        app_root_page[vpn1] = pid_to_pagetable_base[0][vpn1];
        // uint tmp = (pid_to_pagetable_base[0][vpn1] << 2) & 0xFFFFF000;
        // INFO("copy kernel num， vaddr: 0x%x, paddr: 0x%x", base_addr, tmp);
    }
}
// 内核页表做恒等映射
uint init_kernel_page(uint* app_root_page){
    // 系统应用需要内核页表
    copy_kernel_page(app_root_page, RAM_START);
    // copy_kernel_page(app_root_page, APPS_ENTRY);
    copy_kernel_page(app_root_page, APPS_PAGES_BASE);
    copy_kernel_page(app_root_page, APPS_PAGES_BASE + (0x1000 << 10));
    copy_kernel_page(app_root_page, CLINT_BASE);
    copy_kernel_page(app_root_page, UART_BASE);
    copy_kernel_page(app_root_page, SPI_BASE);
}
uint map_work_page(uint* root_page, uint pid){
    uint vaddr = 0x80602000;
    uint paddr = 0;
    uint vpn1 = vaddr >> 22;
    if(!(root_page[vpn1] & 0x1)){
        uint page_id = mmu_alloc();
        paddr = (uint)PAGE_ID_TO_ADDR(page_id);
        page_info_table[page_id].pid = pid;    
        memset((char*)paddr, 0, PAGE_SIZE);
        root_page[vpn1] = (paddr >> 2) | 0x1;
    }
    static uint work_paddr = 0;
    if(work_paddr == 0){
        // leaf页表
        uint page_id = mmu_alloc();
        work_paddr = (uint)PAGE_ID_TO_ADDR(page_id);
        page_info_table[page_id].pid = 0;    // 分配给所有用
        memset((char*)work_paddr, 0, PAGE_SIZE);
    }
    // INFO("map_work_page");
    uint vpn0 = (vaddr >> 12) & 0x3FF;
    if(!(((uint *)paddr)[vpn0] & 0x1)){
        ((uint *)paddr)[vpn0] = (work_paddr >> 2) | USER_RWX; 
    }
}

// 根据pid和vaddr得到页表地址
uint* table_entry(uint vaddr, uint pid){
    
    uint *root_page = pid_to_pagetable_base[pid];
    uint id = 0;
    // 页表不存在
    if(root_page <= 0){
        // 建立根页表
        id = mmu_alloc();
        page_info_table[id].pid = pid;
        memset(PAGE_ID_TO_ADDR(id), 0, PAGE_SIZE);
        root_page = (uint*)PAGE_ID_TO_ADDR(id);
        pid_to_pagetable_base[pid] = root_page; 
        // INFO("root page 0x%x", root_page); 
        // 系统应用映射内核页表
        if(pid < GPID_USER_START){
            init_kernel_page(root_page);
        }
        // 映射工作目录
        map_work_page(root_page, pid);
    }
    uint vpn1 = vaddr >> 22;
    uint page_entry = 0;
    if(!(root_page[vpn1] & 0x1)){
        // 建立leaf页表
        id = mmu_alloc();
        page_info_table[id].pid = pid;
        memset(PAGE_ID_TO_ADDR(id), 0, PAGE_SIZE);
        // RWX全为0表示指向下一级页表
        page_entry = (((uint)PAGE_ID_TO_ADDR(id) >> 2) | 0x1);
        root_page[vpn1] = (uint)page_entry;
    }
    page_entry = ((root_page[vpn1] << 2) & 0xFFFFF000);
    return (uint*)page_entry;
}


void page_table_map(int pid, uint vpage_no, uint ppage_id) {
    if (pid >= MAX_NPROCESS) FATAL("page_table_map: pid too large");
    // if(pid >= GPID_USER_START){
    //     INFO("pid:%d, vpage_no:0x%x, ppage_id:0x%x", pid, vpage_no, ppage_id);
    // }

    /* Student's code goes here (Virtual Memory). */

    /* clang-format off */
    /* Remove the following line of code and, instead,
     * (1) if page tables for pid do not exist, build the tables:
     *  (a) If the process is a system process (pid < GPID_USER_START)
     *      | Start Address | # Pages | Size   | Explanation                         |
     *      +---------------+---------+--------+-------------------------------------+
     *      | 0x80000000    | 1024    | 4 MB   | EGOS region (code+data+heap+stack)  |
     *      | 0x80400000    | 1024    | 4 MB   | Apps region (code+data+heap+stack)  |
     *      | 0x80800000    | 2048    | 8 MB   | Initially free pages for allocation |
     *      | CLINT_BASE    | 16      | 64 KB  | Memory-mapped registers for timer   |
     *      | UART_BASE     | 1       | 4 KB   | Memory-mapped registers for TTY     |
     *      | SPI_BASE      | 1       | 4 KB   | Memory-mapped registers for SD card |
     *
     *  (b) If the process is a user process (pid >= GPID_USER_START)
     *      | Start Address | # Pages | Size   | Explanation                         |
     *      +---------------+---------+--------+-------------------------------------+
     *      | 0x80602000    | 1       | 4 KB   | Work dir (see apps/app.h)           |
     *
     * (2) After building the page tables for pid (or if page tables for pid exist),
     *     find the PTE and update entries of the tables mapping vpage_no to ppage_id. */
    /* clang-format on */
    soft_tlb_map(pid, vpage_no, ppage_id);

    /* Student's code ends here. */
    // 功能，建立页表的映射 vaddr->paddr 虚拟地址到物理地址
    // INFO("page_table_map start");
    uint vaddr = (uint)PAGE_NO_TO_ADDR(vpage_no);
    uint *leaf_entry = table_entry(vaddr, pid);
    uint vpn0 = vaddr >> 12 & 0x3FF;
    // 存在该页表项， 直接返回
    if(leaf_entry[vpn0] & 0x1){
        return;
    }
    leaf_entry[vpn0] = ((uint)PAGE_ID_TO_ADDR(ppage_id) >> 2) |  USER_RWX;
    // INFO("page_table_map end");
  
    // INFO("pid = %d, pagetable_base[pid] = %x,vaddr = 0x%x, phy addr = 0x%x", pid,
    //  pid_to_pagetable_base[pid], vaddr, (leaf_entry[vpn0] << 2) & 0xFFFFF000);
    
}



void page_table_switch(int pid) {
    /* Student's code goes here (Virtual Memory). */

    /* Remove the following line of code and, instead, modify the page table
     * base register (satp) similar to the code in mmu_init(). */
    // soft_tlb_switch(pid);

    /* Student's code ends here. */
    uint page_table = (uint)pid_to_pagetable_base[pid];
    if(page_table <= 0){
        FATAL("page_table_switch error!!!");
    }
    // 切换根页表
    asm("csrw satp, %0" ::"r"(((uint)page_table >> 12) | (1 << 31)));
}

uint page_table_translate(int pid, uint vaddr) {
    /* Student's code goes here (Virtual Memory). */

    /* Remove the following line of code and, instead, walk through the page
     * tables of process pid and return the paddr mapped from vaddr. */
    // return soft_tlb_translate(pid, vaddr);

    /* Student's code ends here. */
    uint* pagetable_base = pid_to_pagetable_base[pid];
    if(!pagetable_base){
        FATAL("page_table_translate: pagetable_base is NULL");
    }
    uint vpn1 = vaddr >> 22;
    // 地址有效
    if(pagetable_base[vpn1] & 0x1){
        uint *page_entry = (uint*)(((uint)pagetable_base[vpn1] << 2) & 0xFFFFF000);
        uint vpn0 = vaddr >> 12 & 0x3FF;
        if(page_entry[vpn0] & 0x1){
            uint paddr = (page_entry[vpn0] << 2) & 0xFFFFF000;
            return paddr + (vaddr & 0xFFF);
        }else{
            FATAL("page_table_translate: error 1!");
        }
    }
    else{
        FATAL("page_table_translate: error 2!");
    }
    
}

void flush_cache() {
    if (earth->platform == ARTY) {
        /* Flush the L1 instruction cache */
        /* See
         * https://github.com/yhzhang0128/litex/blob/egos/litex/soc/cores/cpu/vexriscv_smp/system.h#L9-L25
         */
        // INFO("ARTY");
        asm(".word(0x100F)\nnop\nnop\nnop\nnop\nnop\n");
    }
    if (earth->translation == PAGE_TABLE) {
        /* Flush the TLB, which is the cache of page table entries */
        /* See
         * https://riscv.org/wp-content/uploads/2017/05/riscv-privileged-v1.10.pdf#subsection.4.2.1
         */
        // INFO("PAGE_TABLE");
        asm("sfence.vma zero,zero");
        // asm volatile ("sfence.vma");
    }
}

/* MMU Initialization */
void mmu_init() {
    earth->mmu_free        = mmu_free;
    earth->mmu_alloc       = mmu_alloc;
    earth->mmu_flush_cache = flush_cache;

    /* Setup a PMP region for the whole 4GB address space 
        此处设置所有物理内存可以访问，因为如果页表处的物理内存不能访问的话，mmu无法完成地址转换
        mmu转换地址的时候的特权级，与cpu的当前特权级有关，也就是在用户模式下，mmu相当于也只有用户特权级
    */ 
    asm("csrw pmpaddr0, %0" : : "r"(0x40000000));
    asm("csrw pmpcfg0, %0" : : "r"(0xF));
    INFO("app addrs space: 1GB");

    /* Student's code goes here (System Call & Protection). */

    /* Setup PMP NAPOT region 0x80400000 - 0x80800000 as r/w/x */
    // uint pmpaddr = (0x80400000 >> 2) | ((1 << 19) - 1);
    // INFO("pmpaddr %d", pmpaddr); // 输出538443775
    
    // 用户程序地址空间
    // uint pmpaddr = (0x80400000 >> 2) | ((1 << 19) - 1);
    // uint pmpcfg = 0;
    // asm("csrw pmpaddr0, %0"::"r"(pmpaddr));         // 这个地址读写执行
    // asm("csrr %0, pmpcfg0":"=r"(pmpcfg));
    // pmpcfg |= 0x1f;
    // asm("csrw pmpcfg0, %0"::"r"(pmpcfg));
    // // 页表地址空间
    // pmpaddr = (0x80800000 >> 2) | ((1 << 20) - 1);
    // asm("csrw pmpaddr1, %0"::"r"(pmpaddr));         // 这个地址只读, 页表没有读权限会导致内存访问异常
    // asm("csrr %0, pmpcfg0":"=r"(pmpcfg));
    // pmpcfg |= 0x19 << 8;
    // asm("csrw pmpcfg0, %0"::"r"(pmpcfg));

    /* Choose memory translation mechanism in QEMU */
    CRITICAL("Choose a memory translation mechanism:");
    printf("Enter 0: page tables\r\nEnter 1: software TLB\r\n");

    char buf[2];
    for (buf[0] = 0; buf[0] != '0' && buf[0] != '1'; earth->tty_read(&buf[0]));
    earth->translation = (buf[0] == '0') ? PAGE_TABLE : SOFT_TLB;
    INFO("%s translation is chosen",
         earth->translation == PAGE_TABLE ? "Page table" : "Software");

    if (earth->translation == PAGE_TABLE) {
        /* Setup an identity mapping using page tables */
        pagetable_identity_map(0);
        asm("csrw satp, %0" ::"r"(((uint)root >> 12) | (1 << 31)));

        earth->mmu_map       = page_table_map;
        earth->mmu_switch    = page_table_switch;
        earth->mmu_translate = page_table_translate;
    } else {
        earth->mmu_map       = soft_tlb_map;
        earth->mmu_switch    = soft_tlb_switch;
        earth->mmu_translate = soft_tlb_translate;
    }
}
