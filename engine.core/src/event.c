// Cобственные подключения.
#include "event.h"

// Внутренние подключения.
#include "logger.h"
#include "debug/assert.h"
#include "memory/memory.h"
#include "containers/darray.h"

// Внешние подключения.

typedef struct registered_listener {
    void* instance;
    PFN_event_handler handler;
} registered_listener;

typedef struct event {
    // Представляет динамический массив.
    registered_listener* listeners;
} event;

typedef struct event_system_context {
    event events[EVENT_CODES_MAX];
} event_system_context;

// Указатель на контекст системы событий.
static event_system_context* context = null;

// Сообщения.
static const char* message_context_not_initialized = "Event system was not initialized. Please first call 'event_system_initialize'.";
static const char* message_has_no_listeners = "In function '%s' event code (%s) has no listeners.";
static const char* message_number_out_of_bounds = "Event code is out of bounds.";
static const char* message_handler_not_installed = "Handler not installed.";

bool event_system_initialize()
{
    kassert_debug(context == null, "Trying to call function 'event_system_initialize' more than once!");

    context = kmallocate_t(event_system_context, MEMORY_TAG_SYSTEM);
    if(!context)
    {
        kerror("Memory for event system was not allocated.");
        return false;
    }
    kmzero_tc(context, event_system_context, 1);

    kinfor("Event system started.");
    return true;
}

void event_system_shutdown()
{
    kassert_debug(context != null, message_context_not_initialized);

    for(i32 i = 0; i < EVENT_CODES_MAX; ++i)
    {
        if(context->events[i].listeners != null)
        {
            darray_destroy(context->events[i].listeners);
            context->events[i].listeners = null;
        }
    }

    kmfree(context);
    context = null;

    kinfor("Event system stopped.");
}

bool event_register(event_code code, void* listener, PFN_event_handler handler)
{
    kassert_debug(context != null, message_context_not_initialized);
    kassert_debug(code < EVENT_CODES_MAX, message_number_out_of_bounds);
    kassert_debug(handler != null, message_handler_not_installed);

    if(context->events[code].listeners == null)
    {
        context->events[code].listeners = darray_create(registered_listener);
    }

    u64 registered_count = darray_length_get(context->events[code].listeners);

    for(u64 i = 0; i < registered_count; ++i)
    {
        if(context->events[code].listeners[i].instance == listener
        && context->events[code].listeners[i].handler == handler)
        {
            kwarng("Event has alreay been registered with the code (%s).", event_code_name_get(code));
            return false;
        }
    }

    registered_listener record;
    record.instance = listener;
    record.handler  = handler;
    darray_push(context->events[code].listeners, record);

    return true;
}

bool event_unregister(event_code code, void* listener, PFN_event_handler handler)
{
    kassert_debug(context != null, message_context_not_initialized);
    kassert_debug(code < EVENT_CODES_MAX, message_number_out_of_bounds);
    kassert_debug(handler != null, message_handler_not_installed);

    if(context->events[code].listeners == null)
    {
        kwarng(message_has_no_listeners, __FUNCTION__, event_code_name_get(code));
        return false;
    }

    u64 registered_count = darray_length_get(context->events[code].listeners);

    for(u64 i = 0; i < registered_count; ++i)
    {
        registered_listener record = context->events[code].listeners[i];

        if(record.instance == listener && record.handler == handler)
        {
            darray_pop_at(context->events[code].listeners, i, null);
            return true;
        }
    }

    ktrace("In function '%s' event code (%s) has no listener!", __FUNCTION__, event_code_name_get(code));
    return false;
}

bool event_send(event_code code, void* sender, event_context data)
{
    kassert_debug(context != null, message_context_not_initialized);
    kassert_debug(code < EVENT_CODES_MAX, message_number_out_of_bounds);

    if(context->events[code].listeners == null)
    {
        ktrace(message_has_no_listeners, __FUNCTION__, event_code_name_get(code));
        return false;
    }

    u64 registered_count = darray_length_get(context->events[code].listeners);

    for(u64 i = 0; i < registered_count; ++i)
    {
        registered_listener record = context->events[code].listeners[i];

        if(record.handler(code, sender, record.instance, data))
        {
            // Сообщение было обработано, другие слушатели пропускаются.
            ktrace("In function '%s' was handled.", __FUNCTION__);
            return true;
        }
    }

    return false;
}

const char* event_code_name_get(event_code code)
{
    kassert_debug(code < EVENT_CODES_MAX, message_number_out_of_bounds);

    static const char* code_name[EVENT_CODES_MAX] = {
        [EVENT_CODE_NULL]                  = "EVENT_CODE_NULL",
        [EVENT_CODE_APPLICATION_QUIT]      = "EVENT_CODE_APPLICATION_QUIT",
        [EVENT_CODE_APPLICATION_RESIZE]    = "EVENT_CODE_APPLICATION_RESIZE",
        [EVENT_CODE_KEYBOARD_KEY_PRESSED]  = "EVENT_CODE_KEYBOARD_KEY_PRESSED",
        [EVENT_CODE_KEYBOARD_KEY_RELEASED] = "EVENT_CODE_KEYBOARD_KEY_RELEASED",
        [EVENT_CODE_MOUSE_BUTTON_PRESSED]  = "EVENT_CODE_MOUSE_BUTTON_PRESSED",
        [EVENT_CODE_MOUSE_BUTTON_RELEASED] = "EVENT_CODE_MOUSE_BUTTON_RELEASED",
        [EVENT_CODE_MOUSE_MOVED]           = "EVENT_CODE_MOUSE_MOVED",
        [EVENT_CODE_MOUSE_WHEEL]           = "EVENT_CODE_MOUSE_WHEEL"
    };

    return code_name[code];
}
