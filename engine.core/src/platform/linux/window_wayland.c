// Cобственные подключения.
#include "platform/window.h"
#include "platform/string.h"

#if KPLATFORM_LINUX_WAYLAND_FLAG

    // Внутренние подключения.
    #include "logger.h"
    #include "memory/memory.h"
    #include "input_types.h"
    #include "containers/darray.h"
    #include "window_wayland_xdg.h"
    #include "renderer/vulkan/vulkan_platform.h"

    // Внешние подключения.
    #include <wayland-client.h>
    #include <vulkan/vulkan.h>
    #include <vulkan/vulkan_wayland.h>

    typedef struct platform_window_state {
        // Для работы с окном приложения.
        struct wl_display*    wdisplay;
        struct wl_registry*   wregistry;
        struct wl_compositor* wcompositor;
        struct wl_surface*    wsurface;
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
    } platform_window_state;

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

    static const char* message_missing_instance = "Function '%s' requires an instance of window.";

    bool platform_window_create(u64* memory_requirement, window* instance, window_config* config)
    {
        // TODO: Защита от повтороного вызова для данного экземпляра!

        *memory_requirement = sizeof(struct window) + sizeof(struct platform_window_state);

        if(!instance)
        {
            return true;
        }

        kzero(instance, *memory_requirement);
        platform_window_state* state = (void*)((u8*)instance + sizeof(struct window));

        // Инициализация окна.
        instance->width = config->width;
        instance->height = config->height;
        instance->title = config->title;

        // Инициализация WAYLAND клиента.
        if(!(state->wdisplay = wl_display_connect(null)))
        {
            kerror("Function '%s': Failed to connect to wayland display. Return false!", __FUNCTION__);
            return false;
        }

        state->wregistry = wl_display_get_registry(state->wdisplay);
        wl_registry_add_listener(state->wregistry, &wregistry_listeners, instance);
        kdebug("Supported Wayland interfaces:");
        wl_display_roundtrip(state->wdisplay);

        // Проверка поддержки интерфейсов.
        if(!state->wcompositor)
        {
            kerror("Function '%s': Failed to bind 'wl_compositor' interface.", __FUNCTION__);
            return false;
        }

        if(!state->xbase)
        {
            kerror("Function '%s': Failed to bind 'xdg_wm_base' interface.", __FUNCTION__);
            return false;
        }

        if(!state->wseat)
        {
            kerror("Function '%s': Failed to bind 'seat' interface.", __FUNCTION__);
            return false;
        }

        // Инициализация поверхности окна.
        state->wsurface = wl_compositor_create_surface(state->wcompositor);
        state->xsurface = xdg_wm_base_get_xdg_surface(state->xbase, state->wsurface);
        xdg_surface_add_listener(state->xsurface, &xsurface_listeners, instance);

        state->xtoplevel = xdg_surface_get_toplevel(state->xsurface);
        xdg_toplevel_add_listener(state->xtoplevel, &xtoplevel_listeners, instance);

        xdg_toplevel_set_title(state->xtoplevel, instance->title);
        xdg_toplevel_set_app_id(state->xtoplevel, instance->title);

        // NOTE: Для полноэкранного режима по умолчанию, раскомментируете ниже.
        // xdg_toplevel_set_fullscreen(context->xtoplevel, null);
        // xdg_toplevel_set_maximized();

        // NOTE: Первая настройка поверхности, а потому до нее захват буфера работать не будет!
        wl_surface_commit(state->wsurface);
        wl_display_roundtrip(state->wdisplay);

        return true;
    }

    void platform_window_destroy(window* instance)
    {
        if(!instance)
        {
            kerror(message_missing_instance, __FUNCTION__);
            return;
        }

        platform_window_state* state = (void*)((u8*)instance + sizeof(struct window));
        wl_pointer_destroy(state->wpointer);
        wl_keyboard_destroy(state->wkeyboard);
        wl_seat_destroy(state->wseat);
        xdg_toplevel_destroy(state->xtoplevel);
        xdg_surface_destroy(state->xsurface);
        xdg_wm_base_destroy(state->xbase);
        wl_surface_destroy(state->wsurface);
        wl_compositor_destroy(state->wcompositor);
        wl_registry_destroy(state->wregistry);
        wl_display_disconnect(state->wdisplay);
    }

    bool platform_window_dispatch(window* instance)
    {
        if(!instance)
        {
            kerror("Function '%s' requires an instance of window. Return false!", __FUNCTION__);
            return false;
        }

        platform_window_state* state = (void*)((u8*)instance + sizeof(struct window));
        return wl_display_roundtrip(state->wdisplay) != INVALID_ID;
    }

    void platform_window_set_on_close_handler(window* instance, PFN_window_handler_close handler)
    {
        if(!instance)
        {
            kwarng(message_missing_instance, __FUNCTION__);
            return;
        }
        platform_window_state* state = (void*)((u8*)instance + sizeof(struct window));
        state->on_close = handler;
    }

    void platform_window_set_on_resize_handler(window* instance, PFN_window_handler_resize handler)
    {
        if(!instance)
        {
            kwarng(message_missing_instance, __FUNCTION__);
            return;
        }
        platform_window_state* state = (void*)((u8*)instance + sizeof(struct window));
        state->on_resize = handler;
    }

    void platform_window_set_on_keyboard_key_handler(window* instance, PFN_window_handler_keyboard_key handler)
    {
        if(!instance)
        {
            kwarng(message_missing_instance, __FUNCTION__);
            return;
        }

        platform_window_state* state = (void*)((u8*)instance + sizeof(struct window));
        state->on_keyboard_key = handler;
    }

    void platform_window_set_on_mouse_move_handler(window* instance, PFN_window_handler_mouse_move handler)
    {
        if(!instance)
        {
            kwarng(message_missing_instance, __FUNCTION__);
            return;
        }
        platform_window_state* state = (void*)((u8*)instance + sizeof(struct window));
        state->on_mouse_move = handler;
    }

    void platform_window_set_on_mouse_button_handler(window* instance, PFN_window_handler_mouse_button handler)
    {
        if(!instance)
        {
            kwarng(message_missing_instance, __FUNCTION__);
            return;
        }
        platform_window_state* state = (void*)((u8*)instance + sizeof(struct window));
        state->on_mouse_button = handler;
    }

    void platform_window_set_on_mouse_wheel_handler(window* instance, PFN_window_handler_mouse_wheel handler)
    {
        if(!instance)
        {
            kwarng(message_missing_instance, __FUNCTION__);
            return;
        }
        platform_window_state* state = (void*)((u8*)instance + sizeof(struct window));
        state->on_mouse_wheel = handler;
    }

    void platform_window_set_on_focus_handler(window* instance, PFN_window_handler_focus handler)
    {
        if(!instance)
        {
            kwarng(message_missing_instance, __FUNCTION__);
            return;
        }
        platform_window_state* state = (void*)((u8*)instance + sizeof(struct window));
        state->on_focus = handler;
    }

    void wregistry_add(void* data, struct wl_registry* wregistry, u32 name, const char* interface, u32 version)
    {
        platform_window_state* state = (void*)((u8*)data + sizeof(struct window));
        
        if(platform_string_equal(interface, wl_compositor_interface.name))
        {
            state->wcompositor = wl_registry_bind(wregistry, name, &wl_compositor_interface, 1);
        }
        else if(platform_string_equal(interface, xdg_wm_base_interface.name))
        {
            if((state->xbase = wl_registry_bind(wregistry, name, &xdg_wm_base_interface, 1)))
            {
                xdg_wm_base_add_listener(state->xbase, &xbase_listeners, data);
            }
        }
        else if(platform_string_equal(interface, wl_seat_interface.name))
        {
            if((state->wseat = wl_registry_bind(wregistry, name, &wl_seat_interface, 1)))
            {
                wl_seat_add_listener(state->wseat, &wseat_listeners, data);
            }
        }

        kdebug("[%2d] %s (version %d)", name, interface, version);
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
        window* instance = data;
        platform_window_state* state = (void*)((u8*)data + sizeof(struct window));

        xdg_surface_ack_configure(xsurface, serial);

        if(state->do_resize)
        {
            if(state->on_resize)
            {
                state->on_resize(instance->width, instance->height);
            }

            // FIX: Этим достигается плавность изменения размера.
            wl_surface_commit(state->wsurface);
            state->do_resize = false;
        }
    }

    void xtoplevel_configure(void* data, struct xdg_toplevel* xtoplevel, i32 width, i32 height, struct wl_array* states)
    {
        window* instance = data;
        platform_window_state* state = (void*)((u8*)data + sizeof(struct window));

        if(width && height && (instance->width != width || instance->height != height))
        {
            state->do_resize = true;
            instance->width  = width;
            instance->height = height;
        }
    }

    void xtoplevel_close(void* data, struct xdg_toplevel* xtoplevel)
    {
        platform_window_state* state = (void*)((u8*)data + sizeof(struct window));

        if(state->on_close)
        {
            state->on_close();
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
        platform_window_state* state = (void*)((u8*)data + sizeof(struct window));

        if(capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
        {
            if(state->wkeyboard)
            {
                wl_keyboard_release(state->wkeyboard);
                state->wkeyboard = null;
                kwarng("Wayland seat: the keyboard is lost.");
            }
            else
            {
                state->wkeyboard = wl_seat_get_keyboard(wseat);
                wl_keyboard_add_listener(state->wkeyboard, &keyboard_listeners, data);
                ktrace("Wayland seat: the keyboard is found.");
            }
        }

        if(capabilities & WL_SEAT_CAPABILITY_POINTER)
        {
            if(state->wpointer)
            {
                wl_pointer_release(state->wpointer);
                state->wpointer = null;
                kwarng("Wayland seat: the pointer is lost.");
            }
            else
            {
                state->wpointer = wl_seat_get_pointer(wseat);
                wl_pointer_add_listener(state->wpointer, &pointer_listeners, data);
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

        if(codes[key_code] == KEY_UNKNOWN || key_code >= KEYS_MAX)
        {
            kwarng("In Linux, the key code %X is translated as KEY_UNKNOWN.", key_code);
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
        platform_window_state* wstate = (void*)((u8*)data + sizeof(struct window));

        if(wstate->on_keyboard_key)
        {
            bool pressed = state == WL_KEYBOARD_KEY_STATE_PRESSED ? true : false;
            wstate->on_keyboard_key(kb_translate_keycode(key), pressed);
        }
    }

    void kb_mods(void* data, struct wl_keyboard* wkeyboard, u32 serial, u32 depressed, u32 latched, u32 locked, u32 group)
    {
    }

    void kb_repeat(void* data, struct wl_keyboard* wkeyboard, i32 rate, i32 delay)
    {
    }

    u32 pt_translate_btncode(u32 button_code)
    {
        #define BTNS_START 0x110
        #define BTNS_END   0x114

        // NOTE: Таблица трансляции linux button code -> virtual button code.
        //       Смотреть linux/input-event-codes.h
        static const u16 codes[] = {
            [0x00] = BTN_LEFT, [0x01] = BTN_RIGHT, [0x02] = BTN_MIDDLE, [0x03] = BTN_BACKWARD, [0x04] = BTN_FORWARD
        };

        if(button_code < BTNS_START || button_code >= BTNS_END)
        {
            kwarng("In Linux, the button code %X is translated as BTN_UNKNOWN.", button_code);
        }

        return codes[button_code - BTNS_START];
    }

    void pt_enter(void* data, struct wl_pointer* wpointer, u32 serial, struct wl_surface* wsurface, wl_fixed_t x, wl_fixed_t y)
    {
        platform_window_state* wstate = (void*)((u8*)data + sizeof(struct window));

        if(wstate->on_focus)
        {
            wstate->on_focus(true);
        }

        // TODO: Сокрытие курсора!
        // wl_pointer_set_cursor(context->wpointer, serial, null, 0, 0);
    }

    void pt_leave(void* data, struct wl_pointer* wpointer, u32 serial, struct wl_surface* wsurface)
    {
        platform_window_state* wstate = (void*)((u8*)data + sizeof(struct window));

        if(wstate->on_focus)
        {
            wstate->on_focus(false);
        }
    }

    void pt_motion(void* data, struct wl_pointer* wpointer, u32 time, wl_fixed_t x, wl_fixed_t y)
    {
        platform_window_state* wstate = (void*)((u8*)data + sizeof(struct window));
        
        // Преобразование координат.
        x = wl_fixed_to_int(x);
        y = wl_fixed_to_int(y);

        if(wstate->on_mouse_move)
        {
            wstate->on_mouse_move(x, y);
        }
    }

    void pt_button(void* data, struct wl_pointer* wpointer, u32 serial, u32 time, u32 button, u32 state)
    {
        platform_window_state* wstate = (void*)((u8*)data + sizeof(struct window));

        if(wstate->on_mouse_button)
        {
            bool pressed = state == WL_POINTER_BUTTON_STATE_PRESSED ? true : false;
            wstate->on_mouse_button(pt_translate_btncode(button), pressed);
        }
    }

    void pt_axis(void* data, struct wl_pointer* wpointer, u32 time, u32 axis, wl_fixed_t value)
    {
        platform_window_state* wstate = (void*)((u8*)data + sizeof(struct window));

        // Преобразование значения.
        value = wl_fixed_to_int(value);

        if(wstate->on_mouse_wheel)
        {
            wstate->on_mouse_wheel(value);
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

    void platform_window_get_vulkan_extentions(window* instance, const char*** names)
    {
        if(!instance)
        {
            kerror(message_missing_instance, __FUNCTION__);
            return;
        }

        darray_push(*names, &VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
    }

    VkResult platform_window_create_vulkan_surface(window* instance, vulkan_context* context)
    {
        if(!instance)
        {
            kerror("Function '%s' requires an instance of window. Return VK_ERROR_SURFACE_LOST_KHR!", __FUNCTION__);
            return VK_ERROR_SURFACE_LOST_KHR;
        }

        platform_window_state* state = (void*)((u8*)instance + sizeof(struct window));
        VkWaylandSurfaceCreateInfoKHR surfinfo = { VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR };
        surfinfo.display = state->wdisplay;
        surfinfo.surface = state->wsurface;
        return vkCreateWaylandSurfaceKHR(context->instance, &surfinfo, context->allocator, &context->surface);
    }

    void platform_window_destroy_vulkan_surface(window* instance, vulkan_context* context)
    {
        if(!instance)
        {
            kerror(message_missing_instance, __FUNCTION__);
            return;
        }

        vkDestroySurfaceKHR(context->instance, context->surface, context->allocator);
        context->surface = null;
    }

    bool platform_window_get_vulkan_presentation_support(window* instance, VkPhysicalDevice physical_device, u32 queue_family_index)
    {
        if(!instance)
        {
            kerror("Function '%s' requires an instance of window. Return false!", __FUNCTION__);
            return false;
        }

        platform_window_state* state = (void*)((u8*)instance + sizeof(struct window));
        return (bool)vkGetPhysicalDeviceWaylandPresentationSupportKHR(physical_device, queue_family_index, state->wdisplay);
    }

#endif
