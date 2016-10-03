#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

typedef struct {
    const char* json;	// ��ʱ����ֻ���� json �ַ�����ǰλ�ã�֮��ĵ�Ԫ������Ҫ�����������
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
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

/*
* ��ϰ3. �ο� lept_parse_null() ��ʵ�ֺ͵��÷������� true �� false ֵ
*/
static int lept_parse_true(lept_context *c, lept_value *v)
{
	EXPECT(c, 't');
	if(c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
		return LEPT_PARSE_INVALID_VALUE;
	c->json += 3;
	v->type = LEPT_TRUE;
	return LEPT_PARSE_OK;
}

static int lept_parse_false(lept_context *c, lept_value *v)
{
	EXPECT(c, 'f');
	if(c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
		return LEPT_PARSE_INVALID_VALUE;
	c->json += 4;
	v->type = LEPT_FALSE;
	return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 'n':  return lept_parse_null(c, v);
		case 't':  return lept_parse_true(c, v);
		case 'f':  return lept_parse_false(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        default:   return LEPT_PARSE_INVALID_VALUE;
    }
}

// ���� JSON
int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;		// �� lept_parse() ʧ�ܣ���� v ��Ϊ null ���ͣ����������Ȱ�����Ϊ null���� lept_parse_value() д����������ĸ�ֵ��
    lept_parse_whitespace(&c);
    // return lept_parse_value(&c, v);

	/*
	* ��ϰ1. �������� LEPT_PARSE_ROOT_NOT_SINGULAR �ĵ�Ԫ���ԣ�
	* �� json ��һ��ֵ֮�󣬿հ�֮���������ַ�����Ҫ���� LEPT_PARSE_ROOT_NOT_SINGULAR��
	*
	*/
	int ret = lept_parse_value(&c, v);
	if(ret == LEPT_PARSE_INVALID_VALUE)
		return ret;
	else
	{
		lept_parse_whitespace(&c);
		if(lept_parse_value(&c, v) == LEPT_PARSE_EXPECT_VALUE)
			return ret;
		else
		{
			v->type = LEPT_NULL;
			return LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}