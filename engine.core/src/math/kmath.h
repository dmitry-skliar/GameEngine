#pragma once

#include <defines.h>
#include <math/math_types.h>
#include <platform/math.h>

// Приблизительное представление числа ПИ.
#define K_PI 3.14159265358979323846f

// Приблизительное представление числа ПИ, умноженного на 2.
#define K_2PI (2.0f * K_PI)

// Приблизительное представление числа ПИ, умноженного на 4.
#define K_4PI (4.0f * K_PI)

// Приблизительное представление числа ПИ, деленного на 2.
#define K_HALF_PI (0.5f * K_PI)

// Приблизительное представление числа ПИ, деленного на 4.
#define K_QUARTER_PI (0.25f * K_PI)

// Единица деленная на приблизительное представление числа ПИ.
#define K_ONE_OVER_PI (1.0f / K_PI)

// Единица деленная на приблизительное представление числа ПИ, умноженного на 2.
#define K_ONE_OVER_TWO_PI (1.0f / K_2PI)

// Приближение квадратного корня из 2.
#define K_SQRT_TWO 1.41421356237309504880f

// Приближение квадратного корня из 3.
#define K_SQRT_THREE 1.73205080756887729352f

// Единица деленная на приближенное значение квадратного корня из 2.
#define K_SQRT_ONE_OVER_TWO 0.70710678118654752440f

// Единица деленная на приближенное значение квадратного корня из 3.
#define K_SQRT_ONE_OVER_THREE 0.57735026918962576450f

// Множитель, используемый для преобразования градусов в радианы.
#define K_DEG2RAD_MULTIPLIER (K_PI / 180.0f)

// Множитель, используемый для преобразования радиан в градусы.
#define K_RAD2DEG_MULTIPLIER (180.0f / K_PI)

// Множитель для преобразования секунд в микросекунды.
#define K_SEC_TO_US_MULTIPLIER (1000.0f * 1000.0f)

// Множитель для преобразования секунд в миллисекунды.
#define K_SEC_TO_MS_MULTIPLIER 1000.0f

// Множитель для преобразования миллисекунд в секунды.
#define K_MS_TO_SEC_MULTIPLIER 0.001f

// Огромное число, которое должно быть больше любого допустимого используемого числа.
#define K_INFINITY (1e30f * 1e30f)

// Наименьшее положительное число, где 1.0 + FLOAT_EPSILON != 0.
#define K_FLOAT_EPSILON 1.192092896e-07f

// Максимально отрицательное вещественное число.
#define K_FLOAT_MIN -3.40282e+38F

// Максимально положительное вещественное число.
#define K_FLOAT_MAX 3.40282e+38F

KINLINE f32 ksign(f32 x)
{
    return x == 0.0f ?  0.0f : 
           x <  0.0f ? -1.0f :
                        1.0f ;
}

KINLINE f32 kstep(f32 edge, f32 x)
{
    return x < edge ? 0.0f : 1.0f;
}

/*
    @brief Вычисляет синус числа.
    @param x Число.
    @return Результирующее значение.
*/
#define ksin(x) platform_math_sin(x)

/*
    @brief Вычисляет косинус числа.
    @param x Число.
    @return Результирующее значение.
*/
#define kcos(x) platform_math_cos(x)

/*
    @brief Вычисляет тангенс числа.
    @param x Число.
    @return Результирующее значение.
*/
#define ktan(x) platform_math_tan(x)

/*
    @brief Вычисляет арктангенс числа.
    @param x Число.
    @return Результирующее значение.
*/
#define katan(x) platform_math_atan(x)

/*
    @brief Вычисляет арккосинус числа.
    @param x Число.
    @return Результирующее значение.
*/
#define kacos(x) platform_math_acos(x)

/*
    @brief Вычисляет квадратный корень числа.
    @param x Число.
    @return Результирующее значение.
*/
#define ksqrt(x) platform_math_sqrt(x)

/*
    @brief Вычисляет абсолютное значение числа.
    @param x Число.
    @return Результирующее значение.
*/
#define kabs(x) platform_math_abs(x)

/*
    @brief Возвращает наибольшее целое значение, меньшее или равное числу.
    @param x Число.
    @return Результирующее значение.
*/
#define kfloor(x) platform_math_floor(x)

/*
    @brief Возвращает наименьшее целое значение, большее или равное числу.
    @param x Число.
    @return Результирующее значение.
*/
#define kceil(x) platform_math_ceil(x)

/*
    @brief Вычисляет логарифм числа по основанию 2 (т.е. сколько раз x можно разделить на 2).
    @param x Число.
    @return Результирующее значение.
*/
#define klog2(x) platform_math_log2(x)

/*
    @brief Вычисляет значение возведенного в степень.
    @param x Число.
    @param p Степерь.
    @return Результирующее значение.
*/
#define kpow(x, p) platform_math_pow(x, p)

/*
    @brief Указывает, является ли значение степенью числа 2.
    NOTE: 0 не считается степенью числа 2.
    @param value Значение.
    @return True если степень числа 2, в противном случае false.
*/
KINLINE bool is_power_of_2(u64 value)
{
    return (value != 0) && ((value & (value - 1)) == 0);
}

/*
    @brief Генерирует случайное целое число.
    @return Случайное целое число.
*/
#define krandom() platform_math_random()

/*
    @brief Генерирует случайное целое число, находящееся в указанном диапазоне (включительно).
    @param min Минимум диапазона.
    @param max Максимум диапазона.
    @return Случайное целое число.
*/
#define krandom_in_range(min, max) platform_math_random_in_range(min, max)

/*
    @brief Генерирует случайное число с плавающей точкой.
    @return Случайное число c плавающей точкой.
*/
#define kfrandom() platform_math_frandom()

/*
    @brief Генерирует случайное число с плавающей точкой, находящееся в указанном диапазоне (включительно).
    @param min Минимум диапазона.
    @param max Максимум диапазона.
    @return Случайное число c плавающей точкой.
*/
#define kfrandom_in_range(min, max) platform_math_frandom_in_range(min, max)

/*
    @brief Выполняет интерполяцию Эрмита между двумя значениями.
    @param edge_0 Нижний край функции Эрмита.
    @param edge_1 Верхний край функции Эрмита.
    @param x Значение для интерполяции.
    @return Интерполированное значение.
*/
KINLINE f32 ksmooth_step(f32 edge_0, f32 edge_1, f32 x)
{
    f32 t = KCLAMP((x - edge_0) / (edge_1 - edge_0), 0.0f, 1.0f);
    return t * t * (3.0 - 2.0 * t);
}

/*
    @brief Расчитывает значение затухания на основе расстояния от средней точки min и max.
    @param min Минимальное значение.
    @param max Максимальное значение.
    @param x Значение.
    @return Результирующее значение.
*/
KAPI f32 kattenuation_min_max(f32 min, f32 max, f32 x);

/*
    @brief Сравнивает два числа с плавающей точкой (c применением K_FLOAT_EPSILON).
    @param f0 Первое число.
    @param f1 Второе число.
    @return True если разница числе меньше K_FLOAT_EPSILON, false в остальных случаях.
*/
KINLINE bool kfloat_compare(f32 f0, f32 f1)
{
    return kabs(f0 - f1) < K_FLOAT_EPSILON;
}

/*
    @brief Создает вектор.
    @param x Значение x.
    @param y Значение y.
    @return Вектор.
*/
KINLINE vec2 vec2_create(f32 x, f32 y)
{
    vec2 out_vector;
    out_vector.x = x;
    out_vector.y = y;
    return out_vector;
}

/*
    @brief Создает нулевой вектор.
    @return Нулевой вектор.
*/
KINLINE vec2 vec2_zero()
{
    return (vec2){{0.0f, 0.0f}};
}

/*
    @brief Создает единичный вектор.
    @return Единичный вектор.
*/
KINLINE vec2 vec2_one()
{
    return (vec2){{1.0f, 1.0f}};
}

/*
    @brief Создает вектор направленный вверх (0, 1).
    @return Вектор направленный вверх.
*/
KINLINE vec2 vec2_up()
{
    return (vec2){{0.0f, 1.0f}};
}

/*
    @brief Создает вектор направленный вниз (0, -1).
    @return Вектор направленный вниз.
*/
KINLINE vec2 vec2_down()
{
    return (vec2){{0.0f, -1.0f}};
}

/*
    @brief Создает вектор направленный влево (-1, 0).
    @return Вектор направленный влево.
*/
KINLINE vec2 vec2_left()
{
    return (vec2){{-1.0f, 0.0f}};
}

/*
    @brief Создает вектор направленный вправо (1, 0).
    @return Вектор направленный вправо.
*/
KINLINE vec2 vec2_right()
{
    return (vec2){{1.0f, 0.0f}};
}

/*
    @brief Слаживает два вектора и возвращает копию результата.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Результирующий вектор.
*/
KINLINE vec2 vec2_add(vec2 vector_0, vec2 vector_1)
{
    return (vec2){{
        vector_0.x + vector_1.x,
        vector_0.y + vector_1.y
    }};
}

