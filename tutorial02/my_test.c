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
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")

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

/* 减少重复的代码 */
#define TEST_ERROR(error, json)\
    do {\
        my_value v;\
        v.type = MY_FALSE;\
        EXPECT_EQ_INT(error, my_parse(&v, json));\
        EXPECT_EQ_INT(MY_NULL, my_get_type(&v));\
    } while(0)
    
static void test_parse_expect_value() {
    TEST_ERROR(MY_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(MY_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(MY_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(MY_PARSE_INVALID_VALUE, "?");
}

static void test_parse_root_not_singular() {
    TEST_ERROR(MY_PARSE_ROOT_NOT_SINGULAR, "null x");
}

/* 添加数字的单元测试 */
#define TEST_NUMBER(expect, json)\
    do {\
        my_value v;\
        EXPECT_EQ_INT(MY_PARSE_OK, my_parse(&v, json));\
        EXPECT_EQ_INT(MY_NUMBER, my_get_type(&v);\
        EXPECT_EQ_DOUBLE(expect, my_get_number(&v));\
    } while(0)
    
static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */
}

static void test_parse_invalid_value() {
    /* ... */
    /* invalid number */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");
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
