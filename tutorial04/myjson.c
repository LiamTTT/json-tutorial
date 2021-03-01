#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "myjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod(), malloc(), realloc(), free() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <string.h>  /* memcpy() */

#ifndef MY_PARSE_STACK_INIT_SIZE
#define MY_PARSE_STACK_INIT_SIZE 256
#endif /* MY_PARSE_STACK_INIT_SIZE */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
/* 调用函数返回void指针后强制转换为char指针取地址中的值并将其赋值为ch */
#define PUTC(c, ch)         do { *(char*)my_context_push(c, sizeof(char)) = (ch); } while(0)

typedef struct {
    const char* json;
    char* stack; /* 维护一个栈用于不定长的字符串存储 */
    size_t size, top;
} my_context;

static void* my_context_push(my_context* c, size_t size) {
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0)
            /* init size */
            c->size = MY_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)
            /* realloc size 1+0.5=1.5*/
            c->size += c->size >> 1;
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* my_context_pop(my_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);

}

static void my_parse_whitespace(my_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

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
    v->u.n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
        return MY_PARSE_NUMBER_TOO_BIG;
    v->type = MY_NUMBER;
    c->json = p;
    return MY_PARSE_OK;
}

static int my_parse_string(my_context* c, my_value* v){
    size_t head = c->top, len;
    const char* p;
    EXPECT(c, '\"');  /* the first char must be a " */
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch)
        {
        case '\"':
            len = c->top - head;
            my_set_string(v, (const char*)my_context_pop(c, len), len);
            c->json = p;
            return MY_PARSE_OK;
        case '\\':
            switch (*p++)
            {   
            case '\"': PUTC(c, '\"'); break;
            case '\\': PUTC(c, '\\'); break;
            case '/': PUTC(c, '/'); break;
            case 'b': PUTC(c, '\b'); break;
            case 'f': PUTC(c, '\f'); break;
            case 'n': PUTC(c, '\n'); break;
            case 'r': PUTC(c, '\r'); break;            
            case 't': PUTC(c, '\t'); break;            
            default:
                c->top = head;
                return MY_PARSE_INVALID_STRING_ESCAPE;
            }
            break;
        case '\0':
            c->top = head;
            return MY_PARSE_MISS_QUOTATION_MARK;
        default:
            /* validate the char */
            if ((unsigned char)ch < 0x20) { 
                c->top = head;
                return MY_PARSE_INVALID_STRING_CHAR;
            }
            /* push every single char to stack */
            PUTC(c, ch);
        }
    }
}
static int my_parse_value(my_context* c, my_value* v) {
    switch (*c->json) {
        case 'n':  return my_parse_literal(c, v, "null", MY_NULL);
        case 't':  return my_parse_literal(c, v, "true", MY_TRUE);
        case 'f':  return my_parse_literal(c, v, "false", MY_FALSE);
        default:   return my_parse_number(c, v);
        case '"':  return my_parse_string(c, v);
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
    my_parse_whitespace(&c);
    int ret;
    if ((ret = my_parse_value(&c, v)) == MY_PARSE_OK) { /* 先保证有第一个单词且正确了才能继续 */
        my_parse_whitespace(&c);  /* 老规矩先清零 */
        if (*c.json != '\0') {
            v->type = MY_NULL;  /* 这里要注意将多值的情况类型置为MYNULL */
            ret = MY_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top==0);  /* 保证全部内容解析完毕 */
    free(c.stack);
    return ret;
}

void my_free(my_value* v) {
    assert(v!=NULL);
    if (v->type == MY_STRING)
        free(v->u.s.s); 
    v->type = MY_NULL;
}

my_type my_get_type(const my_value* v) {
    assert(v != NULL);
    return v->type;
}

/* number process */
double my_get_number(const my_value* v) {
    assert(v != NULL && v->type == MY_NUMBER);
    return v->u.n;
}

void my_set_number(my_value* v, double n){
    my_free(v);
    v->u.n = n;
    v->type = MY_NUMBER;
}

/* str process */
const char* my_get_string(const my_value* v) {
    assert(v != NULL && v->type == MY_STRING);
    return v->u.s.s;
}

size_t my_get_string_length(const my_value* v){
    assert(v != NULL && v->type == MY_STRING);
    return v->u.s.len;
}

void my_set_string(my_value* v, const char* s, size_t len){
    assert(v != NULL && (s != NULL || len == 0));
    my_free(v);
    v->u.s.s = (char*)malloc(len+1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = "\0";  /* 单独添加的一个结尾空字符 */
    v->u.s.len = len;
    v->type = MY_STRING;
}
/*boolean process */
int my_get_boolean(const my_value* v){
    assert(v != NULL && (v->type == MY_FALSE || v->type == MY_TRUE));
    return v->type == MY_TRUE;
}

void my_set_boolean(my_value* v, int b){
    my_free(v);
    v->type = b ? MY_TRUE : MY_FALSE;
}
