#ifndef MYJSON_H__
#define MYJSON_H__
#include <stddef.h> /* size_t */

/* 别名 json 文件的数据类型 包括null boolean number string array object */
typedef enum { MY_NULL, MY_FALSE, MY_TRUE, MY_NUMBER, MY_STRING, MY_ARRAY, MY_OBJECT } my_type;
/* 
别名 json 树状结构的节点
加入Union结构用于精细的内存分配，因为不可能同时为数字或者字符串
*/
typedef struct {
    union
    {
        struct { char*s; size_t len;} s;
        double n;
    };
    my_type type;
} my_value;

/* 匿名枚举，等价于静态常量 */
enum {
    MY_PARSE_OK = 0,
    MY_PARSE_EXPECT_VALUE,
    MY_PARSE_INVALID_VALUE,
    MY_PARSE_ROOT_NOT_SINGULAR,

    MY_PARSE_NUMBER_TOO_BIG,

    MY_PARSE_MISS_QUOTATION_MARK,
    MY_PARSE_INVALID_STRING_ESCAPE,
    MY_PARSE_INVALID_STRING_CHAR
};


/* 功能API的声明 */
#define my_init(v) do { (v)->type=MY_NULL; }while(0) /* 初始化类型为NULL */

int my_parse(my_value* v, const char* json);
void my_free(my_value* v);
#define my_set_null(v) my_free(v)

my_type my_get_type(const my_value* v);

/* number process */
double my_get_number(const my_value* v);
void my_set_number(my_value* v, double n);
/* str process */
const char* my_get_string(const my_value* v);
size_t my_get_string_length(const my_value* v);
void my_set_string(my_value* v, const char* s, size_t len);
/*boolean process */
int my_get_boolean(const my_value* v);
void my_set_boolean(my_value* v, int b);

#endif /* MYJSON_H__ */
