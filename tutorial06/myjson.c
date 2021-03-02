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
#define STRING_ERROR(ret)   do {c->top = head; return ret;} while(0)

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

static const char* my_parse_hex4(const char* p, unsigned* u) {
    int i;
    *u = 0;  /* init 0 */
    for (i=0; i<4; ++i) {
        char ch = *p++;
        *u <<= 4;  /* 十六进制 左移2^4 左移四位为进位 */
        if (ch>='0' && ch <= '9') *u |= ch - '0';
        else if (ch>='A' && ch <= 'F') *u |= ch - 'A' + 10;
        else if (ch>='a' && ch <= 'f') *u |= ch - 'a' + 10;
        else return NULL;
    }
    return p;
}

static void my_encode_utf8(my_context* c, unsigned u){
    if (u <= 0x7F)
        PUTC(c, u & 0xFF);
    else if (u <= 0x7FF) {
        PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
        PUTC(c, 0x80 | ( u       & 0x3F));
    }
    else if (u <= 0xFFFF) {
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
    else {
        assert(u <= 0x10FFFF);
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
}
/* 提取方法 */
static int my_parse_string_raw(my_context* c, char** str, size_t* len) {
    size_t head = c->top;
    unsigned u, u2;
    const char* p;
    EXPECT(c, '\"');  /* the first char must be a " */
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch)
        {
        case '\"':
            *len = c->top - head;
            *str = my_context_pop(c, *len);  /* 此处由set string 变换为直接赋值给string*/
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
            case 'u':
                if (!(p = my_parse_hex4(p, &u)))
                    STRING_ERROR(MY_PARSE_INVALID_UNICODE_HEX);
                if (u >= 0xD800 && u <= 0xDBFF) {
                    /* 代理对处理 
                    如果在0xD800-0xDBFF之间就认为是高代理项，开始解析后面的转义字符
                    */
                    if (*p++ != '\\')
                        STRING_ERROR(MY_PARSE_INVALID_UNICODE_SURROGATE);
                    if (*p++ != 'u')
                        STRING_ERROR(MY_PARSE_INVALID_UNICODE_SURROGATE);
                    if (!(p = my_parse_hex4(p, &u2)))
                        STRING_ERROR(MY_PARSE_INVALID_UNICODE_HEX);
                    if (u2 < 0xDC00 || u2 > 0xDFFF)
                        STRING_ERROR(MY_PARSE_INVALID_UNICODE_SURROGATE);
                    /* 左移6个 */
                    u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                }
                my_encode_utf8(c, u);
                break;
            default:
                STRING_ERROR(MY_PARSE_INVALID_STRING_ESCAPE);
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
/* 提取方法为my_parse_string_raw() */
static int my_parse_string(my_context* c, my_value* v) {
    int ret;
    size_t len;
    char* str;
    if ((ret = my_parse_string_raw(c, &str, &len)) == MY_PARSE_OK)
        my_set_string(v, str, len);
    return ret;
}

static int my_parse_array(my_context* c, my_value* v);

static int my_parse_value(my_context* c, my_value* v) {
    switch (*c->json) {
        case 'n':  return my_parse_literal(c, v, "null", MY_NULL);
        case 't':  return my_parse_literal(c, v, "true", MY_TRUE);
        case 'f':  return my_parse_literal(c, v, "false", MY_FALSE);
        default:   return my_parse_number(c, v);
        case '"':  return my_parse_string(c, v);
        case '[':  return my_parse_array(c, v);
        case '\0': return MY_PARSE_EXPECT_VALUE;
    }
}

static int my_parse_array(my_context* c, my_value* v) {
    size_t size = 0;
    int ret;
    EXPECT(c, '[');
    my_parse_whitespace(c);
    /*  empty */
    if (*c->json == ']') {
        v->type = MY_ARRAY;
        v->u.a.size = 0;
        v->u.a.e = NULL;
        c->json++;
        return MY_PARSE_OK;
    }
    /* recursive parse */
    while (1)
    {
        my_value elem;
        my_init(&elem);
        /* error */
        if ((ret = my_parse_value(c, &elem)) != MY_PARSE_OK)
            break;
        /* push stack */
        /* 这一步是为了防止在压栈的时候出现重新分配，导致错误 */
        memcpy(my_context_push(c, sizeof(my_value)), &elem, sizeof(my_value));
        ++size;
        my_parse_whitespace(c);
        if (*c->json == ',') {
            c->json++;
            my_parse_whitespace(c);
        }
        else if (*c->json == ']') {
            v->type = MY_ARRAY;
            v->u.a.size = size;
            c->json++;
            size *= sizeof(my_value); /* 获取当前元素的占用大小 */
            memcpy(v->u.a.e = (my_value*)malloc(size), my_context_pop(c, size), size);
            return MY_PARSE_OK;  /* 成功完成解析的时候是不需要释放内存的 */
        }
        else {
            ret = MY_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
    }
    size_t i;
    for (i=0; i<size; ++i)
        my_free((my_value*)my_context_pop(c, sizeof(my_value)));
    return ret;
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
    size_t i;
    switch (v->type)
    {
    case MY_STRING:
        free(v->u.s.s); 
        break;
    case MY_ARRAY:
        /* 对于数组而言需要逐一释放元素所分配的内存 */
        for (i = 0; i < v->u.a.size; ++i)
            my_free(&v->u.a.e[i]);
        free(v->u.a.e);
        break;
    default:
        break;
    }
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

/* array process */
size_t my_get_array_size(const my_value* v) {
    assert(v!=NULL && v->type == MY_ARRAY);
    return v->u.a.size;
}

my_value* my_get_array_element(const my_value* v, size_t index) {
    assert(index < my_get_array_size(v));
    return &v->u.a.e[index];
}

/* obj process */
size_t my_get_object_size(const my_value* v) {
    assert(v!=NULL && v->type == MY_OBJECT);
    return v->u.o.size;
}
const char* my_get_object_key(const my_value* v, size_t index) {
    assert(index < my_get_object_size(v));
    return (v->u.o.m)[index].k;
}
size_t my_get_object_key_lenth(const my_value* v, size_t index) {
    assert(index < my_get_object_size(v));
    return (v->u.o.m)[index].klen;  
}
my_value* my_get_object_value(const my_value* v, size_t index) {
    assert(index < my_get_object_size(v));
    return &(v->u.o.m)[index].v;
}
