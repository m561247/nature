#include "tests/test.h"
#include "utils/assertf.h"
#include "utils/exec.h"
#include <stdio.h>

static void test_basic() {
    char *raw = exec_output();
    printf("%s", raw);

//    assert_string_equal(raw, "foo= 5\n"
//                             "sums(...)= 53\n");
}

int main(void) {
    TEST_BASIC
}