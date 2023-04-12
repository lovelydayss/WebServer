#include "leptjson.h"
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>
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
#define STRING_ERROR(ret) \
	do {                  \
		c->top = head;    \
		return ret;       \
	} while (0)

/* 减少解析函数之间传递多个参数而产生的封装 */
typedef struct {
	const char* json;
	char* stack;
	size_t size, top;
} lept_context;

/* 释放 stack 空间 */
static void lept_context_free(lept_context* c);

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

/* 解析十六进制编码，转为十进制数值  */
static const char* lept_parse_hex4(const char* p, unsigned* u);

/* unicode 编码解析为 utf8 */
static void lept_encode_utf8(lept_context* c, unsigned u);

/* string = "string" */
static int lept_parse_string(lept_context* c, lept_value* v);

/* array = "array" */
static int lept_parse_array(lept_context* c, lept_value* v);

/* value = null / false / true / number */
static int lept_parse_value(lept_context* c, lept_value* v);

/*******************************/
/* 此后为头文件中定义函数具体实现 */
/*******************************/

int lept_parse(lept_value* v, const char* json) {

	assert(v != NULL);
	lept_value_init(v);

	lept_context* c = (lept_context*)malloc(sizeof(lept_context));
	c->json = json;
	c->stack = NULL;
	c->size = c->top = 0;

	lept_parse_whitespace(c);
	int ret = lept_parse_value(c, v);

	/* 完成解析后处理，对 LEPT_PARSE_ROOT_NOT_SINGULAR 情况进行判断 */
	if (ret == LEPT_PARSE_OK) {
		lept_parse_whitespace(c);
		if (*(c->json) != '\0') {
			/* 此时解析已经完成，需要将 v 的值置空处理掉 */
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
			v->type = LEPT_NULL;
		}
	}

	lept_context_free(c);

	return ret;
}

void lept_free(lept_value* v) {
	size_t i;
	assert(v != NULL);
	switch (v->type) {
	/* string 处理 */
	case LEPT_STRING:
		free(v->u.s.s);
		break;
	/* array 处理 */
	case LEPT_ARRAY:
		for (i = 0; i < v->u.a.size; i++)
			lept_free(&v->u.a.e[i]);
		free(v->u.a.e);
		break;
	default:
		break;
	}
	v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v) { return v->type; }

/* 获取和写入 bollean 值 */
int lept_get_boolean(const lept_value* v) {
	assert(v != NULL && (v->type == LEPT_FALSE || v->type == LEPT_TRUE));
	return v->type == LEPT_TRUE;
}
void lept_set_boolean(lept_value* v, int b) {

	/* lept_free() 中存在空断言 */
	/* assert(v!= NULL); */

	lept_free(v);
	v->type = (b > 0 ? LEPT_TRUE : LEPT_FALSE); /* 此处设置非 0 即为 true */
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

/* 获取 array 数组元素数和元素值 */
/* 对于读取，若要复制其深拷贝交由调用处实现 */
size_t lept_get_array_size(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_ARRAY);
	return v->u.a.size;
}

lept_value* lept_get_array_element(const lept_value* v, size_t index) {
	assert(v != NULL && v->type == LEPT_ARRAY);
	assert(index < v->u.a.size);
	return &v->u.a.e[index];
}

/*******************************/
/* 此后为本文件处定义函数具体实现 */
/*******************************/

static void lept_context_free(lept_context* c) {
	if (c != NULL && c->stack != NULL) {
		free(c->stack);
		c->stack = NULL;
	}

	if (c != NULL) {
		free(c);
		c = NULL;
	}
}

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

static const char* lept_parse_hex4(const char* p, unsigned* u) {
	int i = 0;
	*u = 0;

	/* 4 位 16 进制数字 */
	for (i = 0; i < 4; i++) {
		char ch = *p++;
		*u <<= 4;

		/* 使用位操作求解 */
		if (ch >= '0' && ch <= '9')
			*u |= ch - '0';
		else if (ch >= 'A' && ch <= 'F')
			*u |= ch - ('A' - 10);
		else if (ch >= 'a' && ch <= 'f')
			*u |= ch - ('a' - 10);
		else
			return NULL;
	}

	return p;
}