/*
    @brief Вычитает два вектора и возвращает копию результата.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Результирующий вектор.
*/
KINLINE vec2 vec2_sub(vec2 vector_0, vec2 vector_1)
{
    return (vec2){{
        vector_0.x - vector_1.x,
        vector_1.y - vector_1.y
    }};
}

/*
    @brief Перемножает два вектора и возвращает копию результата.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Результирующий вектор.
*/
KINLINE vec2 vec2_mul(vec2 vector_0, vec2 vector_1)
{
    return (vec2){{
        vector_0.x * vector_1.x,
        vector_1.y * vector_1.y
    }};
}

/*
    @brief Перемножает все элементы вектора на скаляр и возвращает копию результата.
    @param vector Вектор.
    @param scalar Скаляр.
    @return Результирующий вектор.
*/
KINLINE vec2 vec2_mul_scalar(vec2 vector, f32 scalar)
{
    return (vec2){{vector.x * scalar, vector.y * scalar}};
}

/*
    @brief Перемножает два вектора и прибавляет к ним третий, возвращает копию результата.
    @param vector_0 Первый вектор (перемножаемый).
    @param vector_1 Второй вектор (перемножаемый).
    @param vector_2 Третий вектор (складываемый).
    @return Результирующий вектор.
*/
KINLINE vec2 vec2_mul_add(vec2 vector_0, vec2 vector_1, vec2 vector_2)
{
    return (vec2){{
        vector_0.x * vector_1.x + vector_2.x,
        vector_0.y * vector_1.y + vector_2.y
    }};
}

/*
    @brief Делит два вектора и возвращает копию результата.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Результирующий вектор.
*/
KINLINE vec2 vec2_div(vec2 vector_0, vec2 vector_1)
{
    return (vec2){{
        vector_0.x / vector_1.x,
        vector_1.y / vector_1.y
    }};
}

/*
    @brief Вычисляет квадрат длины вектора.
    @param vector Вектор.
    @return Квадрат длинны.
*/
KINLINE f32 vec2_length_squared(vec2 vector)
{
    return vector.x * vector.x + vector.y * vector.y;
}

/*
    @brief Вычисляет длину вектора.
    @param vector Вектор.
    @return Длинна вектора.
*/
KINLINE f32 vec2_length(vec2 vector)
{
    return ksqrt(vec2_length_squared(vector));
}

/*
    @brief Нормализует предоставленный вектор.
    @param vector Вектор.
*/
KINLINE void vec2_normalize(vec2* vector)
{
    const f32 length = vec2_length(*vector);
    vector->x /= length;
    vector->y /= length;
}

/*
    @brief Возвращает копию нормализованного вектора.
    @param vector Вектор.
    @return Копия нормализованого вектора.
*/
KINLINE vec2 vec2_normalized(vec2 vector)
{
    vec2_normalize(&vector);
    return vector;
}

/*
    @brief Сравнивает поэлементную разницу двух векторов на допуск (включительно).
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @param tolerance Допуск различия элементов. Обычно K_FLOAT_EPSILON или аналогично.
    @return True в пределах допуска, false в остальных случаях.
*/
KINLINE bool vec2_compare(vec2 vector_0, vec2 vector_1, f32 tolerance)
{
    if(kabs(vector_0.x - vector_1.x) > tolerance)
    {
        return false;
    }

    if(kabs(vector_0.y - vector_1.y) > tolerance)
    {
        return false;
    }

    return true;
}

/*
    @brief Вычисляет расстояние между 2мя векторами.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Растояние.
*/
KINLINE f32 vec2_distance(vec2 vector_0, vec2 vector_1)
{
    vec2 d = (vec2) {{
        vector_0.x - vector_1.x,
        vector_0.y - vector_1.y
    }};

    return vec2_length(d);
}

/*
    @brief Вычисляет квадрат расстояния между 2мя векторами.
    NOTE: Исключительно для целей сравнения предпочтительнее использовать этот вариант, чем неквадратный.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Квадрат растояния.
*/
KINLINE f32 vec2_distance_squared(vec2 vector_0, vec2 vector_1)
{
    vec2 d = (vec2){{
        vector_0.x - vector_1.x,
        vector_0.y - vector_1.y
    }};
    return vec2_length_squared(d);
}

/*
    @brief Создает вектор.
    @param x Значение x.
    @param y Значение y.
    @param z Значение z.
    @return Вектор.
*/
KINLINE vec3 vec3_create(f32 x, f32 y, f32 z)
{
    return (vec3){{x, y, z}};
}

/*
    @brief Создает 3х элементный вектор из 4х элементного вектора (где w отбрасывается).
    @param vector Вектор из 4х элементов.
    @return Вектор из 3х элементов.
*/
KINLINE vec3 vec3_from_vec4(vec4 vector)
{
    return (vec3){{vector.x, vector.y, vector.z}};
}

/*
    @brief Создает 4х элементный вектор из 3х мерного с добавлением 4го элемента.
    @param vector Вектор из 3х элементов.
    @param w 4ый элемент.
    @return Вектор из 4х элементов.
*/
KINLINE vec4 vec3_to_vec4(vec3 vector, f32 w)
{
    return (vec4){{vector.x, vector.y, vector.z, w}};
}

KINLINE vec3 vec3_min(vec3 vector_0, vec3 vector_1)
{
    return (vec3){{
        KMIN(vector_0.x, vector_1.x),
        KMIN(vector_0.y, vector_1.y),
        KMIN(vector_0.z, vector_1.z)
    }};
}

KINLINE vec3 vec3_max(vec3 vector_0, vec3 vector_1)
{
    return (vec3){{
        KMAX(vector_0.x, vector_1.x),
        KMAX(vector_0.y, vector_1.y),
        KMAX(vector_0.z, vector_1.z)
        
    }};
}

KINLINE vec3 vec3_sign(vec3 v)
{
    return (vec3){{ksign(v.x), ksign(v.y), ksign(v.z)}};
}

/*
    @brief Создает нулевой вектор.
    @return Нулевой вектор.
*/
KINLINE vec3 vec3_zero()
{
    return (vec3){{0.0f, 0.0f, 0.0f}};
}

/*
    @brief Создает единичный вектор.
    @return Единичный вектор.
*/
KINLINE vec3 vec3_one()
{
    return (vec3){{1.0f, 1.0f, 1.0f}};
}

/*
    @brief Создает вектор направленный вверх (0, 1, 0).
    @return Вектор направленный вверх.
*/
KINLINE vec3 vec3_up()
{
    return (vec3){{0.0f, 1.0f, 0.0f}};
}

/*
    @brief Создает вектор направленный вниз (0, -1, 0).
    @return Вектор направленный вниз.
*/
KINLINE vec3 vec3_down()
{
    return (vec3){{0.0f, -1.0f, 0.0f}};
}

/*
    @brief Создает вектор направленный влево (-1, 0, 0).
    @return Вектор направленный влево.
*/
KINLINE vec3 vec3_left()
{
    return (vec3){{-1.0f, 0.0f, 0.0f}};
}

/*
    @brief Создает вектор направленный вправо (1, 0, 0).
    @return Вектор направленный вправо.
*/
KINLINE vec3 vec3_right()
{
    return (vec3){{1.0f, 0.0f, 0.0f}};
}

/*
    @brief Создает вектор направленный вперед (0, 0, -1).
    @return Вектор направленный вперед.
*/
KINLINE vec3 vec3_forward()
{
    return (vec3){{0.0f, 0.0f, -1.0f}};
}

/*
    @brief Создает вектор направленный назад (0, 0, 1).
    @return Вектор направленный назад.
*/
KINLINE vec3 vec3_backward()
{
    return (vec3){{0.0f, 0.0f, 1.0f}};
}

/*
    @brief Слаживает два вектора и возвращает копию результата.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Результирующий вектор.
*/
KINLINE vec3 vec3_add(vec3 vector_0, vec3 vector_1)
{
    return (vec3){{
        vector_0.x + vector_1.x,
        vector_0.y + vector_1.y,
        vector_0.z + vector_1.z
    }};
}

/*
    @brief Вычитает два вектора и возвращает копию результата.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Результирующий вектор.
*/
KINLINE vec3 vec3_sub(vec3 vector_0, vec3 vector_1)
{
    return (vec3){{
        vector_0.x - vector_1.x,
        vector_0.y - vector_1.y,
        vector_0.z - vector_1.z
    }};
}

/*
    @brief Перемножает два вектора и возвращает копию результата.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Результирующий вектор.
*/
KINLINE vec3 vec3_mul(vec3 vector_0, vec3 vector_1)
{
    return (vec3){{
        vector_0.x * vector_1.x,
        vector_0.y * vector_1.y,
        vector_0.z * vector_1.z
    }};
}

/*
    @brief Перемножает все элементы вектора на скаляр и возвращает копию результата.
    @param vector Вектор.
    @param scalar Скаляр.
    @return Результирующий вектор.
*/
KINLINE vec3 vec3_mul_scalar(vec3 vector, f32 scalar)
{
    return (vec3){{
        vector.x * scalar,
        vector.y * scalar,
        vector.z * scalar
    }};
}

