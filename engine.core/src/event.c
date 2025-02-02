// Cобственные подключения.
#include "event.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"
#include "containers/darray.h"

typedef struct registered_listener {
    void* instance;
    PFN_event_handler handler;
} registered_listener;

typedef struct event {
    // Использует динамический массив.
    registered_listener* listeners;
} event;

typedef struct event_system_state {
    event events[EVENT_CODES_MAX];
} event_system_state;

static event_system_state* state_ptr = null;
static const char* message_not_initialized =
    "Function '%s' requires the event system to be initialized. Call 'event_system_initialize' first.";
static const char* message_code_out_of_bounds = "Function '%s': Event code is out of bounds. Return false!";
static const char* message_handler_not_present = "Function '%s' requires a handler function pointer. Return false!";

void event_system_initialize(u64* memory_requirement, void* memory)
{
    if(state_ptr)
    {
        kwarng("Function '%s' was called more than once!", __FUNCTION__);
        return;
    }

    *memory_requirement = sizeof(struct event_system_state);
    if(!memory) return;

    kzero(memory, *memory_requirement);
    state_ptr = memory;
}

void event_system_shutdown()
{
    if(!state_ptr)
    {
        kfatal(message_not_initialized, __FUNCTION__);
    }

    // Уничтожаем все созданые массивы!
    for(i32 i = 0; i < EVENT_CODES_MAX; ++i)
    {
        if(state_ptr->events[i].listeners != null)
        {
            kdebug("Event shutdown -> darray destroy -> event %d, listeners addr %p", i, state_ptr->events[i].listeners);
            darray_destroy(state_ptr->events[i].listeners);
            state_ptr->events[i].listeners = null;
        }
    }

    state_ptr = null;
}

bool event_register(event_code code, void* listener, PFN_event_handler handler)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return false;
    }

    if(!handler)
    {
        kerror(message_handler_not_present, __FUNCTION__);
        return false;
    }

    if(code >= EVENT_CODES_MAX)
    {
        kerror(message_code_out_of_bounds, __FUNCTION__);
        return false;
    }

    if(state_ptr->events[code].listeners == null)
    {
        state_ptr->events[code].listeners = darray_create(registered_listener);
    }

    u64 registered_count = darray_length(state_ptr->events[code].listeners);

    for(u64 i = 0; i < registered_count; ++i)
    {
        if(state_ptr->events[code].listeners[i].instance == listener
        && state_ptr->events[code].listeners[i].handler == handler)
        {
            kwarng(
                "Function '%s': Event registered with code (%s), listener or/and function handler. Return false!",
                __FUNCTION__, event_code_str(code)
            );
            return false;
        }
    }

    registered_listener r = { .instance = listener, .handler = handler };
    darray_push(state_ptr->events[code].listeners, r);
    return true;
}

bool event_unregister(event_code code, void* listener, PFN_event_handler handler)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return false;
    }

    if(!handler)
    {
        kerror(message_handler_not_present, __FUNCTION__);
        return false;
    }

    if(code >= EVENT_CODES_MAX)
    {
        kerror(message_code_out_of_bounds, __FUNCTION__);
        return false;
    }

    if(state_ptr->events[code].listeners == null)
    {
        kwarng("Function '%s': Event code (%s) has no listeners. Return false!", __FUNCTION__, event_code_str(code));
        return false;
    }

    u64 registered_count = darray_length(state_ptr->events[code].listeners);

    for(u64 i = 0; i < registered_count; ++i)
    {
        registered_listener r = state_ptr->events[code].listeners[i];

        if(r.instance == listener && r.handler == handler)
        {
            darray_pop_at(state_ptr->events[code].listeners, i, null);
            return true;
        }
    }

    kwarng(
        "Function '%s': Event code (%s) does not have a listener or/and function handler. Return false!",
        __FUNCTION__, event_code_str(code)
    );
    return false;
}

bool event_send(event_code code, void* sender, event_context* context)
{
    if(!state_ptr)
    {
        kerror(message_not_initialized, __FUNCTION__);
        return false;
    }

    if(code >= EVENT_CODES_MAX)
    {
        kerror(message_code_out_of_bounds, __FUNCTION__);
        return false;
    }

    if(state_ptr->events[code].listeners == null)
    {
        // NOTE: Включить при отладке!
        // ktrace("Function '%s': Event code (%s) has no listeners.", __FUNCTION__, event_code_str(code));
        return false;
    }

    u64 registered_count = darray_length(state_ptr->events[code].listeners);

    for(u64 i = 0; i < registered_count; ++i)
    {
        registered_listener r = state_ptr->events[code].listeners[i];

        if(r.handler(code, sender, r.instance, context))
        {
            // Сообщение было обработано, другие слушатели пропускаются.
            ktrace(
                "Function '%s' has processed an event. Other listeners are skipped (Current %llu, Total %llu).",
                __FUNCTION__, i, registered_count
            );
            return true;
        }
    }

    return false;
}

const char* event_code_str(event_code code)
{
    static const char* names[EVENT_CODES_MAX] = {
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

    if(code >= EVENT_CODES_MAX)
    {
        return names[EVENT_CODE_NULL];
    }
    return names[code];
}
