#include "myjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod(), malloc(), realloc(), free() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <string.h>  /* memcpy() */

#ifndef MY_PARSE_STACK_INIT_SIZE
#define MY_PARSE_STACK_INIT_SIZE = 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
    char* stack; /* 维护一个栈用于不定长的字符串存储 */
    size_t size, top;
} my_context;

/* ws = *(%x20 / %x09 / %x0A / %x0D) */
static void my_parse_whitespace(my_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

/* 重构成literal版本 */
static int my_parse_literal(my_context* c, my_value* v, const char* literal, my_type type) {
    EXPECT(c, literal[0]); /* 这一步用于保证首字母的正确 */
    size_t i;
    for (i=0; literal[i+1]; ++i)
        if (c->json[i] != literal[i+1])
            return MY_PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return MY_PARSE_OK;
}

/* 添加解析数字的函数 */
static int my_parse_number(my_context* c, my_value* v) {
    const char* p = c->json;  /* 仅用作校验，不改变值 */
    /* validation */
    if (*p == '-') p++;
    if (*p == '0') p++;
    else {
        if (!ISDIGIT1TO9(*p)) return MY_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return MY_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return MY_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    /* end validation */
    errno = 0;
    v->n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
        return MY_PARSE_NUMBER_TOO_BIG;
    v->type = MY_NUMBER;
    c->json = p;
    return MY_PARSE_OK;
}

static int my_parse_value(my_context* c, my_value* v) {
    switch (*c->json) {
        case 'n':  return my_parse_literal(c, v, "null", MY_NULL);
        case 't':  return my_parse_literal(c, v, "true", MY_TRUE);
        case 'f':  return my_parse_literal(c, v, "false", MY_FALSE);
        default:   return my_parse_number(c, v);
        case '\0': return MY_PARSE_EXPECT_VALUE;
    }
}

int my_parse(my_value* v, const char* json) {
    my_context c;  /* 创建传递内容的结构体 */
    assert(v != NULL);  /* 保证指针不为空 */
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    my_init(v);
    v->type = MY_NULL;  /* 错误时默认返回为MY_NULL */
    my_parse_whitespace(&c);  /* 放在这里用于略去头部ws */
    /* return my_parse_value(&c, v); */
    /* 添加处理最后的ws和LEPT_PARSE_ROOT_NOT_SINGULAR */
    int ret;
    if ((ret = my_parse_value(&c, v)) == MY_PARSE_OK) { /* 先保证有第一个单词且正确了才能继续 */
        my_parse_whitespace(&c);  /* 老规矩先清零 */
        if (*c.json != '\0') {
            v->type = MY_NULL;  /* 这里要注意将多值的情况类型置为MYNULL */
            ret = MY_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

my_type my_get_type(const my_value* v) {
    assert(v != NULL);
    return v->type;
}

double my_get_number(const my_value* v) {
    assert(v != NULL && v->type == MY_NUMBER);
    return v->n;
}