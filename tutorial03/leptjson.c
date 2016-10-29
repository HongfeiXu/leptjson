#define _WINDOWS
#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "leptjson.h"
#include <assert.h> // assert() 
#include <stdlib.h> // NULL, strtod() 
#include <errno.h>	// errno 
#include <math.h>	// HUGE_VAL 
#include <stdlib.h>	// NULL, malloc(), realloc(), free(), strtod()
#include <string.h>	// memcpy()

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)		((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)	((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch) do{*(char*)lept_context_push(c, sizeof(char)) = (ch);} while(0)

typedef struct
{
	const char* json;
	char *stack;			// 堆栈结构，存储解析字符串时的结果
	size_t size, top;		// 堆栈容量，栈顶的位置（指向下一个元素插入的空位置）
}lept_context;

// 实现 c.stack 堆栈的压入和弹出操作。这里的堆栈是以字节存储的。每次可要求压入或弹出任意大小的数据，它会返回数据起始的指针。
static void *lept_context_push(lept_context *c, size_t size)
{
	void *ret;
	assert(size > 0);
	if(c->top + size >= c->size)
	{
		if(c->size == 0)
			c->size = LEPT_PARSE_STACK_INIT_SIZE;
		while(c->top + size >= c->size)
			c->size += c->size >> 1;
		c->stack = (char*) realloc(c->stack, c->size);
	}
	ret = c->stack + c->top;
	c->top += size;
	return ret;
}

static void *lept_context_pop(lept_context *c, size_t size)
{
	assert(c->top >= size);
	return c->stack + (c->top -= size);
}

static void lept_parse_whitespace(lept_context* c)
{
	const char *p = c->json;
	while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
		p++;
	c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type)
{
	size_t i;
	EXPECT(c, literal[0]);
	for(i = 0; literal[i + 1]; i++)
	{
		if(c->json[i] != literal[i + 1])
		{
			return LEPT_PARSE_INVALID_VALUE;
		}
	}
	c->json += i;
	v->type = type;
	return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v)
{
	const char* p = c->json;
	// 负号
	if(*p == '-') p++;
	// 整数
	if(*p == '0') p++;
	else
	{
		if(!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
		for(p++; ISDIGIT(*p); p++);
	}
	// 小数
	if(*p == '.')
	{
		p++;
		if(!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
		for(p++; ISDIGIT(*p); p++);
	}
	// 指数
	if(*p == 'e' || *p == 'E')
	{
		p++;
		if(*p == '+' || *p == '-') p++;
		if(!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
		for(p++; ISDIGIT(*p); p++);
	}

	errno = 0;
	v->u.n = strtod(c->json, NULL);
	if(errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
	{
		return LEPT_PARSE_NUMBER_TOO_BIG;
	}
	v->type = LEPT_NUMBER;
	c->json = p;

	return LEPT_PARSE_OK;
}



// 解析字符串，先备份栈顶，然后把解析到的字符栈，最后计算出长度并 一次性 把所有的字符弹出，再设置至值里便可。
// 没有处理转义和一些不合法字符的校验。
static int lept_parse_string(lept_context *c, lept_value *v)
{
	size_t head = c->top, len;
	const char *p;
	EXPECT(c, '\"');
	p = c->json;
	for(;;)
	{
		char ch = *p++;
		switch(ch)
		{
		case '\"':
			len = c->top - head;
			lept_set_string(v, (const char*) lept_context_pop(c, len), len);
			c->json = p;
			return LEPT_PARSE_OK;
		case '\0':
			c->top = head;
			return LEPT_PARSE_MISS_QUOTATION_MARK;
		case '\\':
			switch(*p++)
			{
			case '\"': PUTC(c, '\"'); break;
			case '\\': PUTC(c, '\\'); break;
			case '/' : PUTC(c, '/'); break;
			case 'b': PUTC(c, '\b'); break;
			case 'f': PUTC(c, '\f'); break;
			case 'n': PUTC(c, '\n'); break;
			case 'r': PUTC(c, '\r'); break;
			case 't': PUTC(c, '\t'); break;
			default:
				c->top = head;
				return LEPT_PARSE_INVALID_STRING_ESCAPE;
			}
			break;
		default:
			if((unsigned char) ch < 0x20)		// 处理不合法的字符串
			{
				c->top = head;
				return LEPT_PARSE_INVALID_STRING_CHAR;
			}
			PUTC(c, ch);
		}
	}
}

static int lept_parse_value(lept_context* c, lept_value* v)
{
	switch(*c->json)
	{
	case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
	case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
	case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
	case '"':  return lept_parse_string(c, v);
	case '\0': return LEPT_PARSE_EXPECT_VALUE;
	default:   return lept_parse_number(c, v);
	}
}

int lept_parse(lept_value* v, const char* json)
{
	lept_context c;		// 待解析的内容
	int ret;
	assert(v != NULL);
	c.json = json;
	c.stack = NULL;		// 初始化 stack 
	c.size = c.top = 0;	// 同上
	v->type = LEPT_NULL;
	lept_parse_whitespace(&c);
	if((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK)
	{
		lept_parse_whitespace(&c);
		if(*c.json != '\0')
		{
			v->type = LEPT_NULL;
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	assert(c.top == 0);	// 加入断言，确保所有数据都被弹出
	free(c.stack);		// 释放 stack 所占内存
	return ret;
}

void lept_free(lept_value *v)
{
	assert(v != NULL);
	if(v->type == LEPT_STRING)
		free(v->u.s.s);
	v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v)
{
	assert(v != NULL);
	return v->type;
}

int lept_get_boolean(const lept_value *v)
{
	assert(v != NULL && (v->type == LEPT_FALSE || v->type == LEPT_TRUE));
	return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value *v, int b)
{
	lept_free(v);
	v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

double lept_get_number(const lept_value* v)
{
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->u.n;
}

void lept_set_number(lept_value *v, double n)
{
	lept_free(v);
	v->u.n = n;
	v->type = LEPT_NUMBER;
}

const char *lept_get_string(const lept_value *v)
{
	assert(v != NULL && v->type == LEPT_STRING);
	return v->u.s.s;
}

size_t lept_get_string_length(const lept_value *v)
{
	assert(v != NULL && v->type == LEPT_STRING);	// 非空指针（有具体的字符串）或是零长度的字符串都是合法的。
	return v->u.s.len;
}

// 当设置一个值为字符串时，我们需要把参数中的字符串复制一份
void lept_set_string(lept_value *v, const char *s, size_t len)
{
	assert(v != NULL && (s != NULL || len == 0));
	lept_free(v);
	v->u.s.s = (char*) malloc(len + 1);
	memcpy(v->u.s.s, s, len);
	v->u.s.s[len] = '\0';		// 字符串结尾空字符，表姐字符串的结束
	v->u.s.len = len;
	v->type = LEPT_STRING;
}