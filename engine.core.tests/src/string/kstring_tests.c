#include "string/kstring_tests.h"
#include "test_manager.h"
#include "expect.h"

#include <logger.h>
#include <kstring.h>

u8 string_test1()
{
    bool result = false;

    result = string_equal("A string", "A stringa");
    expect_to_be_false(result);

    result = string_equal(null, "some string");
    expect_to_be_false(result);

    result = string_equal("copy the string", "copy the string");
    expect_to_be_true(result);

    result = string_equal("SOME STRING", "some string");
    expect_to_be_false(result);

    return true;
}

u8 string_test2()
{
    bool result = false;

    result = string_equali("A string", "A STRING");
    expect_to_be_true(result);

    result = string_equali("A string", "A string");
    expect_to_be_true(result);

    result = string_equali("A strinGA", "A STRING");
    expect_to_be_false(result);

    result = string_equali(null, "A STRING");
    expect_to_be_false(result);

    result = string_equali("A STRING", null);
    expect_to_be_false(result);

    return true;
}

u8 string_test3()
{
    bool result = false;
    f32 value = 0.0f;

    result = string_to_f32("6.5", &value);
    expect_to_be_true(result);
    expect_float_to_be(6.5f, value);

    result = string_to_f32("-468.9", &value);
    expect_to_be_true(result);
    expect_float_to_be(-468.9f, value);

    result = string_to_f32("a-468.9", &value);
    expect_to_be_false(result);

    return true;
}

u8 string_test4()
{
    // Работает с буферами только.
    char str1[] = "  string with spaces  ";
    char* str2 = "string with spaces";

    char* rstr = string_trim(str1);
    expect_pointer_should_not_be(null, rstr);
    expect_to_be_true(rstr > str1);
    expect_to_be_true(string_equal(rstr, str2));

    char strz1[] = "a line string";
    char* strz2 = "a line string";

    rstr = string_trim(strz1);
    expect_pointer_should_not_be(null, rstr);
    expect_pointer_should_be(strz1, rstr);
    expect_to_be_true(string_equal(strz2, rstr));

    return true;
}

u8 string_test5()
{
    char rstr[20];
    char* str1 = "This is a simple string.";
    char* tstr1 = "simple";
    char* tstr2 = "simple string.";

    string_mid(rstr, str1, 10, 6);
    expect_to_be_true(string_equal(rstr, tstr1));

    string_mid(rstr, str1, 10, -1);
    expect_to_be_true(string_equal(rstr, tstr2));

    return true;
}

u8 string_test6()
{
    char buffer[1024];
    char test_str[] = "this is a test string";
    
    char* p = string_ncopy(buffer, test_str, 512);
    expect_pointer_should_be(buffer, p);
    expect_should_be(sizeof(test_str) - 1, string_length(p));

    return true;
}

// TODO: Добавить еще тесты.

void string_register_tests()
{
    test_managet_register_test(string_test1, "Function 'string_equal' should compare to strings successfully.");
    test_managet_register_test(string_test2, "Function 'string_equali' should compare to strings successfully.");
    test_managet_register_test(string_test3, "Function 'string_to_f32' should convert a string to value successfully.");
    test_managet_register_test(string_test4, "Function 'string_trim' should trimming a string successfully.");
    test_managet_register_test(string_test5, "Function 'string_mid' should successfully retrieve the substring.");
    test_managet_register_test(string_test6, "Function 'string_ncopy' should copy the string successfully.");
}
