#define _GNU_SOURCE

#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <dlfcn.h>

#include "stacktrace.h"


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define UNUSED(v) ((void)v)

#define declare(return_type, func_name, arg_type) \
    return_type func_name arg_type; \
    typedef return_type (*func_name##_type)arg_type; \
    static func_name##_type __real_##func_name = NULL;

#define replace(func_name) \
    do { \
        __real_##func_name = (func_name##_type)dlsym(RTLD_NEXT, #func_name); \
        assert(__real_##func_name != func_name); \
        if (!__real_##func_name) { \
            fprintf(stderr, "Error in `dlsym`: %s\n", dlerror()); \
        } else {\
            fprintf(stderr, "NOTICE: %s() @%p replaced with %p\n", #func_name, __real_##func_name, func_name); \
        } \
    } while (0)

//
// Flag for init; NOT thread-safe
//
enum {
    INIT_UNDO,
    INIT_ING,
    INIT_DONE
};
static int __init_status = INIT_UNDO;

//
// =============== Original functions ============
//
typedef void* (*routine_type)(void *);
declare(int, pthread_create, (pthread_t*, const pthread_attr_t*, routine_type, void*));

//
// ========== Init: load lib functions ===========
//
/* static void do_init() __attribute__((constructor)); */
static void do_init() {
    __init_status = INIT_ING;
    // Override original functions
    replace(pthread_create);
    __init_status = INIT_DONE;
}

#define CHECK_AND_DO_INIT() \
    do { \
        if(__init_status == INIT_UNDO) { \
            do_init(); \
        } \
    } while(false)

typedef struct routine_wrapper_arg_t {
    routine_type routine;
    void *arg;
} routine_wrapper_arg_t;

static void* routine_wrapper(void *arg) {
    routine_wrapper_arg_t *wrapper_arg = (routine_wrapper_arg_t*)arg;

    void *ret = wrapper_arg->routine(wrapper_arg->arg);

    fprintf(stdout, "[ Routine END %lu ]\n", pthread_self());
    stacktrace();
    free(wrapper_arg);
    return ret;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, routine_type routine, void *arg) {
    CHECK_AND_DO_INIT();

    stacktrace();

    routine_wrapper_arg_t *wrapper_arg = (routine_wrapper_arg_t*)malloc(sizeof(routine_wrapper_arg_t));
    wrapper_arg->routine = routine;
    wrapper_arg->arg = arg;

    int ret = __real_pthread_create(thread, attr, routine_wrapper, wrapper_arg);
    fprintf(stdout, "[ Create Routine %lu ]\n", *thread);
    return ret;
}
