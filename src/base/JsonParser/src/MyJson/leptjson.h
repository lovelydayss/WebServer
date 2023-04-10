#ifndef LEPTJSON_H__
#define LEPTJSON_H__

/* Json 数值合法类型 */
typedef enum {
	LEPT_TYPE_TEST_INIT,
	LEPT_NULL,
	LEPT_FALSE,
	LEPT_TRUE,
	LEPT_NUMBER,
	LEPT_STRING,
	LEPT_ARRAY,
	LEPT_OBJECT,
} lept_type;

typedef struct {
	double n; /* 双精度浮点数存储数字 */
	lept_type type;
} lept_value;

/* 返回类型 */
enum {
	LEPT_PARSE_TEST_INIT,     /*用于测试初始赋值*/
	LEPT_PARSE_OK,            /* 成功解析 */
	LEPT_PARSE_EXPECT_VALUE,  /* 空的 Json */
	LEPT_PARSE_INVALID_VALUE, /* 非合法字面值 */
	LEPT_PARSE_ROOT_NOT_SINGULAR,/* 空白值后还有其他字符 */
	LEPT_PARSE_NUMBER_TOO_BIG /* 数值过大双精度浮点数无法表示 */
};

/* json 解析函数 */
/* 传入一个 json（const char* 类型），一个解析后的 json */
/* 结构体指针（使用时分配空间） */
/* JSON-text = ws value ws */
int lept_parse(lept_value* v, const char* json);

/* 获取类型 */
lept_type lept_get_type(const lept_value* v);

/* 获取 double 数值 */
double lept_get_number(const lept_value* v);

#endif /* LEPTJSON_H__ */