static void lept_encode_utf8(lept_context* c, unsigned u) {
	if (u <= 0x7F)
		PUTC(c, u & 0xFF);
	else if (u <= 0x7FF) {
		PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
		PUTC(c, 0x80 | (u & 0x3F));
	} else if (u <= 0xFFFF) {
		PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
		PUTC(c, 0x80 | ((u >> 6) & 0x3F));
		PUTC(c, 0x80 | (u & 0x3F));
	} else {
		assert(u <= 0x10FFFF);
		PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
		PUTC(c, 0x80 | ((u >> 12) & 0x3F));
		PUTC(c, 0x80 | ((u >> 6) & 0x3F));
		PUTC(c, 0x80 | (u & 0x3F));
	}
}

static int lept_parse_string(lept_context* c, lept_value* v) {
	size_t head = c->top, len;
	EXPECT(c, '\"');

	unsigned u, u2;
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
		case '\\':
			/* 转义字符处理 */
			switch (*p++) {
			case '\"':
				PUTC(c, '\"');
				break;
			case '\\':
				PUTC(c, '\\');
				break;
			case '/':
				PUTC(c, '/');
				break;
			case 'b':
				PUTC(c, '\b');
				break;
			case 'f':
				PUTC(c, '\f');
				break;
			case 'n':
				PUTC(c, '\n');
				break;
			case 'r':
				PUTC(c, '\r');
				break;
			case 't':
				PUTC(c, '\t');
				break;
			case 'u':
				if (!(p = lept_parse_hex4(p, &u)))
					STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
				if (u >= 0xD800 && u <= 0xDBFF) {
					if (*p++ != '\\')
						STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
					if (*p++ != 'u')
						STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
					if (!(p = lept_parse_hex4(p, &u2)))
						STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
					if (u2 < 0xDC00 || u2 > 0xDFFF)
						STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
					u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
				}
				lept_encode_utf8(c, u);
				break;
			default:
				STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);
			}
			break;
		case '\0':
			STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK);
		default:
			/* 不合法字符处理 */
			/* unescaped = %x20-21 / %x23-5B / %x5D-10FFFF */
			if ((unsigned char)ch < 0x20)
				STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);

			PUTC(c, ch);
		}
	}
}

static int lept_parse_array(lept_context* c, lept_value* v) {
	size_t i, size = 0;
	int ret;
	EXPECT(c, '[');
	lept_parse_whitespace(c);

	/* 空类型数组解析 */
	if (*c->json == ']') {
		c->json++;
		v->type = LEPT_ARRAY;
		v->u.a.size = 0;
		v->u.a.e = NULL;
		return LEPT_PARSE_OK;
	}

	for (;;) {
		lept_value e;
		lept_value_init(&e);
		ret = lept_parse_value(c, &e);
		if (ret != LEPT_PARSE_OK)
			break;

		memcpy(lept_context_push(c, sizeof(lept_value)), &e,
		       sizeof(lept_value));
		size++;
		lept_parse_whitespace(c);
		if (*c->json == ',') {
			c->json++;
			lept_parse_whitespace(c);
		} else if (*c->json == ']') {
			c->json++;
			v->type = LEPT_ARRAY;
			v->u.a.size = size;
			size *= sizeof(lept_value);
			memcpy(v->u.a.e = (lept_value*)malloc(size),
			       lept_context_pop(c, size), size);
			return LEPT_PARSE_OK;
		} else {
			ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
			break;
		}
	}

	/* 此处释放栈空间主要目的在于对栈进行复原 */
	for (i = 0; i < size; i++)
		lept_free((lept_value*)lept_context_pop(c, sizeof(lept_value)));

	return ret;
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
	case '[':
		return lept_parse_array(c, v);
	case '\0':
		return LEPT_PARSE_EXPECT_VALUE;
	default:
		return lept_parse_number(c, v);
	}
}