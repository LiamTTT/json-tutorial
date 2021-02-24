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
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return MY_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = MY_NULL;
    return MY_PARSE_OK;
}

/* value = null / false / true */
/* 提示：下面代码没处理 false / true，将会是练习之一 */
static int my_parse_value(my_context* c, my_value* v) {
    switch (*c->json) {
        case 'n':  return my_parse_null(c, v);
        case '\0': return MY_PARSE_EXPECT_VALUE;
        default:   return MY_PARSE_INVALID_VALUE;
    }
}

int my_parse(my_value* v, const char* json) {
    my_context c;
    assert(v != NULL);
    c.json = json;
    v->type = MY_NULL;
    my_parse_whitespace(&c);
    return my_parse_value(&c, v);
}

my_type my_get_type(const my_value* v) {
    assert(v != NULL);
    return v->type;
}
