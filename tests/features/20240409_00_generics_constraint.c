#include <stdio.h>

#include "string.h"
#include "tests/test.h"
#include "utils/assertf.h"
#include "utils/exec.h"

static void test_basic() {
    char *raw = exec_output();
    char *str = "bar_t any dump 233 333.444000\n"
            "f32+bool dump 233.332993 true\n"
            "void return, f32|bool+int|string dump true hello world\n"
#if defined(__DARWIN) && defined(__ARM64)
            "bar_t any dump 233.332993 0xc000004180\n";
#elif defined(__RISCV64)
            "bar_t any dump 233.332993 0x400002180\n";
#else
            "bar_t any dump 233.332993 0xc000002180\n";
#endif


    assert_string_equal(raw, str);
}

int main(void) {
    TEST_BASIC
}
