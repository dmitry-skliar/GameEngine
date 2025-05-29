// Собственные подключения.
#include "containers/ring_queue.h"

// Внутренние подключеня.
#include "logger.h"
#include "memory/memory.h"

struct ring_queue {
    // Количество элементов в очереди.
    u32 length;
    // Размер элемента в байтах.
    u32 stride;
    // Размер очереди из элементов.
    u32 capacity;
    // Указатель на память с данными.
    void* memory;
    // Указывает используется ли внутренний распределитель или собственный.
    bool owns_memory; // TODO: Переделать для других контейнеров!
    // Индекс начала очереди.
    i32 head;
    // Индукс окончания очереди.
    i32 tail;
};

bool ring_queue_create(u32 stride, u32 capacity, u64* memory_requirement, void* memory, ring_queue** out_queue)
{
    if(stride == 0 || capacity == 0)
    {
        kerror("Function '%s' requires stride and capacity more than zero.", __FUNCTION__);
        return false;
    }

    if(!out_queue)
    {
        kerror("Function '%s' requires a valid pointer to hold the queue.", __FUNCTION__);
        return false;
    }

    // Определение сколько необходимо памяти для очереди (контекст + данные).
    u64 requirement = sizeof(struct ring_queue) + stride * capacity;
    ring_queue* queue = null;

    // Определение какую память использовать.
    if(memory_requirement)
    {
        if(!memory)
        {
            *memory_requirement = requirement;
            return true;
        }

        queue = memory;
    }
    else
    {
        queue = kallocate(requirement, MEMORY_TAG_RING_QUEUE);
        if(!queue)
        {
            kerror("Function '%s' failed to allocate memory!", __FUNCTION__);
            return false;
        }
    }

    kzero_tc(queue, struct ring_queue, 1);
    queue->owns_memory = memory_requirement ? false : true;
    queue->memory = POINTER_GET_OFFSET(queue, sizeof(struct ring_queue));
    queue->stride = stride;
    queue->capacity = capacity;
    queue->length = 0;
    queue->head = 0;
    queue->tail = -1;

    *out_queue = queue;
    return true;
}

void ring_queue_destroy(ring_queue* queue)
{
    if(!queue)
    {
        kerror("Function '%s' requires a valid pointer to queue.", __FUNCTION__);
        return;
    }

    if(queue->owns_memory)
    {
        u64 requirement = sizeof(struct ring_queue) + queue->stride * queue->capacity;
        kfree(queue, requirement, MEMORY_TAG_RING_QUEUE);
    }
}

bool ring_queue_enqueue(ring_queue* queue, void* value)
{
    if(!queue || !value)
    {
        kerror("Function '%s' requires a valid pointer to queue and value.", __FUNCTION__);
        return false;
    }

    if(queue->length == queue->capacity)
    {
        kerror("Function '%s' attempted to enqueue value in full ring queue: %p", queue);
        return false;
    }

    queue->tail = (queue->tail + 1) % queue->capacity;
    kcopy(POINTER_GET_OFFSET(queue->memory, queue->tail * queue->stride), value, queue->stride);
    queue->length++;

    return true;
}

bool ring_queue_dequeue(ring_queue* queue, void* out_value)
{
    if(!queue || !out_value)
    {
        kerror("Function '%s' requires a valid pointer to queue and value.", __FUNCTION__);
        return false;
    }

    if(queue->length == 0)
    {
        kerror("Function '%s' attempted to dequeue value in empty ring queue: %p", queue);
        return false;
    }

    kcopy(out_value, POINTER_GET_OFFSET(queue->memory, queue->head * queue->stride), queue->stride);
    queue->head = (queue->head + 1) & queue->capacity;
    queue->length--;

    return true;
}

bool ring_queue_peek(const ring_queue* queue, void* out_value)
{
    if(!queue)
    {
        kerror("Function '%s' requires a valid pointer to queue.", __FUNCTION__);
        return false;
    }

    if(queue->length == 0)
    {
        kerror("Function '%s' attempted to peek value in empty ring queue: %p", queue);
        return false;
    }

    if(out_value)
    {
        kcopy(out_value, POINTER_GET_OFFSET(queue->memory, queue->head * queue->stride), queue->stride);
    }

    return true;
}

u32 ring_queue_length(const ring_queue* queue)
{
    if(!queue)
    {
        kerror("Function '%s' requires a valid pointer to queue.", __FUNCTION__);
        return 0;
    }

    return queue->length;
}
