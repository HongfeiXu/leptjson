#ifndef LEPTJSON_H__		// include guide
#define LEPTJSON_H__

// JSON 中有 6 种数据类型，如果把 true 和 false 当作两个类型就是 7 种
// 因为 C 语言没有 C++ 的命名空间（namespace）功能，一般会使用项目的简写作为标识符的前缀。
// 通常枚举值用全大写（如 LEPT_NULL），而类型及函数则用小写（如 lept_type）。
// enum 枚举名{ 枚举值表 }; 在枚举值表中应罗列出所有可用值。这些值也称为枚举元素。
typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

// 声明 JSON 的数据结构
typedef struct {
    lept_type type;
}lept_value;

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR
};

int lept_parse(lept_value* v, const char* json);

lept_type lept_get_type(const lept_value* v);

#endif /* LEPTJSON_H__ */
