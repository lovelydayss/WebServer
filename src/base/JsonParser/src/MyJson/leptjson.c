#include "leptjson.h"
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 此处定义而非头文件中实现封装 */
/* static 函数只有当前文件可见 */

/* 栈大小定义 */
#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

/* 断言 */
#define EXPECT(c, ch)             \
	do {                          \
		assert(*c->json == (ch)); \
		c->json++;                \
	} while (0)

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)                                        \
	do {                                                   \
		*(char*)lept_context_push(c, sizeof(char)) = (ch); \
	} while (0)

/* 减少解析函数之间传递多个参数而产生的封装 */
typedef struct {
	const char* json;
	char* stack;
	size_t size, top;
} lept_context;

/* 入栈 */
static void* lept_context_push(lept_context* c, size_t size);

/* 出栈 */
static void* lept_context_pop(lept_context* c, size_t size);

/* ws = *(%x20 / %x09 / %x0A / %x0D) */
static void lept_parse_whitespace(lept_context* c);

/* 重构 null false true */
static int lept_parse_literal(lept_context* c, lept_value* v,
                              const char* literal, lept_type type);

/* if 0 ...... #endif 禁用代码 */
#if 0
/* null  = "null" */
static int lept_parse_null(lept_context* c, lept_value* v);

/* false  = "false" */
static int lept_parse_false(lept_context* c, lept_value* v);

/* true  = "true" */
static int lept_parse_true(lept_context* c, lept_value* v);
#endif

/* number = "number" */
static int lept_parse_number(lept_context* c, lept_value* v);

/* string = "string" */
static int lept_parse_string(lept_context* c, lept_value* v);

/* value = null / false / true / number */
static int lept_parse_value(lept_context* c, lept_value* v);

/*******************************/
/* 此后为头文件中定义函数具体实现 */
/*******************************/

int lept_parse(lept_value* v, const char* json) {

	assert(v != NULL);
	lept_value_init(v);

	lept_context c;
	c.json = json;
	c.stack = NULL;
	c.size = c.top = 0;

	lept_parse_whitespace(&c);
	int ret = lept_parse_value(&c, v);

	/* 完成解析后处理，对 LEPT_PARSE_ROOT_NOT_SINGULAR 情况进行判断 */
	if (ret == LEPT_PARSE_OK) {
		lept_parse_whitespace(&c);
		if (*(c.json) != '\0') {
			/* 此时解析已经完成，需要将 v 的值置空处理掉 */
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
			v->type = LEPT_NULL;
		}
	}

	return ret;
}

void lept_free(lept_value* v) {
	assert(v != NULL);

	/* 对于 string 特殊处理 */
	if (v->type == LEPT_STRING)
		free(v->u.s.s);

	v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v) { return v->type; }

/* 获取和写入 bollean 值 */
int lept_get_boolean(const lept_value* v) {
	assert(v != NULL && (v->type == LEPT_FALSE || v->type == LEPT_TRUE));
	return v->type;
}
void lept_set_boolean(lept_value* v, int b) {

	/* lept_free() 中存在空断言 */
	/* assert(v!= NULL); */

	lept_free(v);
	v->type = b;
}

/* 获取和写入 double 值 */
double lept_get_number(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->u.n;
}
void lept_set_number(lept_value* v, double n) {

	/* lept_free() 中存在空断言 */
	/* assert(v!= NULL); */

	lept_free(v);
	v->type = LEPT_NUMBER;
	v->u.n = n;
}

/* 获取和写入 string 值 */
/* 对于读取，若要复制其深拷贝交由调用处实现 */
const char* lept_get_string(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_STRING);
	return v->u.s.s;
}
size_t lept_get_string_length(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_STRING);
	return v->u.s.len;
}
void lept_set_string(lept_value* v, const char* s, size_t len) {

	/* lept_free() 中存在空断言 */
	/* assert(v!= NULL); */
	/* 此处由于对 s 进行读，所以该断言仍然需要保留 */

	assert(v != NULL && (s != NULL || len == 0));
	lept_free(v);

	/* 必须执行深拷贝 */
	v->u.s.s = (char*)malloc(len + 1);
	memcpy(v->u.s.s, s, len);

	/* 补充尾部 '\0' 字符 */
	v->u.s.s[len] = '\0';

	v->type = LEPT_STRING;
	v->u.s.len = len;
}

