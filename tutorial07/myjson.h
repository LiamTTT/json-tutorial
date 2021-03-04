#ifndef MYJSON_H__
#define MYJSON_H__
#include <stddef.h> /* size_t */

typedef enum { MY_NULL, MY_FALSE, MY_TRUE, MY_NUMBER, MY_STRING, MY_ARRAY, MY_OBJECT } my_type;

typedef struct my_value my_value;  /* 先声明后使用自身 */
typedef struct my_member my_member;

struct my_value{
    union
    {   
        struct { my_member* m; size_t size;} o;
        struct { my_value* e; size_t size;} a;
        struct { char*s; size_t len;} s;
        double n;
    } u;
    my_type type;
};

struct my_member {
    char* k;
    size_t klen;
    my_value v;
};


enum {
    MY_PARSE_OK = 0,
    MY_PARSE_EXPECT_VALUE,
    MY_PARSE_INVALID_VALUE,
    MY_PARSE_ROOT_NOT_SINGULAR,

    MY_PARSE_NUMBER_TOO_BIG,

    MY_PARSE_MISS_QUOTATION_MARK,
    MY_PARSE_INVALID_STRING_ESCAPE,
    MY_PARSE_INVALID_STRING_CHAR,
    MY_PARSE_INVALID_UNICODE_HEX,
    MY_PARSE_INVALID_UNICODE_SURROGATE,

    MY_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,

    MY_PARSE_MISS_KEY,
    MY_PARSE_MISS_COLON,
    MY_PARSE_MISS_COMMA_OR_CURLY_BRACKET
};


#define my_init(v) do { (v)->type=MY_NULL; }while(0) /* 初始化类型为NULL */

int my_parse(my_value* v, const char* json);
void my_free(my_value* v);
#define my_set_null(v) my_free(v)

my_type my_get_type(const my_value* v);

char* my_stringify(const my_value* v, size_t* length);

double my_get_number(const my_value* v);
void my_set_number(my_value* v, double n);

const char* my_get_string(const my_value* v);
size_t my_get_string_length(const my_value* v);
void my_set_string(my_value* v, const char* s, size_t len);

int my_get_boolean(const my_value* v);
void my_set_boolean(my_value* v, int b);

size_t my_get_array_size(const my_value* v);
my_value* my_get_array_element(const my_value* v, size_t index);

size_t my_get_object_size(const my_value* v);
const char* my_get_object_key(const my_value* v, size_t index);
size_t my_get_object_key_length(const my_value* v, size_t index);
my_value* my_get_object_value(const my_value* v, size_t index);

#endif /* MYJSON_H__ */
