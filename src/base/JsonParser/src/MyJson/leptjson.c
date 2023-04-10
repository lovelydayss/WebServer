#include "leptjson.h"
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* 此处定义而非头文件中实现封装 */
/* static 函数只有当前文件可见 */

/* 断言 */
#define EXPECT(c, ch)             \
	do {                          \
		assert(*c->json == (ch)); \
		c->json++;                \
	} while (0)

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

/* 减少解析函数之间传递多个参数而产生的封装 */
typedef struct {
	const char* json;
} lept_context;

/* ws = *(%x20 / %x09 / %x0A / %x0D) */
static void lept_parse_whitespace(lept_context* c);

/* null  = "null" */
static int lept_parse_null(lept_context* c, lept_value* v);

/* false  = "false" */
static int lept_parse_false(lept_context* c, lept_value* v);

/* true  = "true" */
static int lept_parse_true(lept_context* c, lept_value* v);

/* number = "number" */
static int lept_parse_number(lept_context* c, lept_value* v);

/* value = null / false / true / number */
static int lept_parse_value(lept_context* c, lept_value* v);

/********************/
/* 此后为函数具体实现 */
/********************/

int lept_parse(lept_value* v, const char* json) {

	lept_context c;
	assert(v != NULL);
	c.json = json;
	v->type = LEPT_NULL;

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

lept_type lept_get_type(const lept_value* v) { return v->type; }

double lept_get_number(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->n;
}

/*******************************/
/* 此后为本文件处定义函数具体实现 */
/*******************************/

static void lept_parse_whitespace(lept_context* c) {
	const char* p = c->json;
	while (*p == ' ' || *p == '\t' || *p == '\r')
		p++;
	c->json = p;
}

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
	v->n = strtod(c->json, NULL);

	/* 数字上界和下界 */
	if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
		return LEPT_PARSE_NUMBER_TOO_BIG;

	c->json = p;
	v->type = LEPT_NUMBER;
	return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json) {
	case 'n':
		return lept_parse_null(c, v);
	case 'f':
		return lept_parse_false(c, v);
	case 't':
		return lept_parse_true(c, v);
	case '\0':
		return LEPT_PARSE_EXPECT_VALUE;
	default:
		return lept_parse_number(c, v);
	}
}