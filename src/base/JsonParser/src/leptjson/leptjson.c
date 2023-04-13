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

/* Json 生成缓冲区 */
#ifndef LEPT_PARSE_STRINGIFY_INIT_SIZE
#define LEPT_PARSE_STRINGIFY_INIT_SIZE 256
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
#define PUTS(c, s, len)                            \
	do {                                           \
		memcpy(lept_context_push(c, len), s, len); \
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

/* 重构 string 解析函数 */
/* 将 string 解析和装载分离 */
static int lept_parse_string_raw(lept_context* c, char** str, size_t* len);

/* string = "\"......\"" */
static int lept_parse_string(lept_context* c, lept_value* v);

/* array = "[......]" */
static int lept_parse_array(lept_context* c, lept_value* v);

/* object = {......} */
static int lept_parse_object(lept_context* c, lept_value* v);

/* value = null / false / true / number / string / array / object */
static int lept_parse_value(lept_context* c, lept_value* v);

/* 生成字符串 string */
static void lept_stringify_string(lept_context* c, const char* s, size_t len);

/* 生成 Json 串 */
static void lept_stringify_value(lept_context* c, const lept_value* v);

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

void lept_stringify(const lept_value* v, char* s, size_t* length) {
	assert(v != NULL);

	lept_context* c = (lept_context*)malloc(sizeof(lept_context));
	c->stack = (char*)malloc(c->size = LEPT_PARSE_STRINGIFY_INIT_SIZE);
	c->top = 0;

	lept_stringify_value(c, v);

	if (length)
		*length = c->top;
	PUTC(c, '\0');

	memcpy(s, c->stack, (*length)++);

	lept_context_free(c);
}

void lept_copy(lept_value* dst, const lept_value* src) {
	assert(src != NULL && dst != NULL && src != dst);
	switch (src->type) {
	case LEPT_STRING:
		lept_set_string_copy(dst, src->u.s.s, src->u.s.len);
		break;
	case LEPT_ARRAY:
		lept_set_array_copy(dst, src->u.a.e, src->u.a.size, src->u.a.capacity);
		break;
	case LEPT_OBJECT:
		lept_set_object_copy(dst, src->u.o.m, src->u.o.size, src->u.o.capacity);
		break;
	default:
		lept_free(dst);
		memcpy(dst, src, sizeof(lept_value));
		break;
	}
}
void lept_move(lept_value* dst, lept_value* src) {
	assert(dst != NULL && src != NULL && src != dst);
	lept_free(dst);
	memcpy(dst, src, sizeof(lept_value));
	lept_value_init(src);
}
void lept_swap(lept_value* lhs, lept_value* rhs) {
	assert(lhs != NULL && rhs != NULL);
	if (lhs != rhs) {
		lept_value temp;
		memcpy(&temp, lhs, sizeof(lept_value));
		memcpy(lhs, rhs, sizeof(lept_value));
		memcpy(rhs, &temp, sizeof(lept_value));
	}
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
	case LEPT_OBJECT:
		for (i = 0; i < v->u.o.size; i++) {
			free(v->u.o.m[i].k);
			lept_free(&v->u.o.m[i].v);
		}
		free(v->u.o.m);
		break;
	default:
		break;
	}
	v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v) { return v->type; }

/* 如要考虑同元素不同排序问题，则需要逐层递归排序，开销较大 */
/* 此处将不同排序对象视为不同对象 */
/* 在 mJson 中将实现两个版本 */
int lept_is_equal(const lept_value* lhs, const lept_value* rhs) {
	size_t i;
	assert(lhs != NULL && rhs != NULL);
	if (lhs->type != rhs->type)
		return 0;
	switch (lhs->type) {
	case LEPT_STRING:
		return lhs->u.s.len == rhs->u.s.len &&
		       memcmp(lhs->u.s.s, rhs->u.s.s, lhs->u.s.len) == 0;
	case LEPT_NUMBER:
		return lhs->u.n == rhs->u.n;
	case LEPT_ARRAY:
		if (lhs->u.a.size != rhs->u.a.size)
			return 0;
		for (i = 0; i < lhs->u.a.size; i++)
			if (!lept_is_equal(&lhs->u.a.e[i], &rhs->u.a.e[i]))
				return 0;
		return 1;
	case LEPT_OBJECT:
		if (lhs->u.o.size != rhs->u.o.size)
			return 0;
		for (i = 0; i < lhs->u.o.size; i++) {
			if (lhs->u.o.m[i].klen != rhs->u.o.m[i].klen)
				return 0;
			if (!memcmp(lhs->u.o.m[i].k, rhs->u.o.m[i].k, lhs->u.o.m[i].klen))
				return 0;
			if (!lept_is_equal(&lhs->u.o.m[i].v, &rhs->u.o.m[i].v))
				return 0;
		}
		return 1;
	default:
		return 1;
	}
}

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
void lept_set_string_copy(lept_value* v, const char* s, size_t len) {

	/* lept_free() 中存在空断言 */
	/* assert(v!= NULL); */
	/* 此处由于对 s 进行读，所以该断言仍然需要保留 */

	assert(v != NULL && (s != NULL || len == 0));

	/* 执行深拷贝 */
	char* tmp = (char*)malloc(len + 1);
	memcpy(v->u.s.s, s, len);

	/* 补充尾部 '\0' 字符 */
	tmp[len] = '\0';

	lept_set_string_move(v, tmp, len + 1);
}
void lept_set_string_move(lept_value* v, char* s, size_t len) {

	assert(v != NULL && (s != NULL || len == 0));
	lept_free(v);

	v->u.s.s = s;
	v->u.s.len = len;
	v->type = LEPT_STRING;
}

