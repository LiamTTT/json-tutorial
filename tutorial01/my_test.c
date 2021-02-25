#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "myjson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;
/* 
使用宏以保证能够正确的指出出现错误的位置，若使用函数或者内联函数，那么每次错误提示
的行号将会是指向相同的函数内部，没有提示的作用。effective cpp里推荐做法是尽量不要
使用宏，因为容易出现难发现的错误，但这里是单元测试设计需要，使用宏将更方便
 */
#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

static void test_parse_null() {
    my_value v;
    v.type = MY_FALSE;  /* 这里是传入一个默认值为MY_NULL，在经过my_parse后变成对应值 */
    EXPECT_EQ_INT(MY_PARSE_OK, my_parse(&v, "null "));
    EXPECT_EQ_INT(MY_NULL, my_get_type(&v));
}

/* 加入true和false的单元测试 */
static void test_parse_true() {
    my_value v;
    v.type = MY_FALSE;
    EXPECT_EQ_INT(MY_PARSE_OK, my_parse(&v, "true"));
    EXPECT_EQ_INT(MY_TRUE, my_get_type(&v));
}

static void test_parse_false() {
    my_value v;
    v.type = MY_FALSE;
    EXPECT_EQ_INT(MY_PARSE_OK, my_parse(&v, "false"));
    EXPECT_EQ_INT(MY_FALSE, my_get_type(&v));
}

static void test_parse_expect_value() {
    my_value v;

    v.type = MY_FALSE;
    EXPECT_EQ_INT(MY_PARSE_EXPECT_VALUE, my_parse(&v, ""));
    EXPECT_EQ_INT(MY_NULL, my_get_type(&v));

    v.type = MY_FALSE;
    EXPECT_EQ_INT(MY_PARSE_EXPECT_VALUE, my_parse(&v, " "));
    EXPECT_EQ_INT(MY_NULL, my_get_type(&v));
}

static void test_parse_invalid_value() {
    my_value v;
    v.type = MY_FALSE;
    EXPECT_EQ_INT(MY_PARSE_INVALID_VALUE, my_parse(&v, "nul"));
    EXPECT_EQ_INT(MY_NULL, my_get_type(&v));

    v.type = MY_FALSE;
    EXPECT_EQ_INT(MY_PARSE_INVALID_VALUE, my_parse(&v, "?"));
    EXPECT_EQ_INT(MY_NULL, my_get_type(&v));
}

static void test_parse_root_not_singular() {
    my_value v;
    v.type = MY_FALSE;
    EXPECT_EQ_INT(MY_PARSE_ROOT_NOT_SINGULAR, my_parse(&v, "null x"));
    EXPECT_EQ_INT(MY_NULL, my_get_type(&v));
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
