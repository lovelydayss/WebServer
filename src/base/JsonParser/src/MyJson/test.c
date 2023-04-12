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

/* size_t 类型测试用例扩展宏 */
#define EXPECT_EQ_SIZE_T(expect, actual) \
	EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu")
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
	TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
	TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
	TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
	TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
	TEST_STRING("\xF0\x9D\x84\x9E",
	            "\"\\uD834\\uDD1E\""); /* G clef sign U+1D11E */
	TEST_STRING("\xF0\x9D\x84\x9E",
	            "\"\\ud834\\udd1e\""); /* G clef sign U+1D11E */
}

static void test_parse_array() {
	size_t i, j;
	lept_value v;

	lept_value_init(&v);
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ ]"));
	EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
	EXPECT_EQ_SIZE_T(0, lept_get_array_size(&v));
	lept_free(&v);

	lept_value_init(&v);
	EXPECT_EQ_INT(LEPT_PARSE_OK,
	              lept_parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
	EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
	EXPECT_EQ_SIZE_T(5, lept_get_array_size(&v));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(lept_get_array_element(&v, 0)));
	EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(lept_get_array_element(&v, 1)));
	EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(lept_get_array_element(&v, 2)));
	EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(lept_get_array_element(&v, 3)));
	EXPECT_EQ_INT(LEPT_STRING, lept_get_type(lept_get_array_element(&v, 4)));
	EXPECT_EQ_DOUBLE(123.0, lept_get_number(lept_get_array_element(&v, 3)));
	EXPECT_EQ_STRING("abc", lept_get_string(lept_get_array_element(&v, 4)),
	                 lept_get_string_length(lept_get_array_element(&v, 4)));
	lept_free(&v);

	lept_value_init(&v);
	EXPECT_EQ_INT(
	    LEPT_PARSE_OK,
	    lept_parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
	EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
	EXPECT_EQ_SIZE_T(4, lept_get_array_size(&v));
	for (i = 0; i < 4; i++) {
		lept_value* a = lept_get_array_element(&v, i);
		EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(a));
		EXPECT_EQ_SIZE_T(i, lept_get_array_size(a));
		for (j = 0; j < i; j++) {
			lept_value* e = lept_get_array_element(a, j);
			EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(e));
			EXPECT_EQ_DOUBLE((double)j, lept_get_number(e));
		}
	}
	lept_free(&v);
}

static void test_parse_object() {
	lept_value v;
	size_t i;

	lept_value_init(&v);
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, " { } "));
	EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(&v));
	EXPECT_EQ_SIZE_T(0, lept_get_object_size(&v));
	lept_free(&v);

	lept_value_init(&v);
	EXPECT_EQ_INT(LEPT_PARSE_OK,
	              lept_parse(&v, " { "
	                             "\"n\" : null , "
	                             "\"f\" : false , "
	                             "\"t\" : true , "
	                             "\"i\" : 123 , "
	                             "\"s\" : \"abc\", "
	                             "\"a\" : [ 1, 2, 3 ],"
	                             "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
	                             " } "));
	EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(&v));
	EXPECT_EQ_SIZE_T(7, lept_get_object_size(&v));
	EXPECT_EQ_STRING("n", lept_get_object_key(&v, 0),
	                 lept_get_object_key_length(&v, 0));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(lept_get_object_value(&v, 0)));
	EXPECT_EQ_STRING("f", lept_get_object_key(&v, 1),
	                 lept_get_object_key_length(&v, 1));
	EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(lept_get_object_value(&v, 1)));
	EXPECT_EQ_STRING("t", lept_get_object_key(&v, 2),
	                 lept_get_object_key_length(&v, 2));
	EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(lept_get_object_value(&v, 2)));
	EXPECT_EQ_STRING("i", lept_get_object_key(&v, 3),
	                 lept_get_object_key_length(&v, 3));
	EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(lept_get_object_value(&v, 3)));
	EXPECT_EQ_DOUBLE(123.0, lept_get_number(lept_get_object_value(&v, 3)));
	EXPECT_EQ_STRING("s", lept_get_object_key(&v, 4),
	                 lept_get_object_key_length(&v, 4));
	EXPECT_EQ_INT(LEPT_STRING, lept_get_type(lept_get_object_value(&v, 4)));
	EXPECT_EQ_STRING("abc", lept_get_string(lept_get_object_value(&v, 4)),
	                 lept_get_string_length(lept_get_object_value(&v, 4)));
	EXPECT_EQ_STRING("a", lept_get_object_key(&v, 5),
	                 lept_get_object_key_length(&v, 5));
	EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(lept_get_object_value(&v, 5)));
	EXPECT_EQ_SIZE_T(3, lept_get_array_size(lept_get_object_value(&v, 5)));
	for (i = 0; i < 3; i++) {
		lept_value* e = lept_get_array_element(lept_get_object_value(&v, 5), i);
		EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(e));
		EXPECT_EQ_DOUBLE(i + 1.0, lept_get_number(e));
	}
	EXPECT_EQ_STRING("o", lept_get_object_key(&v, 6),
	                 lept_get_object_key_length(&v, 6));
	{
		lept_value* o = lept_get_object_value(&v, 6);
		EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(o));
		for (i = 0; i < 3; i++) {
			lept_value* ov = lept_get_object_value(o, i);
			EXPECT_TRUE('1' + i == lept_get_object_key(o, i)[0]);
			EXPECT_EQ_SIZE_T(1, lept_get_object_key_length(o, i));
			EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(ov));
			EXPECT_EQ_DOUBLE(i + 1.0, lept_get_number(ov));
		}
	}
	lept_free(&v);
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

static void test_parse_invalid_unicode_hex() {
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
}

static void test_parse_invalid_unicode_surrogate() {
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse_miss_comma_or_square_bracket() {
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

static void test_parse_miss_key() {
	TEST_ERROR(LEPT_PARSE_MISS_KEY, "{:1,");
	TEST_ERROR(LEPT_PARSE_MISS_KEY, "{1:1,");
	TEST_ERROR(LEPT_PARSE_MISS_KEY, "{true:1,");
	TEST_ERROR(LEPT_PARSE_MISS_KEY, "{false:1,");
	TEST_ERROR(LEPT_PARSE_MISS_KEY, "{null:1,");
	TEST_ERROR(LEPT_PARSE_MISS_KEY, "{[]:1,");
	TEST_ERROR(LEPT_PARSE_MISS_KEY, "{{}:1,");
	TEST_ERROR(LEPT_PARSE_MISS_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {
	TEST_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\"}");
	TEST_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
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

		/* array */
		test_parse_array();

		/* object */
		test_parse_object();
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

	/* LEPT_PARSE_INVALID_UNICODE_HEX */
	test_parse_invalid_unicode_hex();

	/* LEPT_PARSE_INVALID_UNICODE_SURROGATE */
	test_parse_invalid_unicode_surrogate();

	/* LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET */
	test_parse_miss_comma_or_square_bracket();

	/* LEPT_PARSE_MISS_KEY */
	test_parse_miss_key();

	/* LEPT_PARSE_MISS_COLON */
	test_parse_miss_colon();

	/* LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET */
	test_parse_miss_comma_or_curly_bracket();
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

	lept_set_boolean(&v, 1);
	EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
	EXPECT_TRUE(lept_get_boolean(&v));

	lept_set_boolean(&v, 100);
	EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
	EXPECT_TRUE(lept_get_boolean(&v));

	lept_set_boolean(&v, 0);
	EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));
	EXPECT_FALSE(lept_get_boolean(&v));

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
	EXPECT_EQ_DOUBLE((double)-1234.66, lept_get_number(&v));

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