/*
    @brief Перемножает два вектора и прибавляет к ним третий, возвращает копию результата.
    @param vector_0 Первый вектор (перемножаемый).
    @param vector_1 Второй вектор (перемножаемый).
    @param vector_2 Третий вектор (складываемый).
    @return Результирующий вектор.
*/
KINLINE vec3 vec3_mul_add(vec3 vector_0, vec3 vector_1, vec3 vector_2)
{
    return (vec3){{
        vector_0.x * vector_1.x + vector_2.x,
        vector_0.y * vector_1.y + vector_2.y,
        vector_0.z * vector_1.z + vector_2.z
    }};
}

/*
    @brief Делит два вектора и возвращает копию результата.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Результирующий вектор.
*/
KINLINE vec3 vec3_div(vec3 vector_0, vec3 vector_1)
{
    return (vec3){{
        vector_0.x / vector_1.x,
        vector_0.y / vector_1.y,
        vector_0.z / vector_1.z
    }};
}

/*
    @brief Делит все элементы вектора на скаляр и возвращает копию результата.
    @param vector Вектор.
    @param scalar Скаляр.
    @return Результирующий вектор.
*/
KINLINE vec3 vec3_div_scalar(vec3 vector, f32 scalar)
{
    return (vec3){{
        vector.x / scalar, vector.y / scalar, vector.z / scalar
    }};
}

/*
    @brief Вычисляет квадрат длины вектора.
    @param vector Вектор.
    @return Квадрат длинны.
*/
KINLINE f32 vec3_length_squared(vec3 vector)
{
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
}

/*
    @brief Вычисляет длину вектора.
    @param vector Вектор.
    @return Длинна.
*/
KINLINE f32 vec3_length(vec3 vector)
{
    return ksqrt(vec3_length_squared(vector));
}

/*
    @brief Нормализует предоставленный вектор.
    @param vector Вектор.
*/
KINLINE void vec3_normalize(vec3* vector)
{
    const f32 length = vec3_length(*vector);
    vector->x /= length;
    vector->y /= length;
    vector->z /= length;
}

/*
    @brief Возвращает копию нормализованного вектора.
    @param vector Вектор.
    @return Копия нормализованого вектора.
*/
KINLINE vec3 vec3_normalized(vec3 vector)
{
    vec3_normalize(&vector);
    return vector;
}

/*
    @brief Вычисляет скалярное произведение двух векторов.
    NOTE: Обычно используется для вычисления разницы в направлении.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Скалярное произведение.
*/
KINLINE f32 vec3_dot(vec3 vector_0, vec3 vector_1)
{
    f32 p = 0;
    p += vector_0.x * vector_1.x;
    p += vector_0.y * vector_1.y;
    p += vector_0.z * vector_1.z;
    return p;
}

/*
    @brief Вычисляет векторное произведение двух векторов.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Новый вектор, ортогональный обоим векторам.
*/
KINLINE vec3 vec3_cross(vec3 vector_0, vec3 vector_1)
{
    return (vec3){{
        vector_0.y * vector_1.z - vector_0.z * vector_1.y,
        vector_0.z * vector_1.x - vector_0.x * vector_1.z,
        vector_0.x * vector_1.y - vector_0.y * vector_1.x
    }};
}

/*
    @brief Сравнивает поэлементную разницу двух векторов на допуск (включительно).
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @param tolerance Допуск различия элементов. Обычно K_FLOAT_EPSILON или аналогично.
    @return True в пределах допуска, false в остальных случаях.
*/
KINLINE bool vec3_compare(vec3 vector_0, vec3 vector_1, f32 tolerance)
{
    if(kabs(vector_0.x - vector_1.x) > tolerance)
    {
        return false;
    }

    if(kabs(vector_0.y - vector_1.y) > tolerance)
    {
        return false;
    }

    if(kabs(vector_0.z - vector_1.z) > tolerance)
    {
        return false;
    }

    return true;
}

/*
    @brief Вычисляет расстояние между 2мя векторами.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Растояние.
*/
KINLINE f32 vec3_distance(vec3 vector_0, vec3 vector_1)
{
    vec3 d = (vec3) {{
        vector_0.x - vector_1.x,
        vector_0.y - vector_1.y,
        vector_0.z - vector_1.z
    }};

    return vec3_length(d);
}

/*
    @brief Вычисляет квадрат расстояния между 2мя векторами.
    NOTE: Исключительно для целей сравнения предпочтительнее использовать этот вариант, чем неквадратный.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Квадрат растояния.
*/
KINLINE f32 vec3_distance_squared(vec3 vector_0, vec3 vector_1)
{
    vec3 d = (vec3){{
        vector_0.x - vector_1.x,
        vector_0.y - vector_1.y,
        vector_0.z - vector_1.z
    }};

    return vec3_length_squared(d);
}

/*
    @brief Преобразовывает вектор с помощью матрицы.
    @param v Вектор.
    @param w Передать 1.0f для точки или 0.0f для направления.
    @param m Матрица.
    @return Преобразованная копия вектора.
*/
KINLINE vec3 vec3_transform(vec3 v, f32 w, mat4 m)
{
    vec3 out;
    out.x = v.x * m.data[0 + 0] + v.y * m.data[4 + 0] + v.z * m.data[8 + 0] + w * m.data[12 + 0];
    out.y = v.x * m.data[0 + 1] + v.y * m.data[4 + 1] + v.z * m.data[8 + 1] + w * m.data[12 + 1];
    out.z = v.x * m.data[0 + 2] + v.y * m.data[4 + 2] + v.z * m.data[8 + 2] + w * m.data[12 + 2];
    return out;
}

/*
    @brief Поворачивает вектор с помощю кватерниона.
    @param v Вектор.
    @param q Кватернион.
    @return Преобразованая копия матрицы.
*/
KINLINE vec3 vec3_rotate(vec3 v, quat q)
{
    vec3 u = {{q.x, q.y, q.z}};
    f32  s = q.w;
    return vec3_add(
                vec3_add(vec3_mul_scalar(u, 2.0f *  vec3_dot(u, v)), vec3_mul_scalar(v,(s * s - vec3_dot(u, u)))),
                vec3_mul_scalar(vec3_cross(u, v), 2.0f * s));
}

/*
    @brief Вычисляет расстояние между точкой и линией.
    @param point Точка.
    @param line_start Начальная точка линии.
    @param line_direction Конечная точка линии.
    @return Растояние.
*/
KAPI f32 vec3_distance_to_line(vec3 point, vec3 line_start, vec3 line_direction);

/*
    @brief Создает вектор.
    @param x Значение x.
    @param y Значение y.
    @param z Значение z.
    @param w Значение w.
    @return Вектор.
*/
KINLINE vec4 vec4_create(f32 x, f32 y, f32 z, f32 w)
{
    vec4 out_vector;
    out_vector.x = x;
    out_vector.y = y;
    out_vector.z = z;
    out_vector.w = w;
    return out_vector;
}

/*
    @brief Усекает 4х элементный вектор (w элемент отбрасывается) и создает 3х элементный вектор.
    @param vector Вектор из 4х элементов.
    @return Вектор из 3х элементов.
*/
KINLINE vec3 vec4_to_vec3(vec4 vector)
{
    return (vec3){{vector.x, vector.y, vector.z}};
}

/*
    @brief Создает 4х элементный вектор из 3х элементного с добавление 4го элемента.
    @param vector Вектор из 3х элементов.
    @param w 4ый элемент.
    @return Вектор из 4х элементов.
*/
KINLINE vec4 vec4_from_vec3(vec3 vector, f32 w)
{
    return (vec4){{vector.x, vector.y, vector.z, w}};
}

/*
    @brief Создает нулевой вектор.
    @return Нулевой вектор.
*/
KINLINE vec4 vec4_zero()
{
    return (vec4){{0.0f, 0.0f, 0.0f, 0.0f}};
}

/*
    @brief Создает единичный вектор.
    @return Единичный вектор.
*/
KINLINE vec4 vec4_one()
{
    return (vec4){{1.0f, 1.0f, 1.0f, 1.0f}};
}

/*
    @brief Слаживает два вектора и возвращает копию результата.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Результирующий вектор.
*/
KINLINE vec4 vec4_add(vec4 vector_0, vec4 vector_1)
{
    vec4 result;
    for(u64 i = 0; i < 4; ++i)
    {
        result.elements[i] = vector_0.elements[i] + vector_1.elements[i];
    }
    return result;
}

/*
    @brief Вычитает два вектора и возвращает копию результата.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Результирующий вектор.
*/
KINLINE vec4 vec4_sub(vec4 vector_0, vec4 vector_1)
{
    vec4 result;
    for(u64 i = 0; i < 4; ++i)
    {
        result.elements[i] = vector_0.elements[i] - vector_1.elements[i];
    }
    return result;
}

