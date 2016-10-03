#ifndef LEPTJSON_H__		// include guide
#define LEPTJSON_H__

// JSON ���� 6 ���������ͣ������ true �� false �����������;��� 7 ��
// ��Ϊ C ����û�� C++ �������ռ䣨namespace�����ܣ�һ���ʹ����Ŀ�ļ�д��Ϊ��ʶ����ǰ׺��
// ͨ��ö��ֵ��ȫ��д���� LEPT_NULL���������ͼ���������Сд���� lept_type����
// enum ö����{ ö��ֵ�� }; ��ö��ֵ����Ӧ���г����п���ֵ����ЩֵҲ��Ϊö��Ԫ�ء�
typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

// ���� JSON �����ݽṹ
typedef struct {
    lept_type type;
}lept_value;

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR
};

int lept_parse(lept_value* v, const char* json);

lept_type lept_get_type(const lept_value* v);

#endif /* LEPTJSON_H__ */
