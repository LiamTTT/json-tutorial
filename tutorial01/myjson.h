#ifndef MYJSON_H__
#define MYJSON_H__

// 别名 json 文件的数据类型 包括null boolean number string array object
typedef enum { MY_NULL, MY_FALSE, MY_TRUE, MY_NUMBER, MY_STRING, MY_ARRAY, MY_OBJECT } my_type;
// 别名 json 树状结构的节点
typedef struct {
    my_type type;
} my_value;
// ??
enum {
    MY_PARSE_OK = 0,
    MY_PARSE_EXPECT_VALUE,
    MY_PARSE_INVALID_VALUE,
    MY_PARSE_ROOT_NOT_SINGULAR
};
// 功能API的声明
int my_parse(my_value* v, const char* json);  // 解析json

my_type my_get_type(const my_value* v);  // 获取类型

#endif /* MYJSON_H__ */