/*
    @brief Перемножает два вектора и возвращает копию результата.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Результирующий вектор.
*/
KINLINE vec4 vec4_mul(vec4 vector_0, vec4 vector_1)
{
    vec4 result;
    for(u64 i = 0; i < 4; ++i)
    {
        result.elements[i] = vector_0.elements[i] * vector_1.elements[i];
    }
    return result;
}

/*
    @brief Перемножает все элементы вектора на скаляр и возвращает копию результата.
    @param vector Вектор.
    @param scalar Скаляр.
    @return Результирующий вектор.
*/
KINLINE vec4 vec4_mul_scalar(vec4 vector, f32 scalar)
{
    return (vec4){{
        vector.x * scalar, vector.y * scalar, vector.z * scalar, vector.w * scalar
    }};
}

/*
    @brief Перемножает два вектора и прибавляет к ним третий, возвращает копию результата.
    @param vector_0 Первый вектор (перемножаемый).
    @param vector_1 Второй вектор (перемножаемый).
    @param vector_2 Третий вектор (складываемый).
    @return Результирующий вектор.
*/
KINLINE vec4 vec4_mul_add(vec4 vector_0, vec4 vector_1, vec4 vector_2)
{
    return (vec4){{
        vector_0.x * vector_1.x + vector_2.x,
        vector_0.y * vector_1.y + vector_2.y,
        vector_0.z * vector_1.z + vector_2.z,
        vector_0.w * vector_1.w + vector_2.w,
    }};
}

/*
    @brief Делит два вектора и возвращает копию результата.
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @return Результирующий вектор.
*/
KINLINE vec4 vec4_div(vec4 vector_0, vec4 vector_1)
{
    return (vec4){{
        vector_0.x / vector_1.x,
        vector_0.y / vector_1.y,
        vector_0.z / vector_1.z,
        vector_0.w / vector_1.w,
    }};
}

/*
    @brief Делит все элементы вектора на скаляр и возвращает копию результата.
    @param vector Вектор.
    @param scalar Скаляр.
    @return Результирующий вектор.
*/
KINLINE vec4 vec4_div_scalar(vec4 vector, f32 scalar)
{
    return (vec4){{
        vector.x / scalar,
        vector.y / scalar,
        vector.z / scalar,
        vector.w / scalar
    }};
}

/*
    @brief Вычисляет квадрат длины вектора.
    @param vector Вектор.
    @return Квадрат длинны.
*/
KINLINE f32 vec4_length_squared(vec4 vector)
{
    return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z + vector.w * vector.w;
}

/*
    @brief Вычисляет длину вектора.
    @param vector Вектор.
    @return Длинна.
    
*/
KINLINE f32 vec4_length(vec4 vector)
{
    return ksqrt(vec4_length_squared(vector));
}

/*
    @brief Нормализует предоставленный вектор.
    @param vector Вектор.
*/
KINLINE void vec4_normalize(vec4* vector)
{
    const f32 length = vec4_length(*vector);
    vector->x /= length;
    vector->y /= length;
    vector->z /= length;
    vector->w /= length;
}

/*
    @brief Возвращает копию нормализованного вектора.
    @param vector Вектор.
    @return Копия нормализованого вектора.
*/
KINLINE vec4 vec4_normalized(vec4 vector)
{
    vec4_normalize(&vector);
    return vector;
}

/*
    @brief Вычисляет скалярное произведение, используя элементы vec4, предоставленные в разделенном формате.
    @param a0 - Первый элемент вектора a.
    @param a1 - Второй элемент вектора a.
    @param a2 - Третий элемент вектора a.
    @param a3 - Четвертый элемент вектора a.
    @param b0 - Первый элемент вектора b.
    @param b1 - Второй элемент вектора b.
    @param b2 - Третий элемент вектора b.
    @param b3 - Четвертый элемент вектора b.
    @return Скалярное произведение векторов a и b.
*/
KINLINE f32 vec4_dot(f32 a0, f32 a1, f32 a2, f32 a3, f32 b0, f32 b1, f32 b2, f32 b3)
{
    return (a0 * b0) + (a1 * b1) + (a2 * b2) + (a3 * b3);
}

/*
    @brief Сравнивает поэлементную разницу двух векторов на допуск (включительно).
    @param vector_0 Первый вектор.
    @param vector_1 Второй вектор.
    @param tolerance Допуск различия элементов. Обычно K_FLOAT_EPSILON или аналогично.
    @return True в пределах допуска, false в остальных случаях.
*/
KINLINE bool vec4_compare(vec4 vector_0, vec4 vector_1, f32 tolerance)
{
    for(u64 i = 0; i < 4; ++i)
    {
        if(kabs(vector_0.elements[i] - vector_1.elements[i]) > tolerance)
        {
            return false;
        }
    }
    return true;
}

/*
    @brief Создает еденичную матрицу.
    @return Единичная матрица.
*/
KINLINE mat4 mat4_identity()
{
    mat4 out_matrix = {0};
    out_matrix.data[0]  = 1.0f;
    out_matrix.data[5]  = 1.0f;
    out_matrix.data[10] = 1.0f;
    out_matrix.data[15] = 1.0f;
    return out_matrix;
}

/*
    @brefi Перемножает две матрицы 4x4.
    @param matrix_0 Первая матрица.
    @param matrix_1 Вторая матрица.
    @return Результирующая матрица.
*/
KINLINE mat4 mat4_mul(mat4 matrix_0, mat4 matrix_1)
{
    mat4 out_matrix = mat4_identity();

    const f32* m1 = matrix_0.data;
    const f32* m2 = matrix_1.data;
    f32* dst = out_matrix.data;

    for(i32 i = 0; i < 4; ++i)
    {
        for(i32 j = 0; j < 4; ++j)
        {
            *dst = m1[0] * m2[0 + j] + m1[1] * m2[4 + j] 
                 + m1[2] * m2[8 + j] + m1[3] * m2[12 + j];

            dst++;
        }
        m1 += 4;
    }

    return out_matrix;
}

/*
    @brief Создает матрицу ортогональной проекции.
    NOTE: Обычно используется для визуализации плоских или 2D-сцен.
    @param left Левая сторона усеченной пирамиды вида.
    @param right Правая сторона усеченной пирамиды вида.
    @param bottom Нижняя сторона усеченной пирамиды вида.
    @param top Верхняя сторона усеченной пирамиды вида.
    @param near_clip Расстояние до ближней плоскости отсечения.
    @param far_clip Расстояние до дальней плоскости отсечения.
    @return Матрица ортогональной проекции.
*/
KINLINE mat4 mat4_orthographic(f32 left, f32 right, f32 bottom, f32 top, f32 near_clip, f32 far_clip)
{
    mat4 out_matrix = mat4_identity();
    f32 lr = 1.0f / (left - right);
    f32 bt = 1.0f / (bottom - top);
    f32 nf = 1.0f / (near_clip - far_clip);

    out_matrix.data[0]  = -2.0f * lr;
    out_matrix.data[5]  = -2.0f * bt;
    out_matrix.data[10] =  2.0f * nf;
    out_matrix.data[12] = (left + right) * lr;
    out_matrix.data[13] = (top + bottom) * bt;
    out_matrix.data[14] = (far_clip + near_clip) * nf;
    return out_matrix;
}

/*
    @brief Создает матрицу перспективной проекции.
    NOTE: Обычно используется для визуализации 3D-сцен.
    @param fov_radians Поле зрения в радианах.
    @param aspects_ratio Соотношение сторон.
    @param near_clip Расстояние до ближней плоскости отсечения.
    @param far_clip Расстояние до дальней плоскости отсечения.
    @return Новая матрица перспективы.
*/
KINLINE mat4 mat4_perspective(f32 fov_radians, f32 aspect_ratio, f32 near_clip, f32 far_clip)
{
    f32 half_tan_fov = ktan(fov_radians * 0.5f);
    mat4 out_matrix = {0};
    out_matrix.data[0]  = 1.0f / (aspect_ratio * half_tan_fov);
    out_matrix.data[5]  = 1.0f / half_tan_fov;
    out_matrix.data[10] = -((far_clip + near_clip) / (far_clip - near_clip));
    out_matrix.data[11] = -1.0f;
    out_matrix.data[14] = -((2.0f * far_clip * near_clip) / (far_clip - near_clip));
    return out_matrix;
}

