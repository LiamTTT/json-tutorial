#ifndef MYJSON_H__
#define MYJSON_H__

/* 别名 json 文件的数据类型 包括null boolean number string array object */
typedef enum { MY_NULL, MY_FALSE, MY_TRUE, MY_NUMBER, MY_STRING, MY_ARRAY, MY_OBJECT } my_type;
/* 别名 json 树状结构的节点 */
typedef struct {
    double n;
    my_type type;
} my_value;

/* 匿名枚举，等价于静态常量 */
enum {
    MY_PARSE_OK = 0,
    MY_PARSE_EXPECT_VALUE,
    MY_PARSE_INVALID_VALUE,
    MY_PARSE_ROOT_NOT_SINGULAR,
    MY_PARSE_NUMBER_TOO_BIG
};

/* 功能API的声明 */
int my_parse(my_value* v, const char* json);

my_type my_get_type(const my_value* v);

double my_get_number(const my_value* v);

#endif /* MYJSON_H__ */