/* 获取和写入 array 数组元素数和元素值 */
/* 没看 miloyip 大佬该函数含义，遂按照前文 set 函数进行修改 */
/* 原文中该函数放入 lept_set_array_move 中，当输入 e=NULL && size == 0 调用 */
void lept_set_array_copy(lept_value* v, const lept_value* e, size_t size,
                         size_t capacity) {
	assert(v != NULL && (e != NULL || size == 0));

	lept_value* tmp = (lept_value*)malloc(capacity * sizeof(lept_value));
	memcpy(v->u.a.e, e, size * sizeof(lept_value));

	lept_set_array_move(v, tmp, size, capacity);
}
void lept_set_array_move(lept_value* v, lept_value* e, size_t size,
                         size_t capacity) {
	assert(v != NULL);
	lept_free(v);

	if (e == NULL && size == 0) {
		v->type = LEPT_ARRAY;
		v->u.a.size = size;
		v->u.a.capacity = capacity;
		v->u.a.e = capacity > 0
		               ? (lept_value*)malloc(capacity * sizeof(lept_value))
		               : NULL;
		return;
	}

	v->type = LEPT_ARRAY;
	v->u.a.size = size;
	v->u.a.capacity = capacity;
	v->u.a.e = e;
}

size_t lept_get_array_size(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_ARRAY);
	return v->u.a.size;
}
size_t lept_get_array_capacity(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_ARRAY);
	return v->u.a.capacity;
}

void lept_reserve_array(lept_value* v, size_t capacity) {
	assert(v != NULL && v->type == LEPT_ARRAY);
	if (v->u.a.capacity < capacity) {
		v->u.a.capacity = capacity;
		v->u.a.e =
		    (lept_value*)realloc(v->u.a.e, capacity * sizeof(lept_value));
	}
}
/* 这直接把 capacity 设置成 size 大小 */
/* 这里会不会有性能问题？ 因为下次执行插入的时候必定会执行扩容 */
void lept_shrink_array(lept_value* v) {
	assert(v != NULL && v->type == LEPT_ARRAY);
	if (v->u.a.capacity > v->u.a.size) {
		v->u.a.capacity = v->u.a.size;
		v->u.a.e = (lept_value*)realloc(v->u.a.e,
		                                v->u.a.capacity * sizeof(lept_value));
	}
}
void lept_clear_array(lept_value* v) {
	assert(v != NULL && v->type == LEPT_ARRAY);
	lept_erase_array_element(v, 0, v->u.a.size);

	/* clear 时容量剩余值不会改变 */
	/* 需使用 lept_shrink_array 手动释放剩余内存 */

	/* lept_shrink_array(v); */
}

