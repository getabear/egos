#include "app.h"
#include "string.h"

int main() {
    SUCCESS("Enter kernel process GPID_IDLE");

    char buf[SYSCALL_MSG_LEN];
    strcpy(buf, "Finish GPID_IDLE initialization");
    // INFO("GPID_IDLE");
    grass->sys_send(GPID_PROCESS, buf, 36);

    while (1) {
    }  
}
