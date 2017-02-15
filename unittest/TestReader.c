#include <string.h>

#include "unity.h"

#include "Reader.h"

typedef struct test_data {
    char *input;
    char *expected;
} test_data;

void setUp(void) {
    init_macros();
}

void tearDown(void) {
}

void test_read_integer(void) {
    struct test_data data[] = {
        { "0", "0"},
        { "1", "1"},
        { "+1", "1"},
        { "-1", "-1"},
        { "10", "10"},
        { "+10", "10"},
        { "-10", "-10"},
        { "2736", "2736"},
        { "+2736", "2736"},
        { "-2736", "-2736"},
        { "536870911", "536870911"},
        { "+536870911", "536870911"},
        { "-536870912", "-536870912"},
        { "01", "1"},
        { "+01", "1"},
        { "-01", "-1"},
        { "010", "8"},
        { "+010", "8"},
        { "-010", "-8"},
        { "02736", "1502"},
        { "+02736", "1502"},
        { "-02736", "-1502"},
        { "03777777777", "536870911"},
        { "+03777777777", "536870911"},
        { "-04000000000", "-536870912"},
        { "0x1", "1"},
        { "+0X1", "1"},
        { "-0x1","-1"},
        { "0X10", "16"},
        { "+0x10", "16"},
        { "-0X10", "-16"},
        { "0x273f", "10047"},
        { "+0X273f", "10047"},
        { "-0x273f", "-10047"},
        { "0x1FFFFFFF", "536870911"},
        { "+0x1FFFFFFF", "536870911"},
        { "-0x20000000", "-536870912"},
    };
    size_t count = sizeof(data)/sizeof(data[0]);

    for(size_t i=0; i<count; i++) {
        struct test_data *d = &(data[i]);
        FILE *stream = fmemopen(d->input, strlen(d->input), "r");
        lisp_object *ret = read(stream, false, '\0');
        char *result = (*ret->toString)(ret);
        TEST_ASSERT_EQUAL_STRING(d->expected, result);
        fclose(stream);
    }
}

void test_read_float(void) {
    struct test_data data[] = {
        { "3.3830102744703936e-74", "3.38301e-74"},
        { "-7.808793319244098e+180", "-7.80879e+180"},
        { "-1.116240615436433e+257", "-1.11624e+257"},
        { "-4.474217486501186e-98", "-4.47422e-98"},
        { "2.551330225124468e+227", "2.55133e+227"},
        { "-5.586243311594498e+114", "-5.58624e+114"},
        { "-1.9638495479072282e+39", "-1.96385e+39"},
        { "-9.379950647423316e+83", "-9.37995e+83"},
        { "-1.779476947470501e+101", "-1.77948e+101"},
        { "-3.1562114238196845e-189", "-3.15621e-189"},
        { "5.923117747870643e-108", "5.92312e-108"},
        { "1.8563926817741926e-264", "1.85639e-264"},
        { "-2.5118492190358267e-16", "-2.51185e-16"},
        { "-2.8979900686128697e-99", "-2.89799e-99"},
        { "5.558913806322226e-202", "5.55891e-202"},
        { "-4.199353966327336e-83", "-4.19935e-83"},
        { "-7.947043443990472e-185", "-7.94704e-185"},
        { "1.2226938225065016e-25", "1.22269e-25"},
        { "1.5983262622130207e+56", "1.59833e+56"},
        { "1.084094471192463e-200", "1.08409e-200"},
        { "3.8547840245831654e-274", "3.85478e-274"},
        { "5.737113723493468e-131", "5.73711e-131"},
        { "-6.354596607132215e+58", "-6.3546e+58"},
        { "-4.644672297406679e-96", "-4.64467e-96"},
        { "-1.247283555379344e+271", "-1.24728e+271"},
        { "-1.922826530910511e+135", "-1.92283e+135"},
        { "-1.150823979210901e+133", "-1.15082e+133"},
        { "-9.625621103417222e-57", "-9.62562e-57"},
        { "-1.394256960569535e+303", "-1.39426e+303"},
        { "1.106935384820126e+184", "1.10694e+184"},
        { "1.3745629336289501e-248", "1.37456e-248"},
        { "-1.6239665626807802e+162", "-1.62397e+162"},
        { "-2.8705374780007167e-09", "-2.87054e-09"},
        { "8.427026998307476e+287", "8.42703e+287"},
        { "9.693509881319873e-164", "9.69351e-164"},
        { "-3.6644724313512395e+199", "-3.66447e+199"},
        { "2.4438302280087908e-17", "2.44383e-17"},
        { "1.603926355223715e+53", "1.60393e+53"},
        { "-6.93715759327617e+263", "-6.93716e+263"},
        { "423247314940.0702", "4.23247e+11"},
    };
    size_t count = sizeof(data)/sizeof(data[0]);

    for(size_t i=0; i<count; i++) {
        struct test_data *d = &(data[i]);
        FILE *stream = fmemopen(d->input, strlen(d->input), "r");
        lisp_object *ret = read(stream, false, '\0');
        char *result = (*ret->toString)(ret);
        TEST_ASSERT_EQUAL_STRING(d->expected, result);
        fclose(stream);
    }
}

