#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <errno.h>	/* errno */
#include <math.h>	/* HUGE_VAL */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

typedef struct
{
	const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c)
{
	const char *p = c->json;
	while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
		p++;
	c->json = p;
}

static int lept_parse_true(lept_context* c, lept_value* v)
{
	EXPECT(c, 't');
	if(c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
		return LEPT_PARSE_INVALID_VALUE;
	c->json += 3;
	v->type = LEPT_TRUE;
	return LEPT_PARSE_OK;
}

static int lept_parse_false(lept_context* c, lept_value* v)
{
	EXPECT(c, 'f');
	if(c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
		return LEPT_PARSE_INVALID_VALUE;
	c->json += 4;
	v->type = LEPT_FALSE;
	return LEPT_PARSE_OK;
}

static int lept_parse_null(lept_context* c, lept_value* v)
{
	EXPECT(c, 'n');
	if(c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
		return LEPT_PARSE_INVALID_VALUE;
	c->json += 3;
	v->type = LEPT_NULL;
	return LEPT_PARSE_OK;
}

/*
* 练习 1. 重构合并 lept_parse_null()、lept_parse_false()、lept_parse_true 为 lept_parse_literal()
* 由于 true / false / null 的字符数量不一样，这个答案以 for 循环作比较，直至 '\0'。
*/
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

#define ISDIGIT(ch)		((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)	((ch) >= '1' && (ch) <= '9')

/*
* 练习 3. 去掉 test_parse_invalid_value() 和 test_parse_root_not_singular 中的 #if 0 ... #endif，执行测试，证实测试失败。
* 按 JSON number 的语法在 lept_parse_number() 校验，不符合标准的程况返回LEPT_PARSE_INVALID_VALUE　错误码。
*
* 这条题目是本单元的重点，考验同学是否能把语法手写为校验规则。
*
* 首先，如同 lept_parse_whitespace()，我们使用一个指针 p 来表示当前的解析字符位置。这样做有两个好处，一是代码更简单，
* 二是在某些编译器下性能更好（因为不能确定 c 会否被改变，从而每次更改 c->json 都要做一次间接访问）。如果校验成功，才把 p 赋值至 c->json。
*
*/
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
	
	/*
	* 练习4. 去掉 test_parse_number_too_big 中的 #if 0 ... #endif，执行测试，证实测试失败。仔细阅读 strtod()，
	* 看看怎样从返回值得知数值是否过大，以返回 LEPT_PARSE_NUMBER_TOO_BIG 错误码。（提示：这里需要 #include 额外两个标准库头文件。）
	*/
	errno = 0;
	v->n = strtod(c->json, NULL);
	if(errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
	{
		return LEPT_PARSE_NUMBER_TOO_BIG;
	}
	v->type = LEPT_NUMBER;
	c->json = p;

	return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v)
{
	switch(*c->json)
	{
		case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
		case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
		case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
		default:   return lept_parse_number(c, v);
		case '\0': return LEPT_PARSE_EXPECT_VALUE;
	}
}

int lept_parse(lept_value* v, const char* json)
{
	lept_context c;		// 待解析的内容
	int ret;
	assert(v != NULL);
	c.json = json;
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
	return ret;
}

lept_type lept_get_type(const lept_value* v)
{
	assert(v != NULL);
	return v->type;
}

double lept_get_number(const lept_value* v)
{
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->n;
}