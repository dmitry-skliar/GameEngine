// Cобственные подключения.
#include "renderer/renderer_frontend.h"
#include "renderer/renderer_backend.h"

// Внутренние подключения.
#include "logger.h"
#include "debug/assert.h"
#include "memory/memory.h"

// Указатель на структуру контекста бэкенда выбранного рендера.
static renderer_backend* backend = null;

// Сообщения.
static const char* message_backend_not_created = "Renderer context was not created. Please first call 'renderer_initialize'.";

bool renderer_initialize(const char* application_name, u32 width, u32 height)
{
    kassert_debug(backend == null, "Trying to call function 'renderer_initialize' more than once!");

    backend = kmallocate_t(renderer_backend, MEMORY_TAG_RENDERER);
    if(!backend)
    {
        kerror("Failed to allocate renderer memory context.");
        return false;
    }

    // TODO: Сделать настраиваемым из приложения!
    renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, backend);

    if(!backend->initialize(backend, application_name, width, height))
    {
        return false;
    }

    return true;
}

void renderer_shutdown()
{
    kassert_debug(backend != null, message_backend_not_created);

    backend->shutdown(backend);
    kmfree(backend);
    backend = null;
}

bool renderer_begin_frame(f32 delta_time)
{
    return backend->begin_frame(backend, delta_time);
}

bool renderer_end_frame(f32 delta_time)
{
    bool result = backend->end_frame(backend, delta_time);
    backend->frame_number++;
    return result;
}

bool renderer_draw_frame(render_packet* packet)
{
    kassert_debug(backend != null, message_backend_not_created);

    if(renderer_begin_frame(packet->delta_time))
    {
        bool result = renderer_end_frame(packet->delta_time);

        if(!result)
        {
            kerror("Failed to complete function 'renderer_end_frame'. Shutting down.");
            return false;
        }
    }

    return true;
}

void renderer_on_resize(i32 width, i32 height)
{
    // TODO: Может плохо сыграть, заменить на простые проверки похожие ситуации!
    kassert_debug(backend != null, message_backend_not_created);
    backend->resized(backend, width, height);
}
