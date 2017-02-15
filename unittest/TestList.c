#include <string.h>

#include "unity.h"

#include "List.h"

typedef struct test_data {
    char *input;
    char *expected;
} test_data;

void setUp(void) {
}

void tearDown(void) {
}

void test_EmptyList_toString(void) {
    lisp_object *obj = (lisp_object*)EmptyList;
    TEST_ASSERT_EQUAL_STRING("()", (*(obj->toString))(obj));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_EmptyList_toString);
    return UNITY_END();
}
