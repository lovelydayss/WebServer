#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <stddef.h> /* size_t */

/* Json 数值合法类型 */
typedef enum {
	LEPT_NULL,
	LEPT_FALSE,
	LEPT_TRUE,
	LEPT_NUMBER,
	LEPT_STRING,
	LEPT_ARRAY,
	LEPT_OBJECT,
} lept_type;

/* 结构体预定义 */
/* 处理递归定义问题 */
typedef struct lept_value lept_value;
typedef struct lept_member lept_member;

/* Json 值结构 */
struct lept_value {
	union {
		struct {
			lept_member* m;
			size_t size;
		} o; /* object 对象类型 */
		struct {
			lept_value* e;
			size_t size;
		} a; /* array 数组类型 */
		struct {
			char* s;
			size_t len;
		} s; /* string 类型存储字符串 */

		double n; /* 双精度浮点数存储数字 */
	} u;

	lept_type type; /* json 值类型 */
};

struct lept_member {
	char* k;
	size_t klen;  /* key string*/
	lept_value v; /* value */
};

/* Json value 值类型初始化 */
#define lept_value_init(v)     \
	do {                       \
		(v)->type = LEPT_NULL; \
	} while (0)

/* 返回类型 */
enum {
	LEPT_PARSE_OK,                /* 成功解析 */
	LEPT_PARSE_EXPECT_VALUE,      /* 空的 Json */
	LEPT_PARSE_INVALID_VALUE,     /* 非合法字面值 */
	LEPT_PARSE_ROOT_NOT_SINGULAR, /* 空白值后还有其他字符 */
	LEPT_PARSE_NUMBER_TOO_BIG, /* 数值过大双精度浮点数无法表示 */
	LEPT_PARSE_MISS_QUOTATION_MARK,          /* 缺少字符串封闭标记 */
	LEPT_PARSE_INVALID_STRING_ESCAPE,        /* 非法转义标志 */
	LEPT_PARSE_INVALID_STRING_CHAR,          /* 非法字符 */
	LEPT_PARSE_INVALID_UNICODE_HEX,          /* 非法十六进制编码  */
	LEPT_PARSE_INVALID_UNICODE_SURROGATE,    /* 非法代理对范围 */
	LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, /* 数组未闭合 */
	LEPT_PARSE_MISS_KEY,                     /* 缺少键值 */
	LEPT_PARSE_MISS_COLON,                   /* 缺少中间 : */
	LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET   /* 对象未闭合 */
};

/* json 解析函数 */
/* 传入一个 json（const char* 类型），一个解析后的 json */
/* 结构体指针（使用时分配空间） */
/* JSON-text = ws value ws */
int lept_parse(lept_value* v, const char* json);

/* Json 值类型释放 */
void lept_free(lept_value* v);

/* 获取 Json 值类型 */
lept_type lept_get_type(const lept_value* v);

/* Json 置空 */
/* null 类型不存在读取 */
#define lept_set_null(v) lept_free(v)

/* 获取和写入 bollean 值 */
int lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value* v, int b);

/* 获取和写入 double 值 */
double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double n);

/* 获取和写入 string 值 */
const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);

/* 对于数组和对象类型的写入未实现？ */

/* 获取数组 */
size_t lept_get_array_size(const lept_value* v);
lept_value* lept_get_array_element(const lept_value* v, size_t index);

/* 获取 Json 对象 */
size_t lept_get_object_size(const lept_value* v);
const char* lept_get_object_key(const lept_value* v, size_t index);
size_t lept_get_object_key_length(const lept_value* v, size_t index);
lept_value* lept_get_object_value(const lept_value* v, size_t index);

#endif /* LEPTJSON_H__ */