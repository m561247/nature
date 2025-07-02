// Copyright 2018 Sen Han <00hnes@gmail.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ACO_H
#define ACO_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <include/uv.h>
#include <pthread.h>

#include "runtime/fixalloc.h"

#define ACO_VERSION_MAJOR 1
#define ACO_VERSION_MINOR 2
#define ACO_VERSION_PATCH 4

#ifdef __i386__
#define ACO_REG_IDX_RETADDR 0
#define ACO_REG_IDX_SP 1
#define ACO_REG_IDX_BP 2
#define ACO_REG_IDX_FPU 6
#elif __x86_64__
#define ACO_REG_IDX_RETADDR 4
#define ACO_REG_IDX_SP 5
#define ACO_REG_IDX_BP 7
#define ACO_REG_IDX_FPU 8
#elif defined(__aarch64__)
#define ACO_REG_IDX_RETADDR 13
#define ACO_REG_IDX_SP 14
#define ACO_REG_IDX_BP 12
#define ACO_REG_IDX_FPU 15
#elif defined(__riscv) && (__riscv_xlen == 64)
#define ACO_REG_IDX_RETADDR 12  // ra 寄存器在索引 12 (偏移量 96)
#define ACO_REG_IDX_SP 13       // sp 寄存器在索引 13 (偏移量 104)
#define ACO_REG_IDX_BP 0        // s0/fp 寄存器在索引 0 (偏移量 0)
#define ACO_REG_IDX_FPU 14      // fcsr 在索引 14 (偏移量 112)
#else
#error "platform no support yet"
#endif

typedef struct {
    char *msg; // 通过 context 进行消息传递
} aco_context_t;

typedef struct {
    void *ptr; // need gc mark
    size_t sz; // save stack size
    size_t valid_sz; // 基于 sp 指针计算出来的 valid sz
    // max copy size in bytes
    size_t max_cpsz;
    // copy from share stack to this save stack
    size_t ct_save;
    // copy from this save stack to share stack
    size_t ct_restore;
} aco_save_stack_t;

struct aco_s;
typedef struct aco_s aco_t;

typedef struct {
    void *ptr;
    size_t sz;
    void *align_highptr;
    void *align_retptr;
    size_t align_validsz;
    size_t align_limit;
    aco_t *owner;

    char guard_page_enabled;
    void *real_ptr;
    size_t real_sz;
#ifdef __LINUX
    pthread_spinlock_t owner_lock;
#else
    pthread_mutex_t owner_lock;
#endif
} aco_share_stack_t;

typedef void (*aco_cofuncp_t)(void);

struct aco_s {
#ifdef __AMD64
    void *reg[9]; // amd64 使用
#elif defined(__ARM64)
    void* reg[16];
#elif defined(__RISCV64)
    void* reg[16];
#else
#error "platform no support yet"
#endif


    aco_t *main_co;
    void *arg;
    char is_end;

    aco_cofuncp_t fp;
    aco_save_stack_t save_stack;
    aco_share_stack_t *share_stack;
    aco_context_t ctx;
    bool inited;
};

#define aco_likely(x) (__builtin_expect(!!(x), 1))

#define aco_unlikely(x) (__builtin_expect(!!(x), 0))

#define aco_assert(EX) ((aco_likely(EX)) ? ((void)0) : (abort()))

#define aco_assertptr(ptr) ((aco_likely((ptr) != NULL)) ? ((void)0) : (abort()))

#define aco_assertalloc_bool(b)                                                                                          \
    do {                                                                                                                 \
        if (aco_unlikely(!(b))) {                                                                                        \
            fprintf(stderr, "Aborting: failed to allocate memory: %s:%d:%s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__); \
            abort();                                                                                                     \
        }                                                                                                                \
    } while (0)

#define aco_assertalloc_ptr(ptr)                                                                                         \
    do {                                                                                                                 \
        if (aco_unlikely((ptr) == NULL)) {                                                                               \
            fprintf(stderr, "Aborting: failed to allocate memory: %s:%d:%s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__); \
            abort();                                                                                                     \
        }                                                                                                                \
    } while (0)

#if defined(aco_attr_no_asan)
#error "aco_attr_no_asan already defined"
#endif
#ifndef aco_attr_no_asan
#define aco_attr_no_asan
#endif

extern uv_key_t aco_gtls_co;           // aco_t*
extern uv_key_t aco_gtls_last_word_fp; // aco_cofuncp_t
extern uv_key_t aco_gtls_fpucw_mxcsr;  // void*
extern pthread_mutex_t share_stack_lock;

static inline void aco_init() {
    pthread_mutex_init(&share_stack_lock, NULL);
    uv_key_create(&aco_gtls_co);
    uv_key_create(&aco_gtls_last_word_fp);
    uv_key_create(&aco_gtls_fpucw_mxcsr);
}

extern void aco_runtime_test(void);

extern void aco_thread_init(aco_cofuncp_t last_word_co_fp);

extern void *acosw(aco_t *from_co, aco_t *to_co) __asm__("acosw"); // asm

extern void aco_save_fpucw_mxcsr(void *p) __asm__("aco_save_fpucw_mxcsr"); // asm

extern void aco_funcp_protector_asm(void) __asm__("aco_funcp_protector_asm"); // asm

extern void aco_funcp_protector(void);

extern void *aco_share_stack_init(aco_share_stack_t *p, size_t sz);

extern void aco_share_stack_destroy(aco_share_stack_t *sstk);

extern void aco_create_init(aco_t *aco, aco_t *main_co, aco_share_stack_t *share_stack, size_t save_stack_sz,
                            aco_cofuncp_t fp, void *arg);

// aco's Global Thread Local Storage variable `co`
// extern __thread aco_t *aco_gtls_co;

static inline aco_t *get_aco_gtls_co() {
    return uv_key_get(&aco_gtls_co);
}

aco_attr_no_asan extern void aco_resume(aco_t *resume_co);

aco_attr_no_asan void aco_share_spill(aco_t *aco);

// extern void aco_yield1(aco_t* yield_co);
#define aco_yield1(yield_co)                    \
    do {                                        \
        aco_assertptr((yield_co));              \
        aco_assertptr((yield_co)->main_co);     \
        acosw((yield_co), (yield_co)->main_co); \
    } while (0)

#define aco_yield()                    \
    do {                               \
        aco_yield1(get_aco_gtls_co()); \
    } while (0)

#define aco_get_arg() (((aco_t *)get_aco_gtls_co())->arg)

#define aco_get_co()       \
    ({                     \
        (void)0;           \
        get_aco_gtls_co(); \
    })

#define aco_co()           \
    ({                     \
        (void)0;           \
        get_aco_gtls_co(); \
    })

extern void aco_destroy(aco_t *co);

#define aco_is_main_co(co) ({ ((co)->main_co) == NULL; })

#define aco_exit1(co)                                 \
    do {                                              \
        (co)->is_end = 1;                             \
        aco_assert((co)->share_stack->owner == (co)); \
        (co)->share_stack->owner = NULL;              \
        (co)->share_stack->align_validsz = 0;         \
        aco_yield1((co));                             \
        aco_assert(0);                                \
    } while (0)

#define aco_exit()                    \
    do {                              \
        aco_exit1(get_aco_gtls_co()); \
    } while (0)

#endif