/*
    @brief Создает матрицу с направлением в сторону точки.
    @param position Матрица позиции.
    @param target Точка на которую нужно "смотреть".
    @param up Вектор вверха.
    @return Матрица, смотрящая на цель с точки зрения позиции.
*/
KINLINE mat4 mat4_look_at(vec3 position, vec3 target, vec3 up)
{
    // RH
    mat4 out_matrix;
    vec3 z_axis;
    z_axis.x = target.x - position.x;
    z_axis.y = target.y - position.y;
    z_axis.z = target.z - position.z;

    z_axis = vec3_normalized(z_axis);
    vec3 x_axis = vec3_normalized(vec3_cross(z_axis, up));
    vec3 y_axis = vec3_cross(x_axis, z_axis);

    out_matrix.data[0]  = x_axis.x;
    out_matrix.data[1]  = y_axis.x;
    out_matrix.data[2]  = -z_axis.x;
    out_matrix.data[3]  = 0;
    out_matrix.data[4]  = x_axis.y;
    out_matrix.data[5]  = y_axis.y;
    out_matrix.data[6]  = -z_axis.y;
    out_matrix.data[7]  = 0;
    out_matrix.data[8]  = x_axis.z;
    out_matrix.data[9]  = y_axis.z;
    out_matrix.data[10] = -z_axis.z;
    out_matrix.data[11] = 0;
    out_matrix.data[12] = -vec3_dot(x_axis, position);
    out_matrix.data[13] = -vec3_dot(y_axis, position);
    out_matrix.data[14] = vec3_dot(z_axis, position);
    out_matrix.data[15] = 1.0f;

    // LH
    // vec3 f = vec3_normalized(vec3_sub(target, position));
    // vec3 s = vec3_normalized(vec3_cross(f, up));
    // vec3 u = vec3_cross(s, f);

    // mat4 Result = mat4_identity();
    // Result.data[0]  = s.x;
    // Result.data[4]  = s.y;
    // Result.data[8]  = s.z;
    // Result.data[1]  = u.x;
    // Result.data[5]  = u.y;
    // Result.data[9]  = u.z;
    // Result.data[2]  =-f.x;
    // Result.data[6]  =-f.y;
    // Result.data[10] =-f.z;
    // Result.data[12] =-vec3_dot(s, position);
    // Result.data[13] =-vec3_dot(u, position);
    // Result.data[14] = vec3_dot(f, position);
    // return Result;

    return out_matrix;
}

/*
    @brief Возвращает транспонированную копию матрицы (строки->столбцы).
    @param matrix Матрица.
    @return Транспонированная копия матрицы.
*/
KINLINE mat4 mat4_transposed(mat4 matrix)
{
    mat4 out_matrix = mat4_identity();
    out_matrix.data[0]  = matrix.data[0];
    out_matrix.data[1]  = matrix.data[4];
    out_matrix.data[2]  = matrix.data[8];
    out_matrix.data[3]  = matrix.data[12];
    out_matrix.data[4]  = matrix.data[1];
    out_matrix.data[5]  = matrix.data[5];
    out_matrix.data[6]  = matrix.data[9];
    out_matrix.data[7]  = matrix.data[13];
    out_matrix.data[8]  = matrix.data[2];
    out_matrix.data[9]  = matrix.data[6];
    out_matrix.data[10] = matrix.data[10];
    out_matrix.data[11] = matrix.data[14];
    out_matrix.data[12] = matrix.data[3];
    out_matrix.data[13] = matrix.data[7];
    out_matrix.data[14] = matrix.data[11];
    out_matrix.data[15] = matrix.data[15];
    return out_matrix;
}

/*
    @brief Вычисляет определитель матрицы.
    @param matrix Матрица.
    @return Определитель матрицы.
*/
KINLINE f32 mat4_determinant(mat4 matrix)
{
    const f32 *m = matrix.data;
    f32 t0  = m[10] * m[15];
    f32 t1  = m[14] * m[11];
    f32 t2  = m[6]  * m[15];
    f32 t3  = m[14] * m[7];
    f32 t4  = m[6]  * m[11];
    f32 t5  = m[10] * m[7];
    f32 t6  = m[2]  * m[15];
    f32 t7  = m[14] * m[3];
    f32 t8  = m[2]  * m[11];
    f32 t9  = m[10] * m[3];
    f32 t10 = m[2]  * m[7];
    f32 t11 = m[6]  * m[3];

    mat3 temp_mat;
    f32 *o = temp_mat.data;
    o[0] = (t0 * m[5] + t3 * m[9] + t4  * m[13]) - (t1 * m[5] + t2 * m[9] + t5  * m[13]);
    o[1] = (t1 * m[1] + t6 * m[9] + t9  * m[13]) - (t0 * m[1] + t7 * m[9] + t8  * m[13]);
    o[2] = (t2 * m[1] + t7 * m[5] + t10 * m[13]) - (t3 * m[1] + t6 * m[5] + t11 * m[13]);
    o[3] = (t5 * m[1] + t8 * m[5] + t11 * m[9])  - (t4 * m[1] + t9 * m[5] + t10 * m[9]);

    return 1.0f / (m[0] * o[0] + m[4] * o[1] + m[8] * o[2] + m[12] * o[3]);
}

/*
    @brief Создает обратную матрицу предоставленной матрицы.
    @param matrix Матрица.
    @return Копия обратной матрицы.
*/
KINLINE mat4 mat4_inverse(mat4 matrix)
{
    const f32 *m = matrix.data;
    f32 t0  = m[10] * m[15];
    f32 t1  = m[14] * m[11];
    f32 t2  = m[6]  * m[15];
    f32 t3  = m[14] * m[7];
    f32 t4  = m[6]  * m[11];
    f32 t5  = m[10] * m[7];
    f32 t6  = m[2]  * m[15];
    f32 t7  = m[14] * m[3];
    f32 t8  = m[2]  * m[11];
    f32 t9  = m[10] * m[3];
    f32 t10 = m[2]  * m[7];
    f32 t11 = m[6]  * m[3];
    f32 t12 = m[8]  * m[13];
    f32 t13 = m[12] * m[9];
    f32 t14 = m[4]  * m[13];
    f32 t15 = m[12] * m[5];
    f32 t16 = m[4]  * m[9];
    f32 t17 = m[8]  * m[5];
    f32 t18 = m[0]  * m[13];
    f32 t19 = m[12] * m[1];
    f32 t20 = m[0]  * m[9];
    f32 t21 = m[8]  * m[1];
    f32 t22 = m[0]  * m[5];
    f32 t23 = m[4]  * m[1];

    mat4 out_matrix;
    f32 *o = out_matrix.data;
    o[0] = (t0 * m[5] + t3 * m[9] + t4  * m[13]) - (t1 * m[5] + t2 * m[9] + t5  * m[13]);
    o[1] = (t1 * m[1] + t6 * m[9] + t9  * m[13]) - (t0 * m[1] + t7 * m[9] + t8  * m[13]);
    o[2] = (t2 * m[1] + t7 * m[5] + t10 * m[13]) - (t3 * m[1] + t6 * m[5] + t11 * m[13]);
    o[3] = (t5 * m[1] + t8 * m[5] + t11 * m[9])  - (t4 * m[1] + t9 * m[5] + t10 * m[9]);

    f32 d = 1.0f / (m[0] * o[0] + m[4] * o[1] + m[8] * o[2] + m[12] * o[3]);
    o[0]  = d * o[0];
    o[1]  = d * o[1];
    o[2]  = d * o[2];
    o[3]  = d * o[3];
    o[4]  = d * ((t1  * m[4]  + t2  * m[8]  + t5  * m[12]) - (t0  * m[4]  + t3  * m[8]  + t4  * m[12]));
    o[5]  = d * ((t0  * m[0]  + t7  * m[8]  + t8  * m[12]) - (t1  * m[0]  + t6  * m[8]  + t9  * m[12]));
    o[6]  = d * ((t3  * m[0]  + t6  * m[4]  + t11 * m[12]) - (t2  * m[0]  + t7  * m[4]  + t10 * m[12]));
    o[7]  = d * ((t4  * m[0]  + t9  * m[4]  + t10 * m[8])  - (t5  * m[0]  + t8  * m[4]  + t11 * m[8]));
    o[8]  = d * ((t12 * m[7]  + t15 * m[11] + t16 * m[15]) - (t13 * m[7]  + t14 * m[11] + t17 * m[15]));
    o[9]  = d * ((t13 * m[3]  + t18 * m[11] + t21 * m[15]) - (t12 * m[3]  + t19 * m[11] + t20 * m[15]));
    o[10] = d * ((t14 * m[3]  + t19 * m[7]  + t22 * m[15]) - (t15 * m[3]  + t18 * m[7]  + t23 * m[15]));
    o[11] = d * ((t17 * m[3]  + t20 * m[7]  + t23 * m[11]) - (t16 * m[3]  + t21 * m[7]  + t22 * m[11]));
    o[12] = d * ((t14 * m[10] + t17 * m[14] + t13 * m[6])  - (t16 * m[14] + t12 * m[6]  + t15 * m[10]));
    o[13] = d * ((t20 * m[14] + t12 * m[2]  + t19 * m[10]) - (t18 * m[10] + t21 * m[14] + t13 * m[2]));
    o[14] = d * ((t18 * m[6]  + t23 * m[14] + t15 * m[2])  - (t22 * m[14] + t14 * m[2]  + t19 * m[6]));
    o[15] = d * ((t22 * m[10] + t16 * m[2]  + t21 * m[6])  - (t20 * m[6]  + t23 * m[10] + t17 * m[2]));

    return out_matrix;
}

