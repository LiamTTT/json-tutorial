#include "myjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

typedef struct {
    const char* json;
} my_context;

/* ws = *(%x20 / %x09 / %x0A / %x0D) */
static void my_parse_whitespace(my_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

/* null  = "null" */
static int my_parse_null(my_context* c, my_value* v) {
    EXPECT(c, 'n'); /* 这一步用于保证首字母的正确 */
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return MY_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = MY_NULL;
    return MY_PARSE_OK;
}

/* true = "true" */
static int my_parse_true(my_context* c, my_value* v) {
    EXPECT(c, 't');
    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
        return MY_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = MY_TRUE;
    return MY_PARSE_OK;
}

/* false = "false" */
static int my_parse_false(my_context* c, my_value* v) {
    EXPECT(c, 'f');
    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
        return MY_PARSE_INVALID_VALUE;
    c->json += 4;
    v->type = MY_FALSE;
    return MY_PARSE_OK;
}

/* value = null / false / true */
/* 提示：下面代码没处理 false / true，将会是练习之一 */
static int my_parse_value(my_context* c, my_value* v) {
    switch (*c->json) {
        case 'n':  return my_parse_null(c, v);
        case 't':  return my_parse_true(c, v);
        case 'f':  return my_parse_false(c, v);
        case '\0': return MY_PARSE_EXPECT_VALUE;
        default:   return MY_PARSE_INVALID_VALUE;
    }
}

/* 提示：这里应该是 JSON-text = ws value ws */
/* 以下实现没处理最后的 ws 和 LEPT_PARSE_ROOT_NOT_SINGULAR */
int my_parse(my_value* v, const char* json) {
    my_context c;  /* 创建传递内容的结构体 */
    assert(v != NULL);  /* 保证指针不为空 */
    c.json = json;
    v->type = MY_NULL;
    my_parse_whitespace(&c);  /* 放在这里用于略去头部ws */
    /* return my_parse_value(&c, v); */
    /* 添加处理最后的ws和LEPT_PARSE_ROOT_NOT_SINGULAR */
    int ret;
    if ((ret = my_parse_value(&c, v)) == MY_PARSE_OK) { /* 先保证有第一个单词且正确了才能继续 */
        my_parse_whitespace(&c);  /* 老规矩先清零 */
        if (*c.json != '\0')
            ret = MY_PARSE_ROOT_NOT_SINGULAR;
    }
    return ret;
}

my_type my_get_type(const my_value* v) {
    assert(v != NULL);
    return v->type;
}