const lept_value* lept_get_array_element(const lept_value* v, size_t index) {
	assert(v != NULL && v->type == LEPT_ARRAY);
	assert(index < v->u.a.size);
	return &v->u.a.e[index];
}
void lept_pushback_array_element(lept_value* v, const lept_value* e) {
	assert(v != NULL && e != NULL && v->type == LEPT_ARRAY);
	if (v->u.a.size + 1 == v->u.a.capacity)
		lept_reserve_array(v, v->u.a.capacity == 0 ? 1 : v->u.a.capacity * 2);

	lept_copy((v->u.a.e) + v->u.a.size - 1, e);
}
void lept_popback_array_element(lept_value* v) {
	assert(v != NULL && v->type == LEPT_ARRAY && v->u.a.size > 0);
	v->u.a.size--;
	lept_free((v->u.a.e) + v->u.a.size - 1);
}
void lept_insert_array_element(lept_value* v, const lept_value* e,
                               size_t index) {
	assert(v != NULL && v->type == LEPT_ARRAY && index <= v->u.a.size);

	// 容量不够，扩容
	if (v->u.a.size + 1 == v->u.a.capacity)
		lept_reserve_array(v, v->u.a.capacity == 0 ? 1 : v->u.a.capacity * 2);

	size_t i;
	// 插入
	for (i = v->u.a.size - 2; i >= index; i--)
		lept_move((v->u.a.e) + i + 1, (v->u.a.e) + i);

	lept_copy((v->u.a.e) + index, e);
}
void lept_erase_array_element(lept_value* v, size_t index, size_t count) {

	/* 断言中设置了删除范围在 size
	 * 内，故考虑直接将删除位置两侧的值拷贝到新的数组中 */
	assert(v != NULL && v->type == LEPT_ARRAY && index + count <= v->u.a.size);

	/* 分配删除后数组大小两倍的空间，如果删除后 size 为 0 则分配一个空间 */
	size_t new_size = v->u.a.size - count;
	size_t new_capacity = 2 * new_size + 1;
	lept_value* tmp = malloc(new_size * sizeof(lept_value));

	size_t i = 0;
	/* 拷贝删除区间左侧区间值 */
	for (i = 0; i < index; i++)
		lept_copy(tmp + i, v + i);

	/* 拷贝删除区间右侧区间值 */
	for (i = index + count; i < v->u.a.size; i++)
		lept_copy(tmp + i - count, v + i);

	lept_set_array_move(v, tmp, new_size, new_capacity);
}

/* 获取和写入 Json 对象 */
/* 对于读取，若要复制其深拷贝交由调用处实现 */
void lept_set_object_copy(lept_value* v, const lept_member* s, size_t size,
                          size_t capacity) {

	assert(v != NULL && (s != NULL || size == 0));

	int i = 0;

	lept_member* tmp = (lept_member*)malloc(capacity * sizeof(lept_member));

	for (i = 0; i < size; i++) {
		tmp[i].klen = s[i].klen;
		tmp[i].k = (char*)malloc(s[i].klen);
		memcpy(tmp[i].k, s->k, s[i].klen);

		lept_copy(&(tmp[i].v), &(s[i].v));
	}

	lept_set_object_move(v, tmp, size, capacity);
}

void lept_set_object_move(lept_value* v, lept_member* s, size_t size,
                          size_t capacity) {
	assert(v != NULL);
	lept_free(v);

	/* 此时对容量执行操作 */
	if (s == NULL && size == 0) {
		v->type = LEPT_OBJECT;
		v->u.o.size = size;
		v->u.o.capacity = capacity;
		v->u.o.m = capacity > 0
		               ? (lept_member*)malloc(capacity * sizeof(lept_member))
		               : NULL;
		return;
	}

	v->type = LEPT_OBJECT;
	v->u.o.size = size;
	v->u.o.capacity = capacity;
	v->u.o.m = s;
}

size_t lept_get_object_size(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_OBJECT);
	return v->u.o.size;
}
size_t lept_get_object_capacity(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_OBJECT);
	return v->u.o.capacity;
	return 0;
}

void lept_reserve_object(lept_value* v, size_t capacity) {
	assert(v != NULL && v->type == LEPT_OBJECT);
	if (v->u.o.capacity < capacity) {
		v->u.o.capacity = capacity;
		v->u.o.m =
		    (lept_member*)realloc(v->u.o.m, capacity * sizeof(lept_member));
	}
}

void lept_shrink_object(lept_value* v) {
	assert(v != NULL && v->type == LEPT_OBJECT);
	if (v->u.o.capacity > v->u.o.size) {
		v->u.o.capacity = v->u.o.size;
		v->u.o.m = (lept_member*)realloc(v->u.o.m,
		                                 v->u.o.capacity * sizeof(lept_member));
	}
}
void lept_clear_object(lept_value* v) {
	assert(v != NULL && v->type == LEPT_OBJECT);

	/* 懒得去实现了，就逐个删得了  */
	size_t i;
	for (i = 0; i < v->u.o.size; i++)
		lept_remove_object_value_by_index(v, i);

	lept_free(v);
}

const char* lept_get_object_key(const lept_value* v, size_t index) {
	assert(v != NULL && v->type == LEPT_OBJECT);
	assert(index < v->u.o.size);
	return v->u.o.m[index].k;
}
size_t lept_get_object_key_length(const lept_value* v, size_t index) {
	assert(v != NULL && v->type == LEPT_OBJECT);
	assert(index < v->u.o.size);
	return v->u.o.m[index].klen;
}