/*
    @brief Создает матрицу перемещения в заданную позицию.
    @param position Позиция.
    @return Матрица перемещения.
*/
KINLINE mat4 mat4_translation(vec3 position)
{
    mat4 out_matrix = mat4_identity();
    out_matrix.data[12] = position.x;
    out_matrix.data[13] = position.y;
    out_matrix.data[14] = position.z;
    return out_matrix;
}

/*
    @brief Создает матрицу масштаба.
    @param scale Масштаб.
    @return Матрица масштаба.
*/
KINLINE mat4 mat4_scale(vec3 scale)
{
    mat4 out_matrix = mat4_identity();
    out_matrix.data[0]  = scale.x;
    out_matrix.data[5]  = scale.y;
    out_matrix.data[10] = scale.z;
    return out_matrix;
}

/*
    @brief Создает матрицу трансформации из позиции, поворота и масштаба (TRS).
    @param position Позиция.
    @param rotation Поворот.
    @param scale Масштаб.
    @return Матрица (в порядке TRS).
*/
KINLINE mat4 mat4_from_translation_rotation_scale(vec3 t, quat r, vec3 s)
{
    mat4 out_matrix;
    out_matrix.data[0]  = (1.0f - 2.0f * (r.y * r.y + r.z * r.z)) * s.x;
    out_matrix.data[1]  = (r.x * r.y + r.z * r.w) * s.x * 2.0f;
    out_matrix.data[2]  = (r.x * r.z - r.y * r.w) * s.x * 2.0f;
    out_matrix.data[3]  = 0.0f;
    out_matrix.data[4]  = (r.x * r.y - r.z * r.w) * s.y * 2.0f;
    out_matrix.data[5]  = (1.0f - 2.0f * (r.x * r.x + r.z * r.z)) * s.y;
    out_matrix.data[6]  = (r.y * r.z + r.x * r.w) * s.y * 2.0f;
    out_matrix.data[7]  = 0.0f;
    out_matrix.data[8]  = (r.x * r.z + r.y * r.w) * s.z * 2.0f;
    out_matrix.data[9]  = (r.y * r.z - r.x * r.w) * s.z * 2.0f;
    out_matrix.data[10] = (1.0f - 2.0f * (r.x * r.x + r.y * r.y)) * s.z;
    out_matrix.data[11] = 0.0f;
    out_matrix.data[12] = t.x;
    out_matrix.data[13] = t.y;
    out_matrix.data[14] = t.z;
    out_matrix.data[15] = 1.0f;
    return out_matrix;
}

/*
    @brief Создает матрицу вращения по оси x.
    @param angle_radians Угол x в радианах.
    @return Матрица поворота.
*/
KINLINE mat4 mat4_euler_x(f32 angle_radians)
{
    mat4 out_matrix = mat4_identity();
    f32 c = kcos(angle_radians);
    f32 s = ksin(angle_radians);
    out_matrix.data[5]  =  c;
    out_matrix.data[6]  =  s;
    out_matrix.data[9]  = -s;
    out_matrix.data[10] =  c;
    return out_matrix;
}

/*
    @brief Создает матрицу вращения по оси y.
    @param angle_radians Угол y в радианах.
    @return Матрица поворота.
*/
KINLINE mat4 mat4_euler_y(f32 angle_radians)
{
    mat4 out_matrix = mat4_identity();
    f32 c = kcos(angle_radians);
    f32 s = ksin(angle_radians);
    out_matrix.data[0]  =  c;
    out_matrix.data[2]  = -s;
    out_matrix.data[8]  =  s;
    out_matrix.data[10] =  c;
    return out_matrix;
}

/*
    @brief Создает матрицу вращения по оси z.
    @param angle_radians Угол z в радианах.
    @return Матрица поворота.
*/
KINLINE mat4 mat4_euler_z(f32 angle_radians)
{
    mat4 out_matrix = mat4_identity();
    f32 c = kcos(angle_radians);
    f32 s = ksin(angle_radians);
    out_matrix.data[0] =  c;
    out_matrix.data[1] =  s;
    out_matrix.data[4] = -s;
    out_matrix.data[5] =  c;
    return out_matrix;
}

/*
    @brief Создает матрицу вращения по осям x, y и z.
    @param x_radians Угол x в радианах.
    @param y_radians Угол y в радианах.
    @param z_radians Угол z в радианах.
    @return Матрица вращения.
*/
KINLINE mat4 mat4_euler_xyz(f32 x_radians, f32 y_radians, f32 z_radians)
{
    mat4 rx = mat4_euler_x(x_radians);
    mat4 ry = mat4_euler_y(y_radians);
    mat4 rz = mat4_euler_z(z_radians);
    mat4 out_matrix = mat4_mul(rx, ry);
    out_matrix = mat4_mul(out_matrix, rz);

    return out_matrix;
}

/*
    @brief Возвращает вектор направленный прямо относительно предоставленной матрицы.
    @param matrix Матрица, на которой базируется вектор.
    @return Направленный вектор.
*/
KINLINE vec3 mat4_forward(mat4 matrix)
{
    vec3 forward;
    forward.x = -matrix.data[2];
    forward.y = -matrix.data[6];
    forward.z = -matrix.data[10];
    vec3_normalize(&forward);
    return forward;
}

/*
    @brief Возвращает вектор направленный назад относительно предоставленной матрицы.
    @param matrix Матрица, на которой базируется вектор.
    @return Направленный вектор.
*/
KINLINE vec3 mat4_backward(mat4 matrix)
{
    vec3 backward;
    backward.x = matrix.data[2];
    backward.y = matrix.data[6];
    backward.z = matrix.data[10];
    vec3_normalize(&backward);
    return backward;
}

/*
    @brief Возвращает вектор направленный вверх относительно предоставленной матрицы.
    @param matrix Матрица, на которой базируется вектор.
    @return Направленный вектор.
*/
KINLINE vec3 mat4_up(mat4 matrix)
{
    vec3 up;
    up.x = matrix.data[1];
    up.y = matrix.data[5];
    up.z = matrix.data[9];
    vec3_normalize(&up);
    return up;
}

/*
    @brief Возвращает вектор направленный вниз относительно предоставленной матрицы.
    @param matrix Матрица, на которой базируется вектор.
    @return Направленный вектор.
*/
KINLINE vec3 mat4_down(mat4 matrix)
{
    vec3 down;
    down.x = -matrix.data[1];
    down.y = -matrix.data[5];
    down.z = -matrix.data[9];
    vec3_normalize(&down);
    return down;
}

/*
    @brief Возвращает вектор направленный влево относительно предоставленной матрицы.
    @param matrix Матрица, на которой базируется вектор.
    @return Направленный вектор.
*/
KINLINE vec3 mat4_left(mat4 matrix)
{
    vec3 left;
    left.x = -matrix.data[0];
    left.y = -matrix.data[4];
    left.z = -matrix.data[8];
    vec3_normalize(&left);
    return left;
}

/*
    @brief Возвращает вектор направленный вправо относительно предоставленной матрицы.
    @param matrix Матрица, на которой базируется вектор.
    @return Направленный вектор.
*/
KINLINE vec3 mat4_right(mat4 matrix)
{
    vec3 right;
    right.x = matrix.data[0];
    right.y = matrix.data[4];
    right.z = matrix.data[8];
    vec3_normalize(&right);
    return right;
}

/*
    @brief Возвращает позицию относительно предоставленной матрицы.
    @param matrix Матрица, на основе которой строится вектор.
    @return Вектор позиции.
*/
KINLINE vec3 mat4_position(mat4 matrix)
{
    vec3 pos;
    pos.x = matrix.data[12];
    pos.y = matrix.data[13];
    pos.z = matrix.data[14];
    return pos;
}

/*
    @brief Выполняет m * v.
    @param m Матрица.
    @param v Вектор.
    @return Преобразованный вектор.
*/
KINLINE vec3 mat4_mul_vec3(mat4 m, vec3 v)
{
    return (vec3){{
        (v.x * m.data[0] + v.y * m.data[1] + v.z * m.data[2]  + m.data[3]),
        (v.x * m.data[4] + v.y * m.data[5] + v.z * m.data[6]  + m.data[7]),
        (v.x * m.data[8] + v.y * m.data[9] + v.z * m.data[10] + m.data[11])
    }};
}

/*
    @brief Выполняет v * m.
    @param v Вектор.
    @param m Матрица.
    @return Преобразованный вектор.
*/
KINLINE vec3 vec3_mul_mat4(vec3 v, mat4 m)
{
    return (vec3){{
        (v.x * m.data[0] + v.y * m.data[4] + v.z * m.data[8]  + m.data[12]),
        (v.x * m.data[1] + v.y * m.data[5] + v.z * m.data[9]  + m.data[13]),
        (v.x * m.data[2] + v.y * m.data[6] + v.z * m.data[10] + m.data[14])
    }};
}

