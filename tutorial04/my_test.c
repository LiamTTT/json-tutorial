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
#define EXPECT_EQ_STRING(expect, actual, alength) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")

static void test_parse_null() {
    my_value v;
    /* 这里两行替代之前v.type = MY_FALSE; */
    my_init(&v);
    my_set_boolean(&v, 0);
    EXPECT_EQ_INT(MY_PARSE_OK, my_parse(&v, "null "));
    EXPECT_EQ_INT(MY_NULL, my_get_type(&v));
    my_free(&v);
}

static void test_parse_true() {
    my_value v;
    my_init(&v);
    my_set_boolean(&v, 0);
    EXPECT_EQ_INT(MY_PARSE_OK, my_parse(&v, "true"));
    EXPECT_EQ_INT(MY_TRUE, my_get_type(&v));
    my_free(&v);
}

static void test_parse_false() {
    my_value v;
    my_init(&v);
    my_set_boolean(&v, 1);
    EXPECT_EQ_INT(MY_PARSE_OK, my_parse(&v, "false"));
    EXPECT_EQ_INT(MY_FALSE, my_get_type(&v));
    my_free(&v);
}

/* 添加数字的单元测试 */
#define TEST_NUMBER(expect, json)\
    do {\
        my_value v;\
        my_init(&v);\
        EXPECT_EQ_INT(MY_PARSE_OK, my_parse(&v, json));\
        EXPECT_EQ_INT(MY_NUMBER, my_get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, my_get_number(&v));\
        my_free(&v);\
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

    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");

}

/* 添加字符串解析单元测试 */
#define TEST_STRING(expect, json)\
    do{\
        my_value v;\
        my_init(&v);\
        EXPECT_EQ_INT(MY_PARSE_OK, my_parse(&v, json));\
        EXPECT_EQ_INT(MY_STRING, my_get_type(&v));\
        EXPECT_EQ_STRING(expect, my_get_string(&v), my_get_string_length(&v));\
        my_free(&v);\
    } while(0)

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
/* #if 0 */
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
/* #endif */
}

/* 重构测试错误的宏 */
#define TEST_ERROR(error, json)\
    do {\
        my_value v;\
        my_init(&v);\
        my_set_boolean(&v, 0);\
        EXPECT_EQ_INT(error, my_parse(&v, json));\
        EXPECT_EQ_INT(MY_NULL, my_get_type(&v));\
        my_free(&v);\
    } while(0)


static void test_parse_expect_value() {
    TEST_ERROR(MY_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(MY_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(MY_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(MY_PARSE_INVALID_VALUE, "?");
    /* invalid number */
    TEST_ERROR(MY_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(MY_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(MY_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(MY_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(MY_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(MY_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(MY_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(MY_PARSE_INVALID_VALUE, "nan");
}

static void test_parse_root_not_singular() {
    TEST_ERROR(MY_PARSE_ROOT_NOT_SINGULAR, "null x");
    /* invalid number */
    TEST_ERROR(MY_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' , 'E' , 'e' or nothing */
    TEST_ERROR(MY_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(MY_PARSE_ROOT_NOT_SINGULAR, "0x123");
}
  
static void test_parse_number_too_big() {
    TEST_ERROR(MY_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(MY_PARSE_NUMBER_TOO_BIG, "-1e309");
    TEST_ERROR(MY_PARSE_NUMBER_TOO_BIG, "1.7976931348623159E+308");
}

static void test_parse_missing_quotation_mark() {
    TEST_ERROR(MY_PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(MY_PARSE_MISS_QUOTATION_MARK, "\"aasd");
}

static void test_parse_invalid_string_escape() {
    TEST_ERROR(MY_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(MY_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(MY_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(MY_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
    TEST_ERROR(MY_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(MY_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

static void test_access_null() {
    my_value v;
    my_init(&v);
    my_set_string(&v, "a", 1);
    my_set_null(&v);
    EXPECT_EQ_INT(MY_NULL, my_get_type(&v));
    my_free(&v);
}

static void test_access_boolean() {
    my_value v;
    my_init(&v);
    my_set_boolean(&v, 1);
    EXPECT_TRUE(my_get_boolean(&v));
    my_set_boolean(&v, 0);
    EXPECT_FALSE(my_get_boolean(&v));
    my_free(&v);
}

static void test_access_number() {
    my_value v;
    my_init(&v);
    my_set_number(&v, 42.);
    EXPECT_EQ_DOUBLE(42., my_get_number(&v));
    EXPECT_EQ_INT(MY_NUMBER, my_get_type(&v));
    my_free(&v);
}

static void test_access_string() {
    my_value v;
    my_init(&v);
    my_set_string(&v, "", 0);
    EXPECT_EQ_STRING("", my_get_string(&v), my_get_string_length(&v));
    my_set_string(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", my_get_string(&v), my_get_string_length(&v));
    my_free(&v);
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();

    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