const lept_value* lept_get_object_value_by_index(const lept_value* v,
                                                 size_t index) {
	assert(v != NULL && v->type == LEPT_OBJECT);
	assert(index < v->u.o.size);
	return &v->u.o.m[index].v;
}
const lept_value* lept_get_object_value_by_key(const lept_value* v,
                                               const char* key, size_t klen) {
	return lept_find_object_value(v, key, klen);
}

void lept_remove_object_value_by_index(lept_value* v, size_t index) {
	assert(v != NULL && v->type == LEPT_OBJECT && index < v->u.o.size);

	size_t i;
	for (i = index; i < v->u.o.size; i++)
		v->u.o.m[i] = v->u.o.m[i + 1];

	if ((v->u.o.size + 1) * 3 < v->u.o.capacity)
		lept_reserve_object(v, v->u.o.size * 2);
}
int lept_remove_object_value_by_key(lept_value* v, const char* key,
                                    size_t klen) {
	size_t index = lept_find_object_index(v, key, klen);
	if (index == LEPT_KEY_NOT_EXIST)
		return 0;

	lept_remove_object_value_by_index(v, index);
	return 1;
}

int lept_set_object_value_copy(lept_value* v, const char* key, size_t klen,
                               const lept_value* s_v) {
	size_t index = lept_find_object_index(v, key, klen);

	if (index == LEPT_KEY_NOT_EXIST)
		return 0;

	lept_copy(&v->u.o.m->v, s_v);
	return 1;
}
int lept_set_object_value_move(lept_value* v, const char* key, size_t klen,
                               lept_value* s_v) {
	size_t index = lept_find_object_index(v, key, klen);

	if (index == LEPT_KEY_NOT_EXIST)
		return 0;

	lept_move(&v->u.o.m->v, s_v);
	return 1;
}

