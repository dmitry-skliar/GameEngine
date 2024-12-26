#include <logger.h>
#include <debug/assert.h>
#include <platform/window.h>

static bool should_close = false;

void on_key(u32 keycode, bool pressed)
{
    KTRACE(pressed ? "Key press: %d" : "Key release: %d", keycode);
    if(keycode == 16) should_close = true;
}

void on_close()
{
    should_close = true;
}

int main()
{
    // Тест отключения логов.
    log_output_hook_set(null);
    KERROR("This message is skipped.");
    log_output_hook_set_default();

    // Тест цветных сообщений.
    KTRACE("Test message: %.3f.", 3.1415f);
    KDEBUG("Test message: %.3f.", 3.1415f);
    KINFOR("Test message: %.3f.", 3.1415f);
    KWARNG("Test message: %.3f.", 3.1415f);
    KERROR("Test message: %.3f.", 3.1415f);
    // KFATAL("Test message: %.3f.", 3.1415f);
    // KASSERT_MSG(1==0,"Test message!");

    // Инициализация.
    window_config config = { "Test window", 0, 0, 1920, 1080 };
    platform_window_create(config);
    platform_window_handler_close_set(on_close);
    platform_window_handler_keyboard_key_set(on_key);

    // Цикл.
    while(!should_close)
    {
        platform_window_dispatch();
        // Другия действия.
    }

    // Завершение.
    platform_window_destroy();

    return 0;
}
