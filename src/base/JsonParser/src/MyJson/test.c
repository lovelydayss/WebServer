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
#define EXPECT_EQ_STRING(expect, actual, alength)            \
	EXPECT_EQ_BASE(sizeof(expect) - 1 == (alength) &&        \
	                   memcmp(expect, actual, alength) == 0, \
	               expect, actual, "%s")
#define EXPECT_TRUE(actual) \
	EXPECT_EQ_BASE((!actual) == 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) \
	EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")

/* 对于 null, false, true 类型测试用例的重构 */
#define TEST_BOLLEAN(expect_lept_type, json)                \
	do {                                                    \
		lept_value v;                                       \
		lept_value_init(&v);                                \
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json)); \
		EXPECT_EQ_INT(expect_lept_type, lept_get_type(&v)); \
	} while (0)

/* 对于 number 类型测试用例的扩展宏 */
#define TEST_NUMBER(expect, json)                           \
	do {                                                    \
		lept_value v;                                       \
		lept_value_init(&v);                                \
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json)); \
		EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));      \
		EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));      \
	} while (0)
/* 对于 string 类型测试用例的扩展宏*/
#define TEST_STRING(expect, json)                           \
	do {                                                    \
		lept_value v;                                       \
		lept_value_init(&v);                                \
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json)); \
		EXPECT_EQ_INT(LEPT_STRING, lept_get_type(&v));      \
		EXPECT_EQ_STRING(expect, lept_get_string(&v),       \
		                 lept_get_string_length(&v));       \
		lept_free(&v);                                      \
	} while (0)

/* 对于 error 类型测试用例的重构 */
#define TEST_ERROR(expect_error_type, json)                     \
	do {                                                        \
		lept_value v;                                           \
		v.type = LEPT_NULL;                                     \
		EXPECT_EQ_INT(expect_error_type, lept_parse(&v, json)); \
	} while (0)

/***********************************************************
************************************************************
************************************************************/
static void test_parse_boolean() {

	TEST_BOLLEAN(LEPT_NULL, "null");
	TEST_BOLLEAN(LEPT_FALSE, "false");
	TEST_BOLLEAN(LEPT_TRUE, "true");
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

static void test_parse_string() {
	TEST_STRING("", "\"\"");
	TEST_STRING("Hello", "\"Hello\"");

	/* 转义序列测试 */
	TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
	TEST_STRING("\" \\ / \b \f \n \r \t",
	            "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
}

static void test_parse_expect_value() {

	TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
	TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {

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

static void test_parse_miss_quotation_mark() {
	TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"");
	TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {

	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {

	TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

/*******************************/
/* 此后为头文件中定义接口函数测试 */
/*******************************/

/* lept_parse() */
static void test_parse() {

	/* LEPT_PARSE_OK */
	{
		/* null false true */
		test_parse_boolean();

		/* number */
		test_parse_number();

		/* string */
		test_parse_string();
	}

	/* LEPT_PARSE_EXPECT_VALUE */
	test_parse_expect_value();

	/* LEPT_PARSE_INVALID_VALUE */
	test_parse_invalid_value();

	/* LEPT_PARSE_ROOT_NOT_SINGULAR */
	test_parse_root_not_singular();

	/* LEPT_PARSE_NUMBER_TOO_BIG */
	test_parse_number_too_big();

	/* LEPT_PARSE_MISS_QUOTATION_MARK */
	test_parse_miss_quotation_mark();

	/* LEPT_PARSE_INVALID_STRING_ESCAPE */
	test_parse_invalid_string_escape();

	/* LEPT_PARSE_INVALID_STRING_CHAR */
	test_parse_invalid_string_char();
}

static void test_access_null() {
	lept_value v;
	lept_value_init(&v);
	lept_set_string(&v, "a", 1);
	lept_set_null(&v);
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
	lept_free(&v);
}

static void test_access_boolean() {
	lept_value v;
	lept_value_init(&v);
	lept_set_string(&v, "a", 1);

	lept_set_boolean(&v, LEPT_TRUE);
	EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
	EXPECT_TRUE(lept_get_boolean(&v));

	lept_set_boolean(&v, LEPT_FALSE);
	EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));
	EXPECT_TRUE(lept_get_boolean(&v));

	lept_free(&v);
}

static void test_access_number() {
	lept_value v;
	lept_value_init(&v);
	lept_set_string(&v, "a", 1);

	lept_set_number(&v, (double)1234.66);
	EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));
	EXPECT_EQ_DOUBLE((double)1234.66, lept_get_number(&v));

	lept_set_number(&v, (double)-1234.66);
	EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));
	EXPECT_EQ_DOUBLE((double)1234.66, lept_get_number(&v));

	lept_free(&v);
}

static void test_access_string() {
	lept_value v;
	lept_value_init(&v);
	lept_set_string(&v, "", 0);
	EXPECT_EQ_STRING("", lept_get_string(&v), lept_get_string_length(&v));
	lept_set_string(&v, "Hello", 5);
	EXPECT_EQ_STRING("Hello", lept_get_string(&v), lept_get_string_length(&v));
	lept_free(&v);
}

/***************/
/* 总的测试函数 */
/***************/

static void test() {

	test_parse();

	/* 其余接口测试 */
	test_access_null();
	test_access_boolean();
	test_access_number();
	test_access_string();
}

int main() {
	test();
	printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count,
	       test_pass * 100.0 / test_count);
	return main_ret;
}
