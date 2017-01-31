#include <string.h>

#include "unity.h"
#include "Lexer.h"

typedef struct test_data {
    char* input;
    token expected;
} test_data;

void setUp(void) {
}

void tearDown(void) {
}

void test_gettoken_parses_identifier(void) {
    struct test_data data[] = {
        { "a", { TOKEN_IDENTIFIER, {.str = "a"}}},
        { "ab", { TOKEN_IDENTIFIER, {.str = "ab"}}},
        { "ab*", { TOKEN_IDENTIFIER, {.str = "ab"}}},
        { "ab9", { TOKEN_IDENTIFIER, {.str = "ab9"}}},
        { " ab9", { TOKEN_IDENTIFIER, {.str = "ab9"}}},
        { "  ab9", { TOKEN_IDENTIFIER, {.str = "ab9"}}},
    };
    size_t count = sizeof(data)/sizeof(data[0]);

    for(size_t i=0; i<count; i++) {
        struct test_data *d = &(data[i]);
        FILE *stream = fmemopen(d->input, strlen(d->input), "r");
        token result = gettok(stream);
        TEST_ASSERT_EQUAL_INT(d->expected.type, result.type);
        TEST_ASSERT_EQUAL_STRING(d->expected.value.str, result.value.str);
        fclose(stream);
    }
}

void test_gettoken_parses_integer(void) {
    struct test_data data[] = {
        { "0", { TOKEN_INTEGER, {.i = 0}}},
        { "1", { TOKEN_INTEGER, {.i = 1}}},
        { "+1", { TOKEN_INTEGER, {.i = 1}}},
        { "-1", { TOKEN_INTEGER, {.i = -1}}},
        { "10", { TOKEN_INTEGER, {.i = 10}}},
        { "+10", { TOKEN_INTEGER, {.i = 10}}},
        { "-10", { TOKEN_INTEGER, {.i = -10}}},
        { "2736", { TOKEN_INTEGER, {.i = 2736}}},
        { "+2736", { TOKEN_INTEGER, {.i = 2736}}},
        { "-2736", { TOKEN_INTEGER, {.i = -2736}}},
        { "536870911", { TOKEN_INTEGER, {.i = 536870911}}},
        { "+536870911", { TOKEN_INTEGER, {.i = 536870911}}},
        { "-536870912", { TOKEN_INTEGER, {.i = -536870912}}},
        { "01", { TOKEN_INTEGER, {.i = 1}}},
        { "+01", { TOKEN_INTEGER, {.i = 1}}},
        { "-01", { TOKEN_INTEGER, {.i = -1}}},
        { "010", { TOKEN_INTEGER, {.i = 8}}},
        { "+010", { TOKEN_INTEGER, {.i = 8}}},
        { "-010", { TOKEN_INTEGER, {.i = -8}}},
        { "02736", { TOKEN_INTEGER, {.i = 1502}}},
        { "+02736", { TOKEN_INTEGER, {.i = 1502}}},
        { "-02736", { TOKEN_INTEGER, {.i = -1502}}},
        { "03777777777", { TOKEN_INTEGER, {.i = 536870911}}},
        { "+03777777777", { TOKEN_INTEGER, {.i = 536870911}}},
        { "-04000000000", { TOKEN_INTEGER, {.i = -536870912}}},
        { "0x1", { TOKEN_INTEGER, {.i = 1}}},
        { "+0X1", { TOKEN_INTEGER, {.i = 1}}},
        { "-0x1", { TOKEN_INTEGER, {.i = -1}}},
        { "0X10", { TOKEN_INTEGER, {.i = 16}}},
        { "+0x10", { TOKEN_INTEGER, {.i = 16}}},
        { "-0X10", { TOKEN_INTEGER, {.i = -16}}},
        { "0x273f", { TOKEN_INTEGER, {.i = 10047}}},
        { "+0X273f", { TOKEN_INTEGER, {.i = 10047}}},
        { "-0x273f", { TOKEN_INTEGER, {.i = -10047}}},
        { "0x1FFFFFFF", { TOKEN_INTEGER, {.i = 536870911}}},
        { "+0x1FFFFFFF", { TOKEN_INTEGER, {.i = 536870911}}},
        { "-0x20000000", { TOKEN_INTEGER, {.i = -536870912}}},
    };
    size_t count = sizeof(data)/sizeof(data[0]);

    for(size_t i=0; i<count; i++) {
        struct test_data *d = &(data[i]);
        FILE *stream = fmemopen(d->input, strlen(d->input), "r");
        token result = gettok(stream);
        TEST_ASSERT_EQUAL_INT(d->expected.type, result.type);
        TEST_ASSERT_EQUAL_INT8(d->expected.value.i, result.value.i);
        fclose(stream);
    }
}

void test_gettoken_parses_symbol(void) {
    struct test_data data[] = {
        { "(", { TOKEN_SYMBOL, {.c = '('}}},
        { " (", { TOKEN_SYMBOL, {.c = '('}}},
        { "  (", { TOKEN_SYMBOL, {.c = '('}}},
    };
    size_t count = sizeof(data)/sizeof(data[0]);

    for(size_t i=0; i<count; i++) {
        struct test_data *d = &(data[i]);
        FILE *stream = fmemopen(d->input, strlen(d->input), "r");
        token result = gettok(stream);
        TEST_ASSERT_EQUAL_INT(d->expected.type, result.type);
        TEST_ASSERT_EQUAL_INT8(d->expected.value.c, result.value.c);
        fclose(stream);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_gettoken_parses_identifier);
    RUN_TEST(test_gettoken_parses_integer);
    RUN_TEST(test_gettoken_parses_symbol);
    return UNITY_END();
}