void test_read_char(void) {
    struct test_data data[] = {
        { "\\tab", "\t"},
        { "\\newline", "\n"},
        { "\\return", "\r"},
        { "\\space", " "},
        { "\\!", "!"},
        { "\\\"", "\""},
        { "\\#", "#"},
        { "\\$", "$"},
        { "\\%", "%"},
        { "\\&", "&"},
        { "\\'", "'"},
        { "\\(", "("},
        { "\\)", ")"},
        { "\\*", "*"},
        { "\\+", "+"},
        { "\\,", ","},
        { "\\-", "-"},
        { "\\.", "."},
        { "\\/", "/"},
        { "\\0", "0"},
        { "\\1", "1"},
        { "\\2", "2"},
        { "\\3", "3"},
        { "\\4", "4"},
        { "\\5", "5"},
        { "\\6", "6"},
        { "\\7", "7"},
        { "\\8", "8"},
        { "\\9", "9"},
        { "\\:", ":"},
        { "\\;", ";"},
        { "\\<", "<"},
        { "\\=", "="},
        { "\\>", ">"},
        { "\\?", "?"},
        { "\\@", "@"},
        { "\\A", "A"},
        { "\\B", "B"},
        { "\\C", "C"},
        { "\\D", "D"},
        { "\\E", "E"},
        { "\\F", "F"},
        { "\\G", "G"},
        { "\\H", "H"},
        { "\\I", "I"},
        { "\\J", "J"},
        { "\\K", "K"},
        { "\\L", "L"},
        { "\\M", "M"},
        { "\\N", "N"},
        { "\\O", "O"},
        { "\\P", "P"},
        { "\\Q", "Q"},
        { "\\R", "R"},
        { "\\S", "S"},
        { "\\T", "T"},
        { "\\U", "U"},
        { "\\V", "V"},
        { "\\W", "W"},
        { "\\X", "X"},
        { "\\Y", "Y"},
        { "\\Z", "Z"},
        { "\\[", "["},
        { "\\]", "]"},
        { "\\^", "^"},
        { "\\_", "_"},
        { "\\`", "`"},
        { "\\a", "a"},
        { "\\b", "b"},
        { "\\c", "c"},
        { "\\d", "d"},
        { "\\e", "e"},
        { "\\f", "f"},
        { "\\g", "g"},
        { "\\h", "h"},
        { "\\i", "i"},
        { "\\j", "j"},
        { "\\k", "k"},
        { "\\l", "l"},
        { "\\m", "m"},
        { "\\n", "n"},
        { "\\o", "o"},
        { "\\p", "p"},
        { "\\q", "q"},
        { "\\r", "r"},
        { "\\s", "s"},
        { "\\t", "t"},
        { "\\u", "u"},
        { "\\v", "v"},
        { "\\w", "w"},
        { "\\x", "x"},
        { "\\y", "y"},
        { "\\z", "z"},
        { "\\{", "{"},
        { "\\|", "|"},
        { "\\}", "}"},
        { "\\~", "~"},
    };
    size_t count = sizeof(data)/sizeof(data[0]);

    for(size_t i=0; i<count; i++) {
        struct test_data *d = &(data[i]);
        FILE *stream = fmemopen(d->input, strlen(d->input), "r");
        lisp_object *ret = read(stream, false, '\0');
        char *result = (*ret->toString)(ret);
        TEST_ASSERT_EQUAL_STRING(d->expected, result);
        fclose(stream);
    }
}

void test_read_string(void) {
    struct test_data data[] = {
        { "\"string\"", "string"},
    };
    size_t count = sizeof(data)/sizeof(data[0]);

    for(size_t i=0; i<count; i++) {
        struct test_data *d = &(data[i]);
        FILE *stream = fmemopen(d->input, strlen(d->input), "r");
        lisp_object *ret = read(stream, false, '\0');
        char *result = (*ret->toString)(ret);
        TEST_ASSERT_EQUAL_STRING(d->expected, result);
        fclose(stream);
    }
}

void test_read_list(void) {
    struct test_data data[] = {
        { "()", "()"},
    };
    size_t count = sizeof(data)/sizeof(data[0]);

    for(size_t i=0; i<count; i++) {
        struct test_data *d = &(data[i]);
        FILE *stream = fmemopen(d->input, strlen(d->input), "r");
        lisp_object *ret = read(stream, false, '\0');
        char *result = (*ret->toString)(ret);
        TEST_ASSERT_EQUAL_STRING(d->expected, result);
        fclose(stream);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_read_integer);
    RUN_TEST(test_read_float);
    RUN_TEST(test_read_char);
    RUN_TEST(test_read_string);
    RUN_TEST(test_read_list);
    return UNITY_END();
}
