#include "leptjson.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

/* 此处多于一个语句使用 do while 包裹宏语句 */
#define EXPECT_EQ_BASE(equality, expect, actual, format)                      \
	do {                                                                      \
		test_count++;                                                         \
		if (equality)                                                         \
			test_pass++;                                                      \
		else {                                                                \
			fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", \
			        __FILE__, __LINE__, expect, actual);                      \
			main_ret = 1;                                                     \
		}                                                                     \
	} while (0)

#define EXPECT_EQ_INT(expect, actual) \
	EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

#define EXPECT_EQ_DOUBLE(expect, actual) \
	EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%f")

/* 对于 null, false, true 类型测试用例的重构 */
#define TEST_SINGLE(expect_lept_type, json)                 \
	do {                                                    \
		lept_value v;                                       \
		v.type = LEPT_TYPE_TEST_INIT;                       \
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json)); \
		EXPECT_EQ_INT(expect_lept_type, lept_get_type(&v)); \
	} while (0)

/* 对于数字类型测试用例的扩展宏 */
#define TEST_NUMBER(expect, json)                           \
	do {                                                    \
		lept_value v;                                       \
		v.type = LEPT_TYPE_TEST_INIT;                       \
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json)); \
		EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));      \
		EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));      \
	} while (0)

/* 对于 error 类型测试用例的重构 */
#define TEST_ERROR(expect_error_type, json)                     \
	do {                                                        \
		lept_value v;                                           \
		v.type = LEPT_TYPE_TEST_INIT;                           \
		EXPECT_EQ_INT(expect_error_type, lept_parse(&v, json)); \
	} while (0)

/***********************************************************
************************************************************
************************************************************/
static void test_parse_single() {

	TEST_SINGLE(LEPT_NULL, "null");
	TEST_SINGLE(LEPT_FALSE, "false");
	TEST_SINGLE(LEPT_TRUE, "true");
}

static void test_parse_number() {

	TEST_NUMBER(0.0, "0");
	TEST_NUMBER(0.0, "-0");
	TEST_NUMBER(0.0, "-0.0");
	TEST_NUMBER(1.0, "1");
	TEST_NUMBER(-1.0, "-1");
	TEST_NUMBER(1.5, "1.5");
	TEST_NUMBER(-1.5, "-1.5");
	TEST_NUMBER(3.1416, "3.1416");
	TEST_NUMBER(1E10, "1E10");
	TEST_NUMBER(1e10, "1e10");
	TEST_NUMBER(1E+10, "1E+10");
	TEST_NUMBER(1E-10, "1E-10");
	TEST_NUMBER(-1E10, "-1E10");
	TEST_NUMBER(-1e10, "-1e10");
	TEST_NUMBER(-1E+10, "-1E+10");
	TEST_NUMBER(-1E-10, "-1E-10");
	TEST_NUMBER(1.234E+10, "1.234E+10");
	TEST_NUMBER(1.234E-10, "1.234E-10");
	TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

	/* 边界值 */

	/* the smallest number > 1 */
	TEST_NUMBER(1.0000000000000002, "1.0000000000000002");

	/* minimum denormal */
	TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324");
	TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");

	/* Max subnormal double */
	TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");
	TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");

	/* Min normal positive double */
	TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");
	TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");

	/* Max double */
	TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308");
	TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

static void test_parse_expect_value() {

	TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
	TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {

	/* #if 0 ... #endif 实现代码禁用 */

	/* invalid single value test */
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "dahsjkdhasjkd");

	/* invalid number test */
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE,
	           ".123"); /* at least one digit before '.' */
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE,
	           "1."); /* at least one digit after '.' */
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");
}

static void test_parse_root_not_singular() {

	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, " false ok  ");
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, " true ok  ");

	/* not singular number test */
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR,
	           "0123"); /* after zero should be '.' , 'E' , 'e' or nothing */
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x0");
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big() {

	TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "1e309");
	TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse() {

	/* null false true */
	test_parse_single();

	/* number */
	test_parse_number();

	/* LEPT_PARSE_EXPECT_VALUE */
	test_parse_expect_value();

	/* LEPT_PARSE_INVALID_VALUE */
	test_parse_invalid_value();

	/* LEPT_PARSE_ROOT_NOT_SINGULAR */
	test_parse_root_not_singular();

	/* LEPT_PARSE_NUMBER_TOO_BIG */
	test_parse_number_too_big();
}

int main() {
	test_parse();
	printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count,
	       test_pass * 100.0 / test_count);
	return main_ret;
}
