#pragma once

#include <espurna/types.h>

#define UNITY_TEST_ASSERT_EQUAL_STRING_VIEW(e, a, l, m) test_assert_equal_string_view(e, a, l, m)
#define TEST_ASSERT_EQUAL_STRING_VIEW_MESSAGE(e, a, m) test_assert_equal_string_view(e, a, __LINE__, m)
#define TEST_ASSERT_EQUAL_STRING_VIEW(e, a) test_assert_equal_string_view(e, a, __LINE__, NULL)

void test_assert_equal_string_view(espurna::StringView expected, espurna::StringView actual, unsigned int line, const char* msg);

