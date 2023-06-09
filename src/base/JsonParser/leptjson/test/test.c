#include "../src/leptjson.h"
#include <stddef.h>
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
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
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
/* Json 生成测试用例扩展宏 */
#define TEST_ROUNDTRIP(json)                                \
	do {                                                    \
		lept_value v;                                       \
		char* json2;                                        \
		size_t length;                                      \
		lept_value_init(&v);                                \
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json)); \
		json2 = lept_stringify(&v, &length);                \
		EXPECT_EQ_STRING(json, json2, length);              \
		lept_free(&v);                                      \
		free_ptr(json2);                                    \
	} while (0)
/* Json 相等判断测试用例扩展宏 */
#define TEST_EQUAL(json1, json2, equality)                    \
	do {                                                      \
		lept_value v1, v2;                                    \
		lept_value_init(&v1);                                 \
		lept_value_init(&v2);                                 \
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v1, json1)); \
		EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v2, json2)); \
		EXPECT_EQ_INT(equality, lept_is_equal(&v1, &v2));     \
		lept_free(&v1);                                       \
		lept_free(&v2);                                       \
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
		const lept_value* a = lept_get_array_element(&v, i);
		EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(a));
		EXPECT_EQ_SIZE_T(i, lept_get_array_size(a));
		for (j = 0; j < i; j++) {
			const lept_value* e = lept_get_array_element(a, j);
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
	EXPECT_EQ_INT(LEPT_NULL,
	              lept_get_type(lept_get_object_value_by_index(&v, 0)));
	EXPECT_EQ_STRING("f", lept_get_object_key(&v, 1),
	                 lept_get_object_key_length(&v, 1));
	EXPECT_EQ_INT(LEPT_FALSE,
	              lept_get_type(lept_get_object_value_by_index(&v, 1)));
	EXPECT_EQ_STRING("t", lept_get_object_key(&v, 2),
	                 lept_get_object_key_length(&v, 2));
	EXPECT_EQ_INT(LEPT_TRUE,
	              lept_get_type(lept_get_object_value_by_index(&v, 2)));
	EXPECT_EQ_STRING("i", lept_get_object_key(&v, 3),
	                 lept_get_object_key_length(&v, 3));
	EXPECT_EQ_INT(LEPT_NUMBER,
	              lept_get_type(lept_get_object_value_by_index(&v, 3)));
	EXPECT_EQ_DOUBLE(123.0,
	                 lept_get_number(lept_get_object_value_by_index(&v, 3)));
	EXPECT_EQ_STRING("s", lept_get_object_key(&v, 4),
	                 lept_get_object_key_length(&v, 4));
	EXPECT_EQ_INT(LEPT_STRING,
	              lept_get_type(lept_get_object_value_by_index(&v, 4)));
	EXPECT_EQ_STRING(
	    "abc", lept_get_string(lept_get_object_value_by_index(&v, 4)),
	    lept_get_string_length(lept_get_object_value_by_index(&v, 4)));
	EXPECT_EQ_STRING("a", lept_get_object_key(&v, 5),
	                 lept_get_object_key_length(&v, 5));
	EXPECT_EQ_INT(LEPT_ARRAY,
	              lept_get_type(lept_get_object_value_by_index(&v, 5)));
	EXPECT_EQ_SIZE_T(
	    3, lept_get_array_size(lept_get_object_value_by_index(&v, 5)));
	for (i = 0; i < 3; i++) {
		const lept_value* e =
		    lept_get_array_element(lept_get_object_value_by_index(&v, 5), i);
		EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(e));
		EXPECT_EQ_DOUBLE(i + 1.0, lept_get_number(e));
	}
	EXPECT_EQ_STRING("o", lept_get_object_key(&v, 6),
	                 lept_get_object_key_length(&v, 6));
	{
		const lept_value* o = lept_get_object_value_by_index(&v, 6);

		EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(o));
		for (i = 0; i < 3; i++) {
			const lept_value* ov = lept_get_object_value_by_index(o, i);
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

static void test_stringify_number() {
	TEST_ROUNDTRIP("0");
	TEST_ROUNDTRIP("-0");
	TEST_ROUNDTRIP("1");
	TEST_ROUNDTRIP("-1");
	TEST_ROUNDTRIP("1.5");
	TEST_ROUNDTRIP("-1.5");
	TEST_ROUNDTRIP("3.25");
	TEST_ROUNDTRIP("1e+20");
	TEST_ROUNDTRIP("1.234e+20");
	TEST_ROUNDTRIP("1.234e-20");

	TEST_ROUNDTRIP("1.0000000000000002");      /* the smallest number > 1 */
	TEST_ROUNDTRIP("4.9406564584124654e-324"); /* minimum denormal */
	TEST_ROUNDTRIP("-4.9406564584124654e-324");
	TEST_ROUNDTRIP("2.2250738585072009e-308"); /* Max subnormal double */
	TEST_ROUNDTRIP("-2.2250738585072009e-308");
	TEST_ROUNDTRIP("2.2250738585072014e-308"); /* Min normal positive double */
	TEST_ROUNDTRIP("-2.2250738585072014e-308");
	TEST_ROUNDTRIP("1.7976931348623157e+308"); /* Max double */
	TEST_ROUNDTRIP("-1.7976931348623157e+308");
}

static void test_stringify_string() {
	TEST_ROUNDTRIP("\"\"");
	TEST_ROUNDTRIP("\"Hello\"");
	TEST_ROUNDTRIP("\"Hello\\nWorld\"");
	TEST_ROUNDTRIP("\"\\\" \\\\ / \\b \\f \\n \\r \\t\"");
	TEST_ROUNDTRIP("\"Hello\\u0000World\"");
}

static void test_stringify_array() {
	TEST_ROUNDTRIP("[]");
	TEST_ROUNDTRIP("[null,false,true,123,\"abc\",[1,2,3]]");
}

static void test_stringify_object() {
	TEST_ROUNDTRIP("{}");
	TEST_ROUNDTRIP("{\"n\":null,\"f\":false,\"t\":true,\"i\":123,\"s\":\"abc\","
	               "\"a\":[1,2,3],\"o\":{\"1\":1,\"2\":2,\"3\":3}}");
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

static void test_stringify() {
	TEST_ROUNDTRIP("null");
	TEST_ROUNDTRIP("false");
	TEST_ROUNDTRIP("true");
	test_stringify_number();
	test_stringify_string();
	test_stringify_array();
	test_stringify_object();
}

static void test_equal() {
	TEST_EQUAL("true", "true", 1);
	TEST_EQUAL("true", "false", 0);
	TEST_EQUAL("false", "false", 1);
	TEST_EQUAL("null", "null", 1);
	TEST_EQUAL("null", "0", 0);
	TEST_EQUAL("123", "123", 1);
	TEST_EQUAL("123", "456", 0);
	TEST_EQUAL("\"abc\"", "\"abc\"", 1);
	TEST_EQUAL("\"abc\"", "\"abcd\"", 0);
	TEST_EQUAL("[]", "[]", 1);
	TEST_EQUAL("[]", "null", 0);
	TEST_EQUAL("[1,2,3]", "[1,2,3]", 1);
	TEST_EQUAL("[1,2,3]", "[1,2,3,4]", 0);
	TEST_EQUAL("[[]]", "[[]]", 1);
	TEST_EQUAL("{}", "{}", 1);
	TEST_EQUAL("{}", "null", 0);
	TEST_EQUAL("{}", "[]", 0);

	TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":2}", 1);
	TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"b\":2,\"a\":1}", 1);
	TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":3}", 0);
	TEST_EQUAL("{\"a\":1,\"b\":2}", "{\"a\":1,\"b\":2,\"c\":3}", 0);
	TEST_EQUAL("{\"a\":{\"b\":{\"c\":{}}}}", "{\"a\":{\"b\":{\"c\":{}}}}", 1);
	TEST_EQUAL("{\"a\":{\"b\":{\"c\":{}}}}", "{\"a\":{\"b\":{\"c\":[]}}}", 0);
}