/*
    @brief Выполняет m * v.
    @param m Матрица.
    @param v Вектор.
    @return Преобразованный вектор.
*/
KINLINE vec4 mat4_mul_vec4(mat4 m, vec4 v)
{
    return (vec4){{
        (v.x * m.data[0]  + v.y * m.data[1]  + v.z * m.data[2]  + v.w * m.data[3]),
        (v.x * m.data[4]  + v.y * m.data[5]  + v.z * m.data[6]  + v.w * m.data[7]),
        (v.x * m.data[8]  + v.y * m.data[9]  + v.z * m.data[10] + v.w * m.data[11]),
        (v.x * m.data[12] + v.y * m.data[13] + v.z * m.data[14] + v.w * m.data[15])
    }};
}

/*
    @brief Выполняет v * m.
    @param v Вектор.
    @param m Матрица.
    @return Преобразованный вектор.
*/
KINLINE vec4 vec4_mul_mat4(vec4 v, mat4 m)
{
    return (vec4){{
        (v.x * m.data[0] + v.y * m.data[4] + v.z * m.data[8]  + v.w * m.data[12]),
        (v.x * m.data[1] + v.y * m.data[5] + v.z * m.data[9]  + v.w * m.data[13]),
        (v.x * m.data[2] + v.y * m.data[6] + v.z * m.data[10] + v.w * m.data[14]),
        (v.x * m.data[3] + v.y * m.data[7] + v.z * m.data[11] + v.w * m.data[15])
    }};
}

/*
    @brief Создает кватернион.
    @return Кватернион.
*/
KINLINE quat quat_identity()
{
    return (quat){{0.0f, 0.0f, 0.0f, 1.0f}};
}

