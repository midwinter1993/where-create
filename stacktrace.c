#include <stdio.h>
#include <stdlib.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>

//
// http://eli.thegreenplace.net/2015/programmatic-access-to-the-call-stack-in-c/
//
void stacktrace() {
    unw_cursor_t cursor;
    unw_context_t context;

    // Initialize cursor to current frame for local unwinding.
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    printf("======================================\n");
    // Unwind frames one by one, going up the frame stack.
    while (unw_step(&cursor) > 0) {
        unw_word_t offset, pc;
        unw_get_reg(&cursor, UNW_REG_IP, &pc);
        if (pc == 0) {
            break;
        }
        printf("0x%lx:", pc);

        char sym[256];
        if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
            char *func_name = sym;
            printf(" (%s+0x%lx)\n", func_name, offset);
        } else {
            printf(" -- error: unable to obtain symbol name for this frame\n");
        }
    }
    printf("======================================\n");
}
