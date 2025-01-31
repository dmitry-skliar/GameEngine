// Собственные подключения.
#include "platform/math.h"
#include "platform/time.h"

#if KPLATFORM_LINUX_FLAG

    // Внутренние подключения.

    // Внешние подключения.
    #include <math.h>
    #include <stdlib.h>

    // Указание что зерно для случайного числа сгенерировано.
    static bool rand_seeded = false;

    f32 platform_math_sin(f32 x)
    {
        return sinf(x);
    }

    f32 platform_math_cos(f32 x)
    {
        return cosf(x);
    }

    f32 platform_math_tan(f32 x)
    {
        return tanf(x);
    }

    f32 platform_math_atan(f32 x)
    {
        return atanf(x);
    }

    f32 platform_math_acos(f32 x)
    {
        return acosf(x);
    }

    f32 platform_math_sqrt(f32 x)
    {
        return sqrtf(x);
    }

    f32 platform_math_abs(f32 x)
    {
        return fabsf(x);
    }

    f32 platform_math_floor(f32 x)
    {
        return floorf(x);
    }

    f32 platform_math_ceil(f32 x)
    {
        return ceilf(x);
    }

    f32 platform_math_log2(f32 x)
    {
        return log2f(x);
    }

    f32 platform_math_pow(f32 x, f32 p)
    {
        return powf(x, p);
    }

    i32 platform_math_random()
    {
        if(!rand_seeded)
        {
            srand((u32)platform_time_get_absolute());
            rand_seeded = true;
        }
        return rand();
    }

    i32 platform_math_random_in_range(i32 min, i32 max)
    {
        return (platform_math_random() % (max - min + 1)) + min;
    }

    f32 platform_math_frandom()
    {
        return (float)platform_math_random() / (f32)RAND_MAX;
    }

    f32 platform_math_frandom_in_range(f32 min, f32 max)
    {
        return ((float)platform_math_random() / ((f32)RAND_MAX / (max - min))) + min;
    }

#endif