/*
    @brief Возвращает нормаль предоставленного кватерниона.
    @param q Кватернион.
    @return Нормаль кватерниона.
*/
KINLINE f32 quat_normal(quat q)
{
    return ksqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

/*
    @brief Возвращает нормализованную копию предоставленного кватерниона.
    @param q Кватернион.
    @return Нормализованная копия кватерниона.
*/
KINLINE quat quat_normalize(quat q)
{
    f32 normal = quat_normal(q);
    return (quat){{
        q.x / normal, q.y / normal, q.z / normal, q.w / normal
    }};
}

/*
    @brief Возвращает сопряжение предоставленного кватерниона.
    NOTE: То есть элементы x, y и z инвертируются, но элемент w остается нетронутым.
    @param q Квыатернион, для которого необходимо получить сопряжение.
    @return Сопряженный кватернион.
*/
KINLINE quat quat_conjugate(quat q)
{
    return (quat){{
        -q.x, -q.y, -q.z, q.w
    }};
}

/*
    @brief Возвращает обратную копию предоставленного кватерниона.
    @param q Кватернион для инвертирования.
    @return Обратная копия кватерниона.
*/
KINLINE quat quat_inverse(quat q)
{
    return quat_normalize(quat_conjugate(q));
}

/*
    @brief Умножает предоставленные кватернионы.
    @param q_0 Первый кватернион.
    @param q_1 Второй кватернион.
    @return Результирующий кватернион.
*/
KINLINE quat quat_mul(quat q_0, quat q_1)
{
    quat out_quaternion;
    out_quaternion.x =  q_0.x * q_1.w + q_0.y * q_1.z 
                     -  q_0.z * q_1.y + q_0.w * q_1.x;
    out_quaternion.y = -q_0.x * q_1.z + q_0.y * q_1.w 
                     +  q_0.z * q_1.x + q_0.w * q_1.y;
    out_quaternion.z =  q_0.x * q_1.y - q_0.y * q_1.x 
                     +  q_0.z * q_1.w + q_0.w * q_1.z;
    out_quaternion.w = -q_0.x * q_1.x - q_0.y * q_1.y 
                     -  q_0.z * q_1.z + q_0.w * q_1.w;
    return out_quaternion;
}

/*
    @brief Вычисляет скалярное произведение предоставленных кватернионов.
    @param q_0 Первый кватернион.
    @param q_1 Второй кватернион.
    @return Скалярное произведение кватернионов.
*/
KINLINE f32 quat_dot(quat q_0, quat q_1)
{
    return q_0.x * q_1.x + q_0.y * q_1.y + q_0.z * q_1.z + q_0.w * q_1.w;
}

/*
    @brief Создает матрицу вращения из заданного кватерниона.
    @param q Кватернион.
    @return Матрица вращения.
*/
KINLINE mat4 quat_to_mat4(quat q)
{
    mat4 out_matrix = mat4_identity();
    quat n = quat_normalize(q);
    out_matrix.data[0]  = 1.0f - 2.0f * n.y * n.y  - 2.0f * n.z * n.z;
    out_matrix.data[1]  = 2.0f * n.x  * n.y - 2.0f * n.z  * n.w;
    out_matrix.data[2]  = 2.0f * n.x  * n.z + 2.0f * n.y  * n.w;

    out_matrix.data[4]  = 2.0f * n.x  * n.y + 2.0f * n.z  * n.w;
    out_matrix.data[5]  = 1.0f - 2.0f * n.x * n.x  - 2.0f * n.z * n.z;
    out_matrix.data[6]  = 2.0f * n.y  * n.z - 2.0f * n.x  * n.w;

    out_matrix.data[8]  = 2.0f * n.x  * n.z - 2.0f * n.y  * n.w;
    out_matrix.data[9]  = 2.0f * n.y  * n.z + 2.0f * n.x  * n.w;
    out_matrix.data[10] = 1.0f - 2.0f * n.x * n.x  - 2.0f * n.y * n.y;
    return out_matrix;
}

/*
    @brief Вычисляет матрицу вращения на основе кватерниона и переданной центральной точки.
    @param q Кватернион.
    @param center Центральная точка.
    @return Матрица вращения.
*/
KINLINE mat4 quat_to_rotation_matrix(quat q, vec3 center)
{
    mat4 out_matrix;

    f32 *o = out_matrix.data;
    o[0]  = (q.x  * q.x)  - (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    o[1]  =  2.0f * ((q.x * q.y) + (q.z * q.w));
    o[2]  =  2.0f * ((q.x * q.z) - (q.y * q.w));
    o[3]  = center.x - center.x * o[0] - center.y * o[1] - center.z * o[2];

    o[4]  =  2.0f * ((q.x * q.y) - (q.z * q.w));
    o[5]  = -(q.x * q.x)  + (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
    o[6]  =  2.0f * ((q.y * q.z) + (q.x * q.w));
    o[7]  = center.y - center.x * o[4] - center.y * o[5] - center.z * o[6];

    o[8]  =  2.0f  * ((q.x * q.z) + (q.y * q.w));
    o[9]  =  2.0f  * ((q.y * q.z) - (q.x * q.w));
    o[10] = -(q.x  * q.x)  - (q.y * q.y) + (q.z * q.z) + (q.w * q.w);
    o[11] = center.z - center.x * o[8] - center.y * o[9] - center.z * o[10];

    o[12] = 0.0f;
    o[13] = 0.0f;
    o[14] = 0.0f;
    o[15] = 1.0f;
    return out_matrix;
}

/*
    @brief Создает кватернион из заданной оси и угла.
    @param axis Ось вращения.
    @param angle Угол вращения.
    @param normalize Указывает, следует ли нормализовать кватернион.
    @return Новый кватернион.
*/
KINLINE quat quat_from_axis_angle(vec3 axis, f32 angle, bool normalize)
{
    const f32 half_angle = 0.5f * angle;
    f32 s = ksin(half_angle);
    f32 c = kcos(half_angle);

    quat q = (quat){{
        s * axis.x, s * axis.y, s * axis.z, c
    }};

    if (normalize)
    {
        return quat_normalize(q);
    }

    return q;
}

/*
    @brief Вычисляет сферическую линейную интерполяцию заданного процента между двумя кватернионами.
    @param q_0 Первый кватернион.
    @param q_1 Второй кватернион.
    @param percentage Процент интерполяции, обычно значение от 0.0f до 1.0f.
    @return Интерполированный кватернион.
*/
KINLINE quat quat_slerp(quat q_0, quat q_1, f32 percentage)
{
    quat out_quaternion;
    // Только единичные кватернионы являются допустимыми вращениями.
    // Нормализуйте, чтобы избежать неопределенного поведения.
    quat v0 = quat_normalize(q_0);
    quat v1 = quat_normalize(q_1);

    // Вычислить косинус угла между двумя векторами.
    f32 dot = quat_dot(v0, v1);

    // Если скалярное произведение отрицательно, slerp не выберет более короткий путь.
    // Обратите внимание, что v1 и -v1 эквивалентны, когда отрицание применяется ко
    // всем четырем компонентам. Исправьте, перевернув один кватернион.
    if(dot < 0.0f)
    {
        v1.x = -v1.x;
        v1.y = -v1.y;
        v1.z = -v1.z;
        v1.w = -v1.w;
        dot  = -dot;
    }

    const f32 DOT_THRESHOLD = 0.9995f;
    if(dot > DOT_THRESHOLD)
    {
        // Если входные данные слишком близки для комфорта, линейно интерполируйте
        // и нормализуйте результат.
        out_quaternion = (quat){{
            (v0.x + ((v1.x - v0.x) * percentage)),
            (v0.y + ((v1.y - v0.y) * percentage)),
            (v0.z + ((v1.z - v0.z) * percentage)),
            (v0.w + ((v1.w - v0.w) * percentage))
        }};

        return quat_normalize(out_quaternion);
    }

    // Поскольку точка находится в диапазоне [0, DOT_THRESHOLD], acos безопасен.
    f32 theta_0 = kacos(dot);          // theta_0 = угол между входными векторами
    f32 theta = theta_0 * percentage;  // тета = угол между v0 и результатом
    f32 sin_theta = ksin(theta);       // вычислить это значение только один раз
    f32 sin_theta_0 = ksin(theta_0);   // вычислить это значение только один раз

    // == sin(theta_0 - theta) / sin(theta_0)
    f32 s0 = kcos(theta) - dot * sin_theta / sin_theta_0;
    f32 s1 = sin_theta / sin_theta_0;

    return (quat){{
        (v0.x * s0) + (v1.x * s1), (v0.y * s0) + (v1.y * s1),
        (v0.z * s0) + (v1.z * s1), (v0.w * s0) + (v1.w * s1)
    }};
}

/*
    @brief Преобразует предоставленные градусы в радианы.
    @param degrees Градусы для преобразования.
    @return Количество в радианах.
*/
KINLINE f32 deg_to_rad(f32 degrees)
{
    return degrees * K_DEG2RAD_MULTIPLIER;
}

/*
    @brief Преобразует предоставленные радианы в градусы.
    @param radians Радианы для преобразования.
    @return Количество в градусах.
*/
KINLINE f32 rad_to_deg(f32 radians)
{
    return radians * K_RAD2DEG_MULTIPLIER;
}

/*
    @brief Преобразует значение из «старого» диапазона в «новый».
    @param value Значение для преобразования.
    @param from_min Минимальное значение из старого диапазона.
    @param from_max Максимальное значение из старого диапазона.
    @param to_min Минимальное значение из нового диапазона.
    @param to_max Максимальное значение из нового диапазона.
    @return Преобразованное значение.
*/
KINLINE f32 range_convert_f32(f32 value, f32 old_min, f32 old_max, f32 new_min, f32 new_max)
{
    return (((value - old_min) * (new_max - new_min)) / (old_max - old_min)) + new_min;
}

/*
    @brief Преобразует значения rgb int [0-255] в одно 32-битное целое число.
    @param r Значение красного [0-255].
    @param g Значение зеленого [0-255].
    @param b Значение синего [0-255].
    @param out_u32 Указатель для хранения полученного целого числа.
*/
KINLINE void rgb_to_u32(u32 r, u32 g, u32 b, u32 *out_u32)
{
    *out_u32 = (((r & 0x0FF) << 16) | ((g & 0x0FF) << 8) | (b & 0x0FF));
}

/*
    @brief Преобразует заданное 32-битное целое число в значения RGB [0-255].
    @param rgbu Целое число, содержащее значение RGB.
    @param out_r Указатель для хранения значения красного.
    @param out_g Указатель для хранения значения зеленого.
    @param out_b Указатель для хранения значения синего.
*/
KINLINE void u32_to_rgb(u32 rgbu, u32 *out_r, u32 *out_g, u32 *out_b)
{
    *out_r = (rgbu >> 16) & 0x0FF;
    *out_g = (rgbu >> 8) & 0x0FF;
    *out_b = (rgbu) & 0x0FF;
}

/*
    @brief Преобразует целочисленные значения RGB [0-255] в vec3 значений с плавающей точкой [0.0-1.0].
    @param r Значение красного [0-255].
    @param g Значение зеленого [0-255].
    @param b Значение синего [0-255].
    @param out_v Указатель для хранения вектора значений с плавающей точкой.
*/
KINLINE void rgb_to_vec3(u32 r, u32 g, u32 b, vec3 *out_v)
{
    out_v->r = r / 255.0f;
    out_v->g = g / 255.0f;
    out_v->b = b / 255.0f;
}

/*
    @brief Преобразует vec3 значений rgb [0.0-1.0] в целые значения rgb [0-255].
    @param v Вектор значений rgb [0.0-1.0], который нужно преобразовать.
    @param out_r Указатель для хранения значения красного.
    @param out_g Указатель для хранения значения зеленого.
    @param out_b Указатель для хранения значения синего.
*/
KINLINE void vec3_to_rgb(vec3 v, u32 *out_r, u32 *out_g, u32 *out_b)
{
    *out_r = v.r * 255;
    *out_g = v.g * 255;
    *out_b = v.b * 255;
}

/*
*/
KAPI plane_3d plane_3d_create(vec3 p1, vec3 norm);

/*
    @brief Создает усеченную пирамиду на основе предоставленного положения, направления, аспекта,
           поля зрения и ближних/дальних плоскостей отсечения (обычно получается с камеры).
    NOTE: Обычно используется для отсечения усеченной пирамиды.
    @param position Указатель на используемую позицию.
    @param forward Указатель на используемый прямой вектор.
    @param right Указатель на используемый правый вектор.
    @param up Указатель на используемый верхний вектор.
    @param aspects Соотношение сторон.
    @param fov Вертикальное поле зрения.
    @param near Расстояние до ближней плоскости отсечения.
    @param far Расстояние до дальней плоскости отсечения.
    @return Усеченная пирамида.
*/
KAPI frustum frustum_create(
    const vec3 *position, const vec3 *forward, const vec3 *right, const vec3 *up, f32 aspect, f32 fov,
    f32 near, f32 far
);

KAPI frustum frustum_from_view_projection(mat4 view_projection);

/*
    @brief Вычисляет угловые точки предоставленного усеченного конуса в мировом пространстве,
           используя заданные матрицы проекции и вида.
    @param projection_view Объединенная матрица проекции/вида из активной камеры.
    @param corners Указатель на vec4 массив из 8 точек.
*/
KAPI void frustum_corner_points_world_space(mat4 projection_view, vec4 *corners);

/*
    @brief Получает знаковое расстояние между плоскостью и предоставленной позицией.
    @param p Указатель на плоскость.
    @param position Указатель на позицию.
    @return Знаковое расстояние от точки до плоскости.
*/
KAPI f32 plane_signed_distance(const plane_3d *p, const vec3 *position);

/*
    @brief Указывает, пересекает ли плоскость сферу, построенную через центр и радиус.
    @param p Указатель на плоскость.
    @param center Указатель на позицию, представляющую центр сферы.
    @param radius Радиус сферы.
    @return True если сфера пересекает плоскость, false в остальных случаях.
*/
KAPI bool plane_intersects_sphere(const plane_3d *p, const vec3 *center, f32 radius);

/*
    @brief Указывает, пересекает ли усеченная пирамида сферу, построенную через центр и радиус, (или содержит ее).
    @param f Указатель на усеченную пирамиду.
    @param center Указатель на позицию, представляющую центр сферы.
    @param radius Радиус сферы.
    @return True если сфера пересекается или содержится в усеченной пирамиде, false в остальных случаях.
*/
KAPI bool frustum_intersects_sphere(const frustum *f, const vec3 *center, f32 radius);

/*
    @brief Указывает, пересекает ли плоскость выровненный по оси ограничивающий прямоугольник,
           созданный с помощью center и extends.
    @param p Указатель на плоскость.
    @param center Указатель на позицию, представляющую центр выровненного по оси
           ограничивающего прямоугольника.
    @param extends Половинные размеры выровненного по оси ограничивающего прямоугольника.
    @return True если выровненный по оси ограничивающий прямоугольник пересекает плоскость,
            false в остальных случаях.
*/
KAPI bool plane_intersects_aabb(const plane_3d *p, const vec3 *center, const vec3 *extents);

/*
    @brief Указывает, пересекает ли frustum выровненный по оси ограничивающий прямоугольник,
           созданный с помощью center и extends.
    @param f Указатель на frustum.
    @param center Указатель на позицию, представляющую центр выровненного по оси
           ограничивающего прямоугольника.
    @param extends Половинные размеры выровненного по оси ограничивающего прямоугольника.
    @return True если выровненный по оси ограничивающий прямоугольник пересекается frustum или содержится в нем,
            false в остальных случаях.
*/
KAPI bool frustum_intersects_aabb(const frustum *f, const vec3 *center, const vec3 *extents);

KINLINE bool rect_2d_contains_point(rect_2d rect, vec2 point)
{
    return (point.x >= rect.x && point.x <= rect.x + rect.width)
        && (point.y >= rect.y && point.y <= rect.y + rect.height);
}

KINLINE vec3 extents_2d_half(extents_2d extents)
{
    return (vec3){{
        (extents.min.x + extents.max.x) * 0.5f,
        (extents.min.y + extents.max.y) * 0.5f,
    }};
}

KINLINE vec3 extents_3d_half(extents_3d extents)
{
    return (vec3){{
        (extents.min.x + extents.max.x) * 0.5f,
        (extents.min.y + extents.max.y) * 0.5f,
        (extents.min.z + extents.max.z) * 0.5f,
    }};
}