/*******************************/
/* 此后为本文件处定义函数具体实现 */
/*******************************/

static void* lept_context_push(lept_context* c, size_t size) {
	void* ret;
	assert(size > 0);

	/* 栈空间不足时执行扩充 */
	if (c->top + size >= c->size) {
		if (c->size == 0)
			c->size = LEPT_PARSE_STACK_INIT_SIZE;
		while (c->top + size >= c->size)
			c->size += c->size >> 1; /* 1.5 倍扩充 */
		c->stack = (char*)realloc(c->stack, c->size);
	}

	ret = c->stack + c->top;
	c->top += size;
	return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
	assert(c->top >= size);
	return c->stack + (c->top -= size);
}

static void lept_parse_whitespace(lept_context* c) {
	const char* p = c->json;
	while (*p == ' ' || *p == '\t' || *p == '\r')
		p++;
	c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v,
                              const char* literal, lept_type type) {
	size_t i;
	EXPECT(c, literal[0]);

	for (i = 0; literal[i + 1]; i++)
		if (c->json[i] != literal[i + 1])
			return LEPT_PARSE_INVALID_VALUE;

	c->json += i;
	v->type = type;
	return LEPT_PARSE_OK;
}

#if 0
static int lept_parse_null(lept_context* c, lept_value* v) {
	EXPECT(c, 'n');
	if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
		return LEPT_PARSE_INVALID_VALUE;

	c->json += 3;
	v->type = LEPT_NULL;
	return LEPT_PARSE_OK;
}

static int lept_parse_false(lept_context* c, lept_value* v) {
	EXPECT(c, 'f');
	if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' ||
	    c->json[3] != 'e')
		return LEPT_PARSE_INVALID_VALUE;

	c->json += 4;
	v->type = LEPT_FALSE;
	return LEPT_PARSE_OK;
}

static int lept_parse_true(lept_context* c, lept_value* v) {
	EXPECT(c, 't');
	if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
		return LEPT_PARSE_INVALID_VALUE;

	c->json += 3;
	v->type = LEPT_TRUE;
	return LEPT_PARSE_OK;
}
#endif

static int lept_parse_number(lept_context* c, lept_value* v) {
	const char* p = c->json;
	/* validate number */
	if (*p == '-')
		p++;
	if (*p == '0')
		p++;
	else {
		if (!ISDIGIT1TO9(*p))
			return LEPT_PARSE_INVALID_VALUE;
		for (p++; ISDIGIT(*p); p++)
			;
	}
	if (*p == '.') {
		p++;
		if (!ISDIGIT(*p))
			return LEPT_PARSE_INVALID_VALUE;
		for (p++; ISDIGIT(*p); p++)
			;
	}
	if (*p == 'E' || *p == 'e') {
		p++;
		if (*p == '+' || *p == '-')
			p++;
		if (!ISDIGIT(*p))
			return LEPT_PARSE_INVALID_VALUE;
		for (p++; ISDIGIT(*p); p++)
			;
	}

	/* strtod endptr 指向转换后数字字符串后一个位置 */
	errno = 0;
	v->u.n = strtod(c->json, NULL);

	/* 数字上界和下界 */
	if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
		return LEPT_PARSE_NUMBER_TOO_BIG;

	c->json = p;
	v->type = LEPT_NUMBER;
	return LEPT_PARSE_OK;
}

static int lept_parse_string(lept_context* c, lept_value* v) {
	size_t head = c->top, len;
	EXPECT(c, '\"');

	const char* p;
	p = c->json;
	for (;;) {
		char ch = *p++;
		switch (ch) {
		case '\"':
			len = c->top - head;
			lept_set_string(v, (const char*)lept_context_pop(c, len), len);
			c->json = p;
			return LEPT_PARSE_OK;
		case '\0':
			c->top = head;
			return LEPT_PARSE_MISS_QUOTATION_MARK;
		default:
			PUTC(c, ch); /* 逐个字符入栈 */
		}
	}
}

static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json) {
	case 'n':
		return lept_parse_literal(c, v, "null", LEPT_NULL);
	case 'f':
		return lept_parse_literal(c, v, "false", LEPT_FALSE);
	case 't':
		return lept_parse_literal(c, v, "true", LEPT_TRUE);
	case '"':
		return lept_parse_string(c, v);
	case '\0':
		return LEPT_PARSE_EXPECT_VALUE;
	default:
		return lept_parse_number(c, v);
	}
}