// Cобственные подключения.
#include "platform/window.h"

#if KPLATFORM_LINUX_WAYLAND_FLAG

    // Внутренние подключения.
    #include "logger.h"
    #include "debug/assert.h"
    #include "memory/memory.h"
    #include "input_types.h"
    #include "containers/darray.h"
    #include "window_wayland_xdg.h"
    #include "renderer/vulkan/vulkan_platform.h"

    // Внешние подключения.
    #include <wayland-client.h>
    #include <vulkan/vulkan.h>
    #include <vulkan/vulkan_wayland.h>
    #include <string.h>
    // TODO: Временно начало.
    #include <sys/mman.h>
    #include <fcntl.h>
    #include <unistd.h>
    // TODO: Временно конец.

    typedef struct platform_window_context {
        // Для работы с окном приложения.
        struct wl_display*    wdisplay;
        struct wl_registry*   wregistry;
        struct wl_compositor* wcompositor;
        struct wl_surface*    wsurface;
        // TODO: Временно начало.
        struct wl_shm*        wshm;
        struct wl_buffer*     wbuffer;
        // TODO: Временно конец.
        struct xdg_wm_base*   xbase;
        struct xdg_surface*   xsurface;
        struct xdg_toplevel*  xtoplevel;
        // Для работы с устройствами ввода.
        struct wl_seat*       wseat;
        struct wl_keyboard*   wkeyboard;
        struct wl_pointer*    wpointer;
        // Обработчики событий окна.
        PFN_window_handler_close        on_close;
        PFN_window_handler_resize       on_resize;
        PFN_window_handler_keyboard_key on_keyboard_key;
        PFN_window_handler_mouse_move   on_mouse_move;
        PFN_window_handler_mouse_button on_mouse_button;
        PFN_window_handler_mouse_wheel  on_mouse_wheel;
        PFN_window_handler_focus        on_focus;
        // Флаги состояний.
        bool  do_resize;
        // Кеширование значений.
        char* title;
        i32   width;
        i32   height;
    } platform_window_context;

    // Указатель на структура контекста окна.
    static platform_window_context* context = null;

    // Сообщения.
    static const char* message_context_not_created = "Window context was not created. Please first call 'platform_window_create'.";
    static const char* message_event_not_set       = "Wayland event '%s' not set.";

    // TODO: Временно. Вынести в отдельную API функцию!
    static void screen_clear(i32 width, i32 heigth, u32 color)
    {
        i32 stride  = width * 4;
        i32 size    = stride * heigth;
        char name[] = "/platform_window_linux_wayland_shm_xxxxxx";

        shm_unlink(name);
        i32 fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0777);
        kassert(fd >= 0, "Unable to open shared memory object.");

        i32 result = ftruncate(fd, size);;
        kassert(result >= 0, "Error truncating shared memory object.");

        void* mem = mmap(null, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        kassert(mem != MAP_FAILED, "Memory map failed.");
        shm_unlink(name);

        // Рисуем в буфер.
        for(i32 i = 0; i < size; i += 4)
        {
            *((u32*)(mem + i)) = color;
        }

        struct wl_shm_pool* pool = wl_shm_create_pool(context->wshm, fd, size);
        context->wbuffer = wl_shm_pool_create_buffer(pool, 0, width, heigth, stride, WL_SHM_FORMAT_ARGB8888);
        wl_shm_pool_destroy(pool);

        // Так как wl_buffer и файл(fd) сопоставленны, то можно закрыть дескриптор.
        close(fd);

        // Выполняем захват кадра и подтверждение изменений.
        wl_surface_attach(context->wsurface, context->wbuffer, 0, 0);
        wl_surface_commit(context->wsurface);
    }

    // Объявления функций (обработчичи событий).
    static void wregistry_add(void* data, struct wl_registry* wregistry, u32 name, const char* interface, u32 version);
    static void wregistry_remove(void* data, struct wl_registry* wregistry, u32 name);
    static void xbase_ping(void* data, struct xdg_wm_base *xbase, u32 serial);
    static void xsurface_configure(void* data, struct xdg_surface* xsurface, u32 serial);
    static void xtoplevel_configure(void* data, struct xdg_toplevel* xtoplevel, i32 width, i32 height, struct wl_array* states);
    static void xtoplevel_close(void* data, struct xdg_toplevel* xtoplevel);
    static void xtoplevel_configure_bounds(void* data, struct xdg_toplevel* xtoplevel, i32 width, i32 height);
    static void wtoplevel_wm_capabilities(void* data, struct xdg_toplevel* xtoplevel, struct wl_array* capabilities);
    static void wseat_capabilities(void* data, struct wl_seat* wseat, u32 capabilities);
    static void wseat_name(void* data, struct wl_seat* wseat, const char* name);
    static void kb_keymap(void* data, struct wl_keyboard* wkeyboard, u32 format, i32 fd, u32 size);
    static void kb_enter(void* data, struct wl_keyboard* wkeyboard, u32 serial, struct wl_surface* wsurface, struct wl_array* keys);
    static void kb_leave(void* data, struct wl_keyboard* wkeyboard, u32 serial, struct wl_surface* wsurface);
    static void kb_key(void* data, struct wl_keyboard* wkeyboard, u32 serial, u32 time, u32 key, u32 state);
    static void kb_mods(void* data, struct wl_keyboard* wkeyboard, u32 serial, u32 depressed, u32 latched, u32 locked, u32 group);
    static void kb_repeat(void* data, struct wl_keyboard* wkeyboard, i32 rate, i32 delay);
    static void pt_enter(void* data, struct wl_pointer* wpointer, u32 serial, struct wl_surface* wsurface, wl_fixed_t x, wl_fixed_t y);
    static void pt_leave(void* data, struct wl_pointer* wpointer, u32 serial, struct wl_surface* wsurface);
    static void pt_motion(void* data, struct wl_pointer* wpointer, u32 time, wl_fixed_t x, wl_fixed_t y);
    static void pt_button(void* data, struct wl_pointer* wpointer, u32 serial, u32 time, u32 button, u32 state);
    static void pt_axis(void* data, struct wl_pointer* wpointer, u32 time, u32 axis, wl_fixed_t value);
    static void pt_frame(void* data, struct wl_pointer* wpointer);
    static void pt_axis_source(void* data, struct wl_pointer* wpointer, u32 axis_source);
    static void pt_axis_stop(void* data, struct wl_pointer* wpointer, u32 time, u32 axis);
    static void pt_axis_discrete(void* data, struct wl_pointer* wpointer, u32 axis, i32 discrete);
    static void pt_axis_value120(void* data, struct wl_pointer* wpointer, u32 axis, i32 value120);
    static void pt_axis_relative_direction(void* data, struct wl_pointer* wpointer, u32 axis, u32 direction);

    // Структуры обработчиков событий (слушателей).
    static const struct wl_registry_listener  wregistry_listeners = { wregistry_add, wregistry_remove };
    static const struct xdg_wm_base_listener  xbase_listeners     = { xbase_ping };
    static const struct xdg_surface_listener  xsurface_listeners  = { xsurface_configure };
    static const struct xdg_toplevel_listener xtoplevel_listeners = { xtoplevel_configure, xtoplevel_close, xtoplevel_configure_bounds, wtoplevel_wm_capabilities };
    static const struct wl_seat_listener      wseat_listeners     = { wseat_capabilities, wseat_name };
    static const struct wl_keyboard_listener  keyboard_listeners  = { kb_keymap, kb_enter, kb_leave, kb_key, kb_mods, kb_repeat };
    static const struct wl_pointer_listener   pointer_listeners   = {
        pt_enter, pt_leave, pt_motion, pt_button, pt_axis, pt_frame, pt_axis_source, pt_axis_stop, pt_axis_discrete,
        pt_axis_value120, pt_axis_relative_direction
    };

    bool platform_window_create(window_config* config)
    {
        // Проверка вызова функции.
        kassert_debug(context == null, "Trying to call function 'platform_window_create' more than once!");
        kassert_debug(config != null, "Function 'platform_window_create' requires configuration!");

        context = kmallocate_t(platform_window_context, MEMORY_TAG_APPLICATION);
        if(!context)
        {
            kerror("Memory for window context was not allocated!");
            return false;
        }
        kmzero_tc(context, platform_window_context, 1);

        // Инициализация конфигурации окна платформы.
        context->height = config->height;
        context->width  = config->width;
        // TODO: Добавить context->title!

        // Инициализация WAYLAND клиента.
        context->wdisplay = wl_display_connect(null);
        if(!context->wdisplay)
        {
            kerror("Failed to connect to Wayland display!");
            return false;
        }

        context->wregistry = wl_display_get_registry(context->wdisplay);
        wl_registry_add_listener(context->wregistry, &wregistry_listeners, null);
        wl_display_roundtrip(context->wdisplay);

        // Инициализация поверхности окна.
        context->wsurface = wl_compositor_create_surface(context->wcompositor);
        context->xsurface = xdg_wm_base_get_xdg_surface(context->xbase, context->wsurface);
        xdg_surface_add_listener(context->xsurface, &xsurface_listeners, null);

        context->xtoplevel = xdg_surface_get_toplevel(context->xsurface);
        xdg_toplevel_add_listener(context->xtoplevel, &xtoplevel_listeners, null);

        // TODO: Заменить на context->title!
        xdg_toplevel_set_title(context->xtoplevel, config->title);
        xdg_toplevel_set_app_id(context->xtoplevel, config->title);

        // NOTE: Для полноэкранного режима по умолчанию, раскомментируете ниже.
        // xdg_toplevel_set_fullscreen(context->xtoplevel, null);

        // NOTE: Первая настройка поверхности, а потому до нее захват буфера работать не будет!
        wl_surface_commit(context->wsurface);
        wl_display_roundtrip(context->wdisplay);

        // TODO: Временно начало. Вынести в отдельную API функцию!
        // screen_clear(context->width, context->height, 0x77101010);
        // TODO: Временно конец.

        kinfor("Platform window started.");

        return true;
    }

    void platform_window_destroy()
    {
        // Проверка вызова функции.
        kassert_debug(context != null, message_context_not_created);

        if(context->wpointer)    { wl_pointer_destroy(context->wpointer); context->wpointer = null;          }
        if(context->wkeyboard)   { wl_keyboard_destroy(context->wkeyboard); context->wkeyboard = null;       }
        if(context->wseat)       { wl_seat_destroy(context->wseat); context->wseat = null;                   }
        if(context->xtoplevel)   { xdg_toplevel_destroy(context->xtoplevel); context->xtoplevel = null;      }
        if(context->xsurface)    { xdg_surface_destroy(context->xsurface); context->xsurface = null;         }
        if(context->xbase)       { xdg_wm_base_destroy(context->xbase); context->xbase = null;               }
        if(context->wsurface)    { wl_surface_destroy(context->wsurface); context->wsurface = null;          }
        if(context->wcompositor) { wl_compositor_destroy(context->wcompositor); context->wcompositor = null; }
        // TODO: Временно начало.
        if(context->wbuffer)     { wl_buffer_destroy(context->wbuffer); context->wbuffer = null;             }
        if(context->wshm)        { wl_shm_destroy(context->wshm); context->wshm = null;                      }
        // TODO: Временно конец.
        if(context->wregistry)   { wl_registry_destroy(context->wregistry); context->wregistry = null;       }
        if(context->wdisplay)    { wl_display_disconnect(context->wdisplay); context->wdisplay = null;       }

        kmfree(context);
        context = null;

        kinfor("Platform window stopped.");
    }

    bool platform_window_dispatch()
    {
        // Проверка вызова функции.
        kassert_debug(context != null, message_context_not_created);

        return wl_display_roundtrip(context->wdisplay) != -1;
    }

    void wregistry_add(void* data, struct wl_registry* wregistry, u32 name, const char* interface, u32 version)
    {
        if(strcmp(interface, wl_compositor_interface.name) == 0)
        {
            context->wcompositor = wl_registry_bind(wregistry, name, &wl_compositor_interface, 1);
            if(!context->wcompositor) kfatal("Wayland interface 'compositor' is not supported!");
        }
        else if(strcmp(interface, xdg_wm_base_interface.name) == 0)
        {
            context->xbase = wl_registry_bind(wregistry, name, &xdg_wm_base_interface, 1);
            if(!context->xbase) kfatal("Wayland interface 'xdg-shell' is not supported!");
            xdg_wm_base_add_listener(context->xbase, &xbase_listeners, null);
        }
        else if(strcmp(interface, wl_seat_interface.name) == 0)
        {
            context->wseat = wl_registry_bind(wregistry, name, &wl_seat_interface, 1);
            if(!context->wseat) kfatal("Wayland interface 'seat' is not supported!");
            wl_seat_add_listener(context->wseat, &wseat_listeners, null);
        }
        else if(strcmp(interface, wl_shm_interface.name) == 0)
        {
            context->wshm = wl_registry_bind(wregistry, name, &wl_shm_interface, 1);
            if(!context->wshm) kfatal("Wayland interface 'shm' is not supported!");
        }
    }

    void wregistry_remove(void* data, struct wl_registry* wregistry, u32 name)
    {
    }

    void xbase_ping(void* data, struct xdg_wm_base *xbase, u32 serial)
    {
        xdg_wm_base_pong(xbase, serial); // Проверка на доступность окна.
    }

    void xsurface_configure(void* data, struct xdg_surface* xsurface, u32 serial)
    {
        xdg_surface_ack_configure(xsurface, serial);

        if(context->do_resize)
        {
            if(context->on_resize)
            {
                context->on_resize(context->width, context->height);
            }
            else
            {
                ktrace(message_event_not_set, "on_resize");
            }

            // FIX: Этим достигается плавность изменения размера.
            wl_surface_commit(context->wsurface);

            context->do_resize = false;
        }
    }

    void xtoplevel_configure(void* data, struct xdg_toplevel* xtoplevel, i32 width, i32 height, struct wl_array* states)
    {
        if(width != 0 && height != 0 && (context->width != width || context->height != height))
        {
            context->do_resize = true;
            context->width     = width;
            context->height    = height;
        }
    }

    void xtoplevel_close(void* data, struct xdg_toplevel* xtoplevel)
    {
        if(context->on_close)
        {
            context->on_close();
        }
        else
        {
            ktrace(message_event_not_set, "on_close");
        }
    }

    void xtoplevel_configure_bounds(void* data, struct xdg_toplevel* xtoplevel, i32 width, i32 height)
    {
    }

    void wtoplevel_wm_capabilities(void* data, struct xdg_toplevel* xtoplevel, struct wl_array* capabilities)
    {
    }

    void wseat_capabilities(void* data, struct wl_seat* wseat, u32 capabilities)
    {
        if(capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
        {
            if(context->wkeyboard)
            {
                wl_keyboard_release(context->wkeyboard);
                context->wkeyboard = null;
                ktrace("Wayland seat: the keyboard is lost.");
            }
            else
            {
                context->wkeyboard = wl_seat_get_keyboard(wseat);
                wl_keyboard_add_listener(context->wkeyboard, &keyboard_listeners, null);
                ktrace("Wayland seat: the keyboard is found.");
            }
        }

        if(capabilities & WL_SEAT_CAPABILITY_POINTER)
        {
            if(context->wpointer)
            {
                wl_pointer_release(context->wpointer);
                context->wpointer = null;
                ktrace("Wayland seat: the pointer is lost.");
            }
            else
            {
                context->wpointer = wl_seat_get_pointer(wseat);
                wl_pointer_add_listener(context->wpointer, &pointer_listeners, null);
                ktrace("Wayland seat: the pointer is found.");
            }
        }
    }

    void wseat_name(void* data, struct wl_seat* wseat, const char* name)
    {
    }

    u32 kb_translate_keycode(u32 key_code)
    {
        // NOTE: Таблица трансляции linux keycode -> virtual keycode.
        //       Смотреть linux/input-event-codes.h
        static const key codes[KEYS_MAX] = {
            [0x01] = KEY_ESCAPE,       [0x02] = KEY_1,            [0x03] = KEY_2,            [0x04] = KEY_3,
            [0x05] = KEY_4,            [0x06] = KEY_5,            [0x07] = KEY_6,            [0x08] = KEY_7,
            [0x09] = KEY_8,            [0x0A] = KEY_9,            [0x0B] = KEY_0,            [0x0C] = KEY_MINUS,
            [0x0D] = KEY_EQUAL,        [0x0E] = KEY_BACKSPACE,    [0x0F] = KEY_TAB,          [0x10] = KEY_Q,
            [0x11] = KEY_W,            [0x12] = KEY_E,            [0x13] = KEY_R,            [0x14] = KEY_T,
            [0x15] = KEY_Y,            [0x16] = KEY_U,            [0x17] = KEY_I,            [0x18] = KEY_O,
            [0x19] = KEY_P,            [0x1A] = KEY_LBRACKET,     [0x1B] = KEY_RBRACKET,     [0x1C] = KEY_ENTER,
            [0x1D] = KEY_LCONTROL,     [0x1E] = KEY_A,            [0x1F] = KEY_S,            [0x20] = KEY_D,
            [0x21] = KEY_F,            [0x22] = KEY_G,            [0x23] = KEY_H,            [0x24] = KEY_J,
            [0x25] = KEY_K,            [0x26] = KEY_L,            [0x27] = KEY_SEMICOLON,    [0x28] = KEY_APOSTROPHE,
            [0x29] = KEY_GRAVE,        [0x2A] = KEY_LSHIFT,       [0x2B] = KEY_BACKSLASH,    [0x2C] = KEY_Z,
            [0x2D] = KEY_X,            [0x2E] = KEY_C,            [0x2F] = KEY_V,            [0x30] = KEY_B,
            [0x31] = KEY_N,            [0x32] = KEY_M,            [0x33] = KEY_COMMA,        [0x34] = KEY_DOT,
            [0x35] = KEY_SLASH,        [0x36] = KEY_RSHIFT,       [0x37] = KEY_UNKNOWN,      [0x38] = KEY_LALT,
            [0x39] = KEY_SPACE,        [0x3A] = KEY_CAPSLOCK,     [0x3B] = KEY_F1,           [0x3C] = KEY_F2,
            [0x3D] = KEY_F3,           [0x3E] = KEY_F4,           [0x3F] = KEY_F5,           [0x40] = KEY_F6,
            [0x41] = KEY_F7,           [0x42] = KEY_F8,           [0x43] = KEY_F9,           [0x44] = KEY_F10,
            [0x45] = KEY_NUMLOCK,      [0x46] = KEY_SCROLLOCK,    [0x47] = KEY_NUMPAD7,      [0x48] = KEY_NUMPAD8,
            [0x49] = KEY_NUMPAD9,      [0x4A] = KEY_SUBTRACT,     [0x4B] = KEY_NUMPAD4,      [0x4C] = KEY_NUMPAD5,
            [0x4D] = KEY_NUMPAD6,      [0x4E] = KEY_ADD,          [0x4F] = KEY_NUMPAD1,      [0x50] = KEY_NUMPAD2,
            [0x51] = KEY_NUMPAD3,      [0x52] = KEY_NUMPAD0,      [0x53] = KEY_DECIMAL,      [0x57] = KEY_F11,
            [0x58] = KEY_F12,          [0x60] = KEY_UNKNOWN,      [0x61] = KEY_RCONTROL,     [0x62] = KEY_DIVIDE,
            [0x63] = KEY_PRINTSCREEN,  [0x64] = KEY_RALT,         [0x65] = KEY_UNKNOWN,      [0x66] = KEY_HOME,
            [0x67] = KEY_UP,           [0x68] = KEY_PAGEUP,       [0x69] = KEY_LEFT,         [0x6A] = KEY_RIGHT,
            [0x6B] = KEY_END,          [0x6C] = KEY_DOWN,         [0x6D] = KEY_PAGEDOWN,     [0x6E] = KEY_INSERT,
            [0x6F] = KEY_DELETE,       [0x75] = KEY_NUMPAD_EQUAL, [0x76] = KEY_UNKNOWN,      [0x77] = KEY_PAUSE,
            [0x7D] = KEY_LSUPER,       [0x7E] = KEY_RSUPER,       [0x7F] = KEY_APPS,      
            //...
            [0xB7] = KEY_F13,          [0xB8] = KEY_F14,          [0xB9] = KEY_F15,          [0xBA] = KEY_F16,
            [0xBB] = KEY_F17,          [0xBC] = KEY_F18,          [0xBD] = KEY_F19,          [0xBE] = KEY_F20,
            [0xBF] = KEY_F21,          [0xC0] = KEY_F22,          [0xC1] = KEY_F23,          [0xC2] = KEY_F24,
            //...
            [0xD2] = KEY_PRINT
        };

        if(codes[key_code] == KEY_UNKNOWN || key_code > KEYS_MAX)
        {
            kwarng("In Linux, the key code %X is translated as KEY_UNKNOWN", key_code);
            return KEY_UNKNOWN;
        }

        return codes[key_code];
    }

    void kb_keymap(void* data, struct wl_keyboard* wkeyboard, u32 format, i32 fd, u32 size)
    {
    }

    void kb_enter(void* data, struct wl_keyboard* wkeyboard, u32 serial, struct wl_surface* wsurface, struct wl_array* keys)
    {
    }

    void kb_leave(void* data, struct wl_keyboard* wkeyboard, u32 serial, struct wl_surface* wsurface)
    {
    }

    void kb_key(void* data, struct wl_keyboard* wkeyboard, u32 serial, u32 time, u32 key, u32 state)
    {
        if(context->on_keyboard_key)
        {
            context->on_keyboard_key(
                kb_translate_keycode(key), state == WL_KEYBOARD_KEY_STATE_PRESSED ? true : false
            );
        }
        else
        {
            ktrace(message_event_not_set, "on_keyboard_key");
        }
    }

    void kb_mods(void* data, struct wl_keyboard* wkeyboard, u32 serial, u32 depressed, u32 latched, u32 locked, u32 group)
    {
    }

    void kb_repeat(void* data, struct wl_keyboard* wkeyboard, i32 rate, i32 delay)
    {
    }

    u32 pt_translate_button_code(u32 button_code)
    {
        #define BTNS_START 0x110
        #define BTNS_END   0x114

        // NOTE: Таблица трансляции linux button code -> virtual button code.
        //       Смотреть linux/input-event-codes.h
        static const u16 codes[] = {
            [0x00] = BTN_LEFT, [0x01] = BTN_RIGHT, [0x02] = BTN_MIDDLE, [0x03] = BTN_BACKWARD, [0x04] = BTN_FORWARD
        };

        if(button_code < BTNS_START || button_code > BTNS_END)
        {
            kwarng("In Linux, the key code %X is translated as KEY_UNKNOWN", button_code);
        }

        return codes[button_code - BTNS_START];
    }

    void pt_enter(void* data, struct wl_pointer* wpointer, u32 serial, struct wl_surface* wsurface, wl_fixed_t x, wl_fixed_t y)
    {
        if(context->on_focus)
        {
            context->on_focus(true);
        }
        else
        {
            ktrace(message_event_not_set, "on_focus");
        }
    }

    void pt_leave(void* data, struct wl_pointer* wpointer, u32 serial, struct wl_surface* wsurface)
    {
        if(context->on_focus)
        {
            context->on_focus(false);
        }
        else
        {
            ktrace(message_event_not_set, "on_focus");
        }
    }

    void pt_motion(void* data, struct wl_pointer* wpointer, u32 time, wl_fixed_t x, wl_fixed_t y)
    {
        // Преобразование координат.
        x = wl_fixed_to_int(x);
        y = wl_fixed_to_int(y);

        if(context->on_mouse_move)
        {
            context->on_mouse_move(x, y);
        }
        else
        {
            ktrace(message_event_not_set, "on_mouse_move");
        }
    }

    void pt_button(void* data, struct wl_pointer* wpointer, u32 serial, u32 time, u32 button, u32 state)
    {
        if(context->on_mouse_button)
        {
            context->on_mouse_button(
                pt_translate_button_code(button), state == WL_POINTER_BUTTON_STATE_PRESSED ? true : false
            );
        }
        else
        {
            ktrace(message_event_not_set, "on_mouse_button");
        }
    }

    void pt_axis(void* data, struct wl_pointer* wpointer, u32 time, u32 axis, wl_fixed_t value)
    {
        // Преобразование значения.
        value = wl_fixed_to_int(value);

        if(context->on_mouse_wheel)
        {
            context->on_mouse_wheel(value);
        }
        else
        {
            ktrace(message_event_not_set, "on_mouse_wheel");
        }
    }

    void pt_frame(void* data, struct wl_pointer* wpointer)
    {
    }

    void pt_axis_source(void* data, struct wl_pointer* wpointer, u32 axis_source)
    {
    }

    void pt_axis_stop(void* data, struct wl_pointer* wpointer, u32 time, u32 axis)
    {
    }

    void pt_axis_discrete(void* data, struct wl_pointer* wpointer, u32 axis, i32 discrete)
    {
    }

    void pt_axis_value120(void* data, struct wl_pointer* wpointer, u32 axis, i32 value120)
    {
    }

    void pt_axis_relative_direction(void* data, struct wl_pointer* wpointer, u32 axis, u32 direction)
    {
    }

    void platform_window_set_on_close_handler(PFN_window_handler_close handler)
    {
        // Проверка вызова функции.
        kassert_debug(context != null, message_context_not_created);
        context->on_close = handler;
    }

    void platform_window_set_on_resize_handler(PFN_window_handler_resize handler)
    {
        // Проверка вызова функции.
        kassert_debug(context != null, message_context_not_created);
        context->on_resize = handler;
    }

    void platform_window_set_on_keyboard_key_handler(PFN_window_handler_keyboard_key handler)
    {
        // Проверка вызова функции.
        kassert_debug(context != null, message_context_not_created);
        context->on_keyboard_key = handler;
    }

    void platform_window_set_on_mouse_move_handler(PFN_window_handler_mouse_move handler)
    {
        // Проверка вызова функции.
        kassert_debug(context != null, message_context_not_created);
        context->on_mouse_move = handler;
    }

    void platform_window_set_on_mouse_button_handler(PFN_window_handler_mouse_button handler)
    {
        // Проверка вызова функции.
        kassert_debug(context != null, message_context_not_created);
        context->on_mouse_button = handler;
    }

    void platform_window_set_on_mouse_wheel_handler(PFN_window_handler_mouse_wheel handler)
    {
        // Проверка вызова функции.
        kassert_debug(context != null, message_context_not_created);
        context->on_mouse_wheel = handler;
    }

    void platform_window_set_on_focus_handler(PFN_window_handler_focus handler)
    {
        // Проверка вызова функции.
        kassert_debug(context != null, message_context_not_created);
        context->on_focus = handler;
    }

    void platform_window_get_dimentions(u32* width, u32* height)
    {
        *width = context->width;
        *height = context->height;
    }

    void platform_window_get_vulkan_extentions(const char*** names)
    {
        darray_push(*names, &VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
    }

    VkResult platform_window_create_vulkan_surface(vulkan_context* vcontext)
    {
        VkWaylandSurfaceCreateInfoKHR surfinfo = { VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR };
        surfinfo.display = context->wdisplay;
        surfinfo.surface = context->wsurface;
        return vkCreateWaylandSurfaceKHR(vcontext->instance, &surfinfo, vcontext->allocator, &vcontext->surface);
    }

    void platform_window_destroy_vulkan_surface(vulkan_context* vcontext)
    {
        vkDestroySurfaceKHR(vcontext->instance, vcontext->surface, vcontext->allocator);
        vcontext->surface = null;
    }

    bool platform_window_get_vulkan_presentation_support(vulkan_context* vcontext, VkPhysicalDevice physical_device, u32 queue_family_index)
    {
        return (bool)vkGetPhysicalDeviceWaylandPresentationSupportKHR(physical_device, queue_family_index, context->wdisplay);
    }

#endif