static void test_copy() {
	lept_value v1, v2;
	lept_value_init(&v1);
	lept_parse(&v1,
	           "{\"t\":true,\"f\":false,\"n\":null,\"d\":1.5,\"a\":[1,2,3]}");
	lept_value_init(&v2);
	lept_copy(&v2, &v1);
	EXPECT_TRUE(lept_is_equal(&v2, &v1));
	lept_free(&v1);
	lept_free(&v2);
}

static void test_move() {
	lept_value v1, v2, v3;
	lept_value_init(&v1);
	lept_parse(&v1,
	           "{\"t\":true,\"f\":false,\"n\":null,\"d\":1.5,\"a\":[1,2,3]}");
	lept_value_init(&v2);
	lept_copy(&v2, &v1);
	lept_value_init(&v3);
	lept_move(&v3, &v2);
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v2));
	EXPECT_TRUE(lept_is_equal(&v3, &v1));
	lept_free(&v1);
	lept_free(&v2);
	lept_free(&v3);
}

static void test_swap() {
	lept_value v1, v2;
	lept_value_init(&v1);
	lept_value_init(&v2);
	lept_set_string(&v1, "Hello", 5);
	lept_set_string(&v2, "World!", 6);
	lept_swap(&v1, &v2);
	EXPECT_EQ_STRING("World!", lept_get_string(&v1),
	                 lept_get_string_length(&v1));
	EXPECT_EQ_STRING("Hello", lept_get_string(&v2),
	                 lept_get_string_length(&v2));
	lept_free(&v1);
	lept_free(&v2);
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

static void test_access_array() {
	lept_value a, e;
	size_t i, j;

	lept_value_init(&a);

	/* push_back lept_set_array_move */
	for (j = 0; j <= 5; j += 5) {
		lept_set_array(&a, j);
		EXPECT_EQ_SIZE_T(0, lept_get_array_size(&a));
		EXPECT_EQ_SIZE_T(j, lept_get_array_capacity(&a));
		for (i = 0; i < 10; i++) {
			lept_value_init(&e);
			lept_set_number(&e, i);
			lept_pushback_array_element(&a, &e);
			lept_free(&e);
		}

		EXPECT_EQ_SIZE_T(10, lept_get_array_size(&a));
		for (i = 0; i < 10; i++)
			EXPECT_EQ_DOUBLE((double)i,
			                 lept_get_number(lept_get_array_element(&a, i)));
	}

	/* pop_back */
	lept_popback_array_element(&a);
	EXPECT_EQ_SIZE_T(9, lept_get_array_size(&a));
	for (i = 0; i < 9; i++)
		EXPECT_EQ_DOUBLE((double)i,
		                 lept_get_number(lept_get_array_element(&a, i)));

	/* erase */
	lept_erase_array_element(&a, 4, 0);
	EXPECT_EQ_SIZE_T(9, lept_get_array_size(&a));
	for (i = 0; i < 9; i++)
		EXPECT_EQ_DOUBLE((double)i,
		                 lept_get_number(lept_get_array_element(&a, i)));

	lept_erase_array_element(&a, 8, 1);
	EXPECT_EQ_SIZE_T(8, lept_get_array_size(&a));
	for (i = 0; i < 8; i++)
		EXPECT_EQ_DOUBLE((double)i,
		                 lept_get_number(lept_get_array_element(&a, i)));

	lept_erase_array_element(&a, 0, 2);
	EXPECT_EQ_SIZE_T(6, lept_get_array_size(&a));
	for (i = 0; i < 6; i++)
		EXPECT_EQ_DOUBLE((double)i + 2,
		                 lept_get_number(lept_get_array_element(&a, i)));

	/* insert */
	for (i = 0; i < 2; i++) {
		lept_value_init(&e);
		lept_set_number(&e, i);
		lept_insert_array_element(&a, &e, i);
		lept_free(&e);
	}

	EXPECT_EQ_SIZE_T(8, lept_get_array_size(&a));
	for (i = 0; i < 8; i++)
		EXPECT_EQ_DOUBLE((double)i,
		                 lept_get_number(lept_get_array_element(&a, i)));

	EXPECT_TRUE(lept_get_array_capacity(&a) > 8);
	lept_shrink_array(&a);
	EXPECT_EQ_SIZE_T(8, lept_get_array_capacity(&a));
	EXPECT_EQ_SIZE_T(8, lept_get_array_size(&a));
	for (i = 0; i < 8; i++)
		EXPECT_EQ_DOUBLE((double)i,
		                 lept_get_number(lept_get_array_element(&a, i)));

	lept_set_string(&e, "Hello", 5);
	lept_pushback_array_element(&a, &e);
	lept_free(&e);

	/* 此处在 earse 时修改了其容量变化规则，故在清空时容量值会变为 1 */
	/* i = lept_get_array_capacity(&a); */
	lept_clear_array(&a);
	EXPECT_EQ_SIZE_T(0, lept_get_array_size(&a));
	EXPECT_EQ_SIZE_T(
	    1, lept_get_array_capacity(&a)); /* capacity remains unchanged */
	lept_shrink_array(&a);
	EXPECT_EQ_SIZE_T(0, lept_get_array_capacity(&a));

	lept_free(&a);
}

static void test_access_object() {

	lept_value o, v;
	const lept_value* pv;
	size_t i, j, index;

	lept_value_init(&o);

	for (j = 0; j <= 5; j += 5) {

		lept_set_object(&o, j);
		EXPECT_EQ_SIZE_T(0, lept_get_object_size(&o));
		EXPECT_EQ_SIZE_T(j, lept_get_object_capacity(&o));

		for (i = 0; i < 10; i++) {
			char key[2] = "a";
			key[0] += i;
			lept_value_init(&v);
			lept_set_number(&v, i);
			lept_set_object_value_by_key(&o, key, 1, &v);

			lept_free(&v);
		}

		EXPECT_EQ_SIZE_T(10, lept_get_object_size(&o));
		for (i = 0; i < 10; i++) {
			char key[] = "a";
			key[0] += i;
			index = lept_find_object_index(&o, key, 1);
			EXPECT_TRUE(index != LEPT_KEY_NOT_EXIST);
			pv = lept_get_object_value_by_index(&o, index);
			EXPECT_EQ_DOUBLE((double)i, lept_get_number(pv));
		}
	}

	/* find */
	index = lept_find_object_index(&o, "j", 1);
	EXPECT_TRUE(index != LEPT_KEY_NOT_EXIST);
	lept_remove_object_value_by_index(&o, index);
	index = lept_find_object_index(&o, "j", 1);
	EXPECT_TRUE(index == LEPT_KEY_NOT_EXIST);
	EXPECT_EQ_SIZE_T(9, lept_get_object_size(&o));

	index = lept_find_object_index(&o, "a", 1);
	EXPECT_TRUE(index != LEPT_KEY_NOT_EXIST);
	lept_remove_object_value_by_index(&o, index);
	index = lept_find_object_index(&o, "a", 1);
	EXPECT_TRUE(index == LEPT_KEY_NOT_EXIST);
	EXPECT_EQ_SIZE_T(8, lept_get_object_size(&o));

	EXPECT_TRUE(lept_get_object_capacity(&o) > 8);
	lept_shrink_object(&o);
	EXPECT_EQ_SIZE_T(8, lept_get_object_capacity(&o));
	EXPECT_EQ_SIZE_T(8, lept_get_object_size(&o));
	for (i = 0; i < 8; i++) {
		char key[] = "a";
		key[0] += i + 1;
		EXPECT_EQ_DOUBLE((double)i + 1,
		                 lept_get_number(lept_get_object_value_by_index(
		                     &o, lept_find_object_index(&o, key, 1))));
	}

	lept_set_string(&v, "Hello", 5);
	lept_set_object_value_by_key(&o, "World", 5, &v);
	lept_free(&v);

	pv = lept_find_object_value(&o, "World", 5);
	EXPECT_TRUE(pv != NULL);
	EXPECT_EQ_STRING("Hello", lept_get_string(pv), lept_get_string_length(pv));

	lept_get_object_capacity(&o);
	lept_clear_object(&o);
	EXPECT_EQ_SIZE_T(0, lept_get_object_size(&o));
	/* 删除值时会调整 capacity ，清空时其大小应为 0 * 2 + 1 = 1 */
	EXPECT_EQ_SIZE_T(1, lept_get_object_capacity(&o));
	lept_shrink_object(&o);
	EXPECT_EQ_SIZE_T(0, lept_get_object_capacity(&o));

	lept_free(&o);
}

/***************/
/* 总的测试函数 */
/***************/

static void test() {

	test_parse();
	test_stringify();

	test_equal();
	test_copy();
	test_move();
	test_swap();

	/* 其余接口测试 */
	test_access_null();
	test_access_boolean();
	test_access_number();
	test_access_string();
	test_access_array();
	test_access_object();
}

int main() {

	test();
	printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count,
	       test_pass * 100.0 / test_count);
	return main_ret;
}
