void terminal_write(const char *str, int len) {
    for (int i = 0; i < len; i++) {
        *(char*)(0x10000000UL) = str[i];
    }
}

#include <string.h>  // for strlen() and strcat()
#include <stdlib.h>  // for itoa()
#include <stdarg.h>  // for va_start(), va_end() and va_arg()

void format_to_str(char* out, const char* fmt, va_list args) {
    for(out[0] = 0; *fmt != '\0'; fmt++) {
        if (*fmt != '%') {
            strncat(out, fmt, 1);
        } else {
            fmt++;
            if (*fmt == 's') {
                strcat(out, va_arg(args, char*));
            } else if (*fmt == 'd') {
                itoa(va_arg(args, int), out + strlen(out), 10);
            }
        }
    }
}

int printf(const char* format, ...) {
    char buf[512];
    va_list args;
    va_start(args, format);
    format_to_str(buf, format, args);
    va_end(args);
    terminal_write(buf, strlen(buf));

    return 0;
}

extern char __heap_start, __heap_max;
static char* brk = &__heap_start;
char* _sbrk(int size) {
    if (brk + size > (char*)&__heap_max) {
        terminal_write("_sbrk: heap grows too large\r\n", 29);
        return NULL;
    }

    char* old_brk = brk;
    brk += size;
    return old_brk;
}

#include "queue.h"
#include "thread.h"

/* Student's code goes here (Cooperative Threads). */
/* Define helper functions, if needed, for multi-threading */
typedef struct thread* thread_t; 
thread_t current = NULL;
queue_t tcb_queue = NULL;
thread_t main_tcb = NULL;
/* Student's code ends here. */

// 主线程使用
void thread_init() {
    /* Student's code goes here (Cooperative Threads). */
    // 创建tcb控制队列
    tcb_queue = queue_new();
    // 主线程tcb
    thread_t main_tcb = malloc(sizeof(struct thread));
    main_tcb->sp = NULL;
    current = main_tcb;
    /* Student's code ends here. */
}

void ctx_entry() {
    /* Student's code goes here (Cooperative Threads). */
    // 初始化栈
    void (*entry)(void *arg) = current->fun;
    entry(current->args);
    thread_exit();
    /* Student's code ends here. */
}

void thread_create(void (*entry)(void *arg), void *arg, int stack_size) {
    /* Student's code goes here (Cooperative Threads). */
    thread_t child_t = malloc(sizeof(struct thread));
    child_t->init_sp = malloc(stack_size);
    // 移动到栈顶
    child_t->sp = (void*)((char *)child_t->init_sp + stack_size);
    // 设置线程入口及参数
    child_t->fun = entry;
    child_t->args = arg;
    // 切换
    thread_t old = current;
    queue_enqueue(tcb_queue, old);
    current = child_t;
    // 开始执行, 保存当前上下文
    ctx_start(&old->sp, current->sp);
    /* Student's code ends here. */
}

void thread_yield() {
    /* Student's code goes here (Cooperative Threads). */
    void *tmp = NULL;
    int ret = queue_dequeue(tcb_queue, &tmp);
    // 没有别的线程了
    if(ret != 0){
        return;
    }
    thread_t old = current;
    // 将其放入队列中
    queue_enqueue(tcb_queue, old);
    current = (thread_t)tmp;
    ctx_switch(&old->sp, current->sp);
    /* Student's code ends here. */
}

void thread_exit() {
    /* Student's code goes here (Cooperative Threads). */
    printf("thread_exit\n\r");
    thread_t end = current;
    // 获取下一个线程
    void *tmp = NULL;
    int ret = queue_dequeue(tcb_queue, &tmp);
    // 没有别的线程了
    if(ret != 0){
        printf("thread_exit() no other\n\r");
        free(end->init_sp);
        free(end);
        return;
    }
    current = (thread_t)tmp;
    // 释放结束tcb资源
    free(end->init_sp);
    free(end);
    // 切换线程, 传入局部变量的tmp就不会保留上下文的栈指针了,永不返回
    ctx_switch(&tmp, current->sp);
    printf("never print");
    /* Student's code ends here. */
}

/* Student's code goes here (Cooperative Threads). */
/* Define helper functions, if needed, for conditional variables */
struct cv nonempty, nonfull;

/* Student's code ends here. */

void cv_init(struct cv *condition) {
    /* Student's code goes here (Cooperative Threads). */
    condition->tcb_queue = queue_new();
    condition->cv = 0;
    /* Student's code ends here. */
}

void cv_wait(struct cv *condition) {
    /* Student's code goes here (Cooperative Threads). */
    while(!condition->cv){
        void *tmp = NULL;
        queue_dequeue(tcb_queue, &tmp);
        thread_t next = (thread_t)tmp;
        thread_t old = current;
        current = next;
        queue_enqueue(condition->tcb_queue, old);
        ctx_switch(&old->sp, current->sp);
    }
    condition->cv -= 1; 
    /* Student's code ends here. */
}

void cv_signal(struct cv *condition) {
    /* Student's code goes here (Cooperative Threads). */
    condition->cv += 1;
    void *tmp = NULL;
    int ret = queue_dequeue(condition->tcb_queue, &tmp);
    if(ret != 0){
        return;
    }
    queue_enqueue(tcb_queue, tmp);
    
    /* Student's code ends here. */
}

#define BUF_SIZE 3
void* buffer[BUF_SIZE];
int count = 0;
int head = 0, tail = 0;
struct cv nonempty, nonfull;

void produce(void* item) {
    // printf("produce()\n\r");
    int i = 0;
    while (i < 1) {
        while (count == BUF_SIZE) cv_wait(&nonfull);
        /* At this point, the buffer is not full. */

        /* Student's code goes here (Cooperative Threads). */
        /* Print out producer thread ID and the item pointer */
        printf("produce %d\n\r", i++);
        /* Student's code ends here. */
        buffer[tail] = item;
        tail = (tail + 1) % BUF_SIZE;
        count += 1;
        cv_signal(&nonempty);
    }
}

void consume(void *arg) {
    // printf("consume()\n\r");
    int i = 0;
    while (i < 1) {
        while (count == 0) cv_wait(&nonempty);
        /* At this point, the buffer is not empty. */

        /* Student's code goes here (Cooperative Threads). */
        /* Print out producer thread ID and the item pointer */
        printf("consume %d\n\r", i++);
        /* Student's code ends here. */
        void* result = buffer[head];
        head = (head + 1) % BUF_SIZE;
        count -= 1;
        cv_signal(&nonfull);
    }
}

void nihao(){
    for(int i = 0; i < 20; i++){
        printf("nihao : %d\n\r", i);
        thread_yield();
    }
    printf("nihao end\n\r");
}

int main() {
    thread_init();
    printf("main()\n\r");
    cv_init(&nonempty);
    cv_init(&nonfull);
    for (int i = 0; i < 10; i++){
        // printf("consume create %d \n\r", i);
        thread_create(consume, NULL, STACK_SIZE / 16);
    }
        

    for (int i = 0; i < 10; i++)
        thread_create(produce, NULL, STACK_SIZE / 16);
    // thread_create(nihao, NULL, STACK_SIZE / 16);
    // for(int i = 0; i < 10; i++){
    //     printf("main : %d\n\r", i);
    //     thread_yield();
    // }
    printf("main thread exits\n\r");
    thread_exit();
}