/* find 查找 */
size_t lept_find_object_index(const lept_value* v, const char* key,
                              size_t klen) {
	size_t i;
	assert(v != NULL && v->type == LEPT_OBJECT && key != NULL);
	for (i = 0; i < v->u.o.size; i++)
		if (v->u.o.m[i].klen == klen && memcmp(v->u.o.m[i].k, key, klen) == 0)
			return i;
	return LEPT_KEY_NOT_EXIST;
}
const lept_value* lept_find_object_value(const lept_value* v, const char* key,
                                         size_t klen) {
	size_t index = lept_find_object_index(v, key, klen);
	return index != LEPT_KEY_NOT_EXIST ? &v->u.o.m[index].v : NULL;
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

static int lept_parse_string_raw(lept_context* c, char** str, size_t* len) {
	size_t head = c->top;
	unsigned u, u2;
	const char* p;
	EXPECT(c, '\"');
	p = c->json;
	for (;;) {
		char ch = *p++;
		switch (ch) {
		case '\"':
			*len = c->top - head;
			*str = lept_context_pop(c, *len);
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

static int lept_parse_string(lept_context* c, lept_value* v) {
	char* s;
	size_t len;
	int ret = lept_parse_string_raw(c, &s, &len);
	if (ret == LEPT_PARSE_OK)
		lept_set_string_copy(v, s, len);
	return ret;
}

static int lept_parse_array(lept_context* c, lept_value* v) {
	size_t i, size = 0;
	int ret;
	EXPECT(c, '[');
	lept_parse_whitespace(c);

	/* 空类型数组解析 */
	if (*c->json == ']') {
		c->json++;
		lept_set_array_move(v, NULL, 0, 0);
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
			lept_set_array_move(v, NULL, 0, size);
			memcpy(v->u.a.e, lept_context_pop(c, size * sizeof(lept_value)),
			       size * sizeof(lept_value));
			v->u.a.size = size;

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

static int lept_parse_object(lept_context* c, lept_value* v) {
	size_t i, size;
	lept_member m;

	int ret;
	EXPECT(c, '{');
	lept_parse_whitespace(c);

	/* 空对象处理 */
	/* 可以考虑将空初始化抽象成函数 */
	if (*c->json == '}') {
		c->json++;
		lept_set_object_move(v, NULL, 0, 0);
		return LEPT_PARSE_OK;
	}

	m.k = NULL;
	size = 0;
	for (;;) {
		char* str;
		lept_value_init(&m.v);

		/* 解析 key */
		if (*c->json != '"') {
			ret = LEPT_PARSE_MISS_KEY;
			break;
		}
		ret = lept_parse_string_raw(c, &str, &m.klen);
		if (ret != LEPT_PARSE_OK)
			break;
		memcpy(m.k = (char*)malloc(m.klen + 1), str, m.klen);
		m.k[m.klen] = '\0';

		/* 解析中间 : */
		lept_parse_whitespace(c);
		if (*c->json != ':') {
			ret = LEPT_PARSE_MISS_COLON;
			break;
		}
		c->json++;
		lept_parse_whitespace(c);

		/* 解析对象值 */
		ret = lept_parse_value(c, &m.v);
		if (ret != LEPT_PARSE_OK)
			break;
		memcpy(lept_context_push(c, sizeof(lept_member)), &m,
		       sizeof(lept_member));
		size++;
		m.k = NULL; /* ownership is transferred to member on stack */

		/* parse ws [comma | right-curly-brace] ws */
		lept_parse_whitespace(c);
		if (*c->json == ',') {
			c->json++;
			lept_parse_whitespace(c);
		} else if (*c->json == '}') {
			size_t s = sizeof(lept_member) * size;
			c->json++;
			lept_set_object_move(v, NULL, 0, size);
			memcpy(v->u.o.m, lept_context_pop(c, sizeof(lept_member) * size),
			       sizeof(lept_member) * size);
			v->u.o.size = size;
			return LEPT_PARSE_OK;
		} else {
			ret = LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
			break;
		}
	}
	/* Pop and free members on the stack */
	free(m.k);
	for (i = 0; i < size; i++) {
		lept_member* m = (lept_member*)lept_context_pop(c, sizeof(lept_member));
		free(m->k);
		lept_free(&m->v);
	}
	v->type = LEPT_NULL;
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
	case '{':
		return lept_parse_object(c, v);
	case '\0':
		return LEPT_PARSE_EXPECT_VALUE;
	default:
		return lept_parse_number(c, v);
	}
}

static void lept_stringify_string(lept_context* c, const char* s, size_t len) {
	static const char hex_digits[] = {'0', '1', '2', '3', '4', '5', '6', '7',
	                                  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	size_t i, size;
	char *head, *p;
	assert(s != NULL);
	p = head = lept_context_push(c, size = len * 6 + 2); /* "\u00xx..." */
	*p++ = '"';
	for (i = 0; i < len; i++) {
		unsigned char ch = (unsigned char)s[i];
		switch (ch) {
		case '\"':
			*p++ = '\\';
			*p++ = '\"';
			break;
		case '\\':
			*p++ = '\\';
			*p++ = '\\';
			break;
		case '\b':
			*p++ = '\\';
			*p++ = 'b';
			break;
		case '\f':
			*p++ = '\\';
			*p++ = 'f';
			break;
		case '\n':
			*p++ = '\\';
			*p++ = 'n';
			break;
		case '\r':
			*p++ = '\\';
			*p++ = 'r';
			break;
		case '\t':
			*p++ = '\\';
			*p++ = 't';
			break;
		default:
			if (ch < 0x20) {
				*p++ = '\\';
				*p++ = 'u';
				*p++ = '0';
				*p++ = '0';
				*p++ = hex_digits[ch >> 4];
				*p++ = hex_digits[ch & 15];
			} else
				*p++ = s[i];
		}
	}
	*p++ = '"';
	c->top -= size - (p - head);
}

static void lept_stringify_value(lept_context* c, const lept_value* v) {
	size_t i;
	switch (v->type) {
	case LEPT_NULL:
		PUTS(c, "null", 4);
		break;
	case LEPT_FALSE:
		PUTS(c, "false", 5);
		break;
	case LEPT_TRUE:
		PUTS(c, "true", 4);
		break;
	case LEPT_NUMBER:
		c->top -= 32 - sprintf(lept_context_push(c, 32), "%.17g", v->u.n);
		break;
	case LEPT_STRING:
		lept_stringify_string(c, v->u.s.s, v->u.s.len);
		break;
	case LEPT_ARRAY:
		PUTC(c, '[');
		for (i = 0; i < v->u.a.size; i++) {
			if (i > 0)
				PUTC(c, ',');
			lept_stringify_value(c, &v->u.a.e[i]);
		}
		PUTC(c, ']');
		break;
	case LEPT_OBJECT:
		PUTC(c, '{');
		for (i = 0; i < v->u.o.size; i++) {
			if (i > 0)
				PUTC(c, ',');
			lept_stringify_string(c, v->u.o.m[i].k, v->u.o.m[i].klen);
			PUTC(c, ':');
			lept_stringify_value(c, &v->u.o.m[i].v);
		}
		PUTC(c, '}');
		break;
	default:
		assert(0 && "invalid type");
	}
}