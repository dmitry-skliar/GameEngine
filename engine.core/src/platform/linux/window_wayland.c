#include "platform/window.h"
#include "platform/memory.h"

#if KPLATFORM_LINUX_WAYLAND_FLAG

    #include <wayland-client.h>
    #include <string.h>

    #include "window_wayland_xdg.h"
    #include "debug/assert.h"
    #include "logger.h"

    typedef struct platform_window_context {
        // Для работы с окном приложения.
        struct wl_display*    wdisplay;
        struct wl_registry*   wregistry;
        struct wl_compositor* wcompositor;
        struct wl_surface*    wsurface;
        struct wl_shm*        wshm;
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
        bool  resized;
        // Кеширование значений.
        char* title;
        i32   width;
        i32   height;
    } platform_window_context;

    // Указатель на структура контекста окна.
    static platform_window_context* context = null;

    // Обработчичи событий.
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

    bool platform_window_create(window_config config)
    {
        // Исключает повторный вызов.
        if(context) return false;

        // TODO: Сделать обертку над platform_memory_* 
        context = platform_memory_allocate(sizeof(platform_window_context));
        KASSERT_MSG(context != null, "Memory was not allocated!");
        platform_memory_zero(context, sizeof(platform_window_context));

        // 1. Инициализация WAYLAND клиента.
        context->wdisplay = wl_display_connect(null);
        KASSERT_MSG(context->wdisplay != null, "Failed to connect to Wayland display!");

        context->wregistry = wl_display_get_registry(context->wdisplay);
        KASSERT_DEBUG(context->wregistry != null);

        wl_registry_add_listener(context->wregistry, &wregistry_listeners, null);
        wl_display_roundtrip(context->wdisplay);

        // 2. Инициализация поверхности окна.
        context->wsurface = wl_compositor_create_surface(context->wcompositor);
        KASSERT_DEBUG(context->wsurface != null);

        context->xsurface = xdg_wm_base_get_xdg_surface(context->xbase, context->wsurface);
        KASSERT_DEBUG(context->xsurface != null);
        xdg_surface_add_listener(context->xsurface, &xsurface_listeners, null);

        context->xtoplevel = xdg_surface_get_toplevel(context->xsurface);
        KASSERT_DEBUG(context->xtoplevel != null);
        xdg_toplevel_add_listener(context->xtoplevel, &xtoplevel_listeners, null);
        
        xdg_toplevel_set_title(context->xtoplevel, config.title);
        xdg_toplevel_set_app_id(context->xtoplevel, config.title);

        // INFO: For full screen mode, uncomment one item below.
        // xdg_toplevel_set_fullscreen(context->xtoplevel, null);

        wl_surface_commit(context->wsurface);
        wl_display_roundtrip(context->wdisplay);

        KTRACE("Platform window created...");

        return true;
    }

    void platform_window_destroy()
    {
        if(context)
        {
            if(context->wpointer)    { wl_pointer_destroy(context->wpointer); context->wpointer = null;          }
            if(context->wkeyboard)   { wl_keyboard_destroy(context->wkeyboard); context->wkeyboard = null;       }
            if(context->wseat)       { wl_seat_destroy(context->wseat); context->wseat = null;                   }
            if(context->xtoplevel)   { xdg_toplevel_destroy(context->xtoplevel); context->xtoplevel = null;      }
            if(context->xsurface)    { xdg_surface_destroy(context->xsurface); context->xsurface = null;         }
            if(context->xbase)       { xdg_wm_base_destroy(context->xbase); context->xbase = null;               }
            if(context->wsurface)    { wl_surface_destroy(context->wsurface); context->wsurface = null;          }
            if(context->wcompositor) { wl_compositor_destroy(context->wcompositor); context->wcompositor = null; }
            if(context->wregistry)   { wl_registry_destroy(context->wregistry); context->wregistry = null;       }
            if(context->wdisplay)    { wl_display_disconnect(context->wdisplay); context->wdisplay = null;       }

            platform_memory_free(context);
            context = null;

            KTRACE("Platform window destroyed...");
        }
    }

    bool platform_window_dispatch()
    {
        // return wl_display_roundtrip(context->wdisplay) != -1;
        return wl_display_dispatch(context->wdisplay) != -1;
    }

    void wregistry_add(void* data, struct wl_registry* wregistry, u32 name, const char* interface, u32 version)
    {
        if(strcmp(interface, wl_compositor_interface.name) == 0)
        {
            context->wcompositor = wl_registry_bind(wregistry, name, &wl_compositor_interface, 1);
            KASSERT_MSG(context->wcompositor != null, "The Wayland compositor or its version is not supported!");
        }
        else if(strcmp(interface, xdg_wm_base_interface.name) == 0)
        {
            context->xbase = wl_registry_bind(wregistry, name, &xdg_wm_base_interface, 1);
            KASSERT_MSG(context->xbase != null,"The Wayland xdg shell or its version is not supported!");
            xdg_wm_base_add_listener(context->xbase, &xbase_listeners, null);
        }
        else if(strcmp(interface, wl_seat_interface.name) == 0)
        {
            context->wseat = wl_registry_bind(wregistry, name, &wl_seat_interface, 1);
            KASSERT_MSG(context->wseat != null, "The Wayland seat or its version is not supported!");
            wl_seat_add_listener(context->wseat, &wseat_listeners, null);
        }
        else if(strcmp(interface, wl_shm_interface.name) == 0)
        {
            context->wshm = wl_registry_bind(wregistry, name, &wl_shm_interface, 1);
            KASSERT_MSG(context->wshm, "The Wayland shm or its version is not supported!");
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

        if(context->resized)
        {
            if(context->on_resize)
            {
                context->on_resize(context->width, context->height);
            }

            context->resized = false;
        }
    }

    void xtoplevel_configure(void* data, struct xdg_toplevel* xtoplevel, i32 width, i32 height, struct wl_array* states)
    {
        if(width != 0 && height != 0)
        {
            context->resized = true;
            context->width   = width;
            context->height  = height;
        }
    }

    void xtoplevel_close(void* data, struct xdg_toplevel* xtoplevel)
    {
        if(context->on_close)
        {
            context->on_close();
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
                KTRACE("Wayland seat: the keyboard is lost.");
            }
            else
            {
                context->wkeyboard = wl_seat_get_keyboard(wseat);
                wl_keyboard_add_listener(context->wkeyboard, &keyboard_listeners, null);
                KTRACE("Wayland seat: the keyboard is found.");
            }
        }

        if(capabilities & WL_SEAT_CAPABILITY_POINTER)
        {
            if(context->wpointer)
            {
                wl_pointer_release(context->wpointer);
                context->wpointer = null;
                KTRACE("Wayland seat: the pointer is lost.");
            }
            else
            {
                context->wpointer = wl_seat_get_pointer(wseat);
                wl_pointer_add_listener(context->wpointer, &pointer_listeners, null);
                KTRACE("Wayland seat: the pointer is found.");
            }
        }
    }

    void wseat_name(void* data, struct wl_seat* wseat, const char* name)
    {
    }

    u32 kb_translate_keycode(u32 keycode)
    {
        // TODO: Реализовать таблицу трансляции linux keycode -> virtual keycode.
        //       Смотреть linux/input-event-codes.h
        return keycode;
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
    }

    void kb_mods(void* data, struct wl_keyboard* wkeyboard, u32 serial, u32 depressed, u32 latched, u32 locked, u32 group)
    {
    }

    void kb_repeat(void* data, struct wl_keyboard* wkeyboard, i32 rate, i32 delay)
    {
    }

    u32 pt_translate_button_code(u32 button_code)
    {
        // TODO: Реализовать таблицу трансляции linux button code -> virtual button code.
        //       Смотреть linux/input-event-codes.h
        return button_code;
    }

    void pt_enter(void* data, struct wl_pointer* wpointer, u32 serial, struct wl_surface* wsurface, wl_fixed_t x, wl_fixed_t y)
    {
        if(context->on_focus)
        {
            context->on_focus(true);
        }
    }

    void pt_leave(void* data, struct wl_pointer* wpointer, u32 serial, struct wl_surface* wsurface)
    {
        if(context->on_focus)
        {
            context->on_focus(false);
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
    }

    void pt_button(void* data, struct wl_pointer* wpointer, u32 serial, u32 time, u32 button, u32 state)
    {
        if(context->on_mouse_button)
        {
            context->on_mouse_button(
                pt_translate_button_code(button), state == WL_POINTER_BUTTON_STATE_PRESSED ? true : false
            );
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

    void platform_window_handler_close_set(PFN_window_handler_close handler)
    {
        if(context)
        {
            context->on_close = handler;
        }
    }

    void platform_window_handler_resize_set(PFN_window_handler_resize handler)
    {
        if(context)
        {
            context->on_resize = handler;
        }
    }

    void platform_window_handler_keyboard_key_set(PFN_window_handler_keyboard_key handler)
    {
        if(context)
        {
            context->on_keyboard_key = handler;
        }
    }

    void platform_window_handler_mouse_move_set(PFN_window_handler_mouse_move handler)
    {
        if(context)
        {
            context->on_mouse_move = handler;
        }
    }

    void platform_window_handler_mouse_button_set(PFN_window_handler_mouse_button handler)
    {
        if(context)
        {
            context->on_mouse_button = handler;
        }
    }

    void platform_window_handler_mouse_wheel_set(PFN_window_handler_mouse_wheel handler)
    {
        if(context)
        {
            context->on_mouse_wheel = handler;
        }
    }

    void platform_window_handler_focus_set(PFN_window_handler_focus handler)
    {
        if(context)
        {
            context->on_focus = handler;
        }
    }

#endif
