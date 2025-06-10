// Собственные подключения.
#include "systems/job_system.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"
#include "containers/ring_queue.h"
#include "kmutex.h"
#include "kthread.h"

// Представляет рабочий поток для выполнения заданий.
typedef struct job_thread {
    // Индекс потока.
    u8 index;
    // Контекст потока.
    thread thread;
    // Контекст задания.
    job job;
    // Контекст мьютекса для доступа к данным задания.
    mutex job_mutex;
    // Тип заданий для этого потока (можно комбинировать).
    job_type type_mask;
} job_thread;

// Представляет запись результата.
typedef struct job_result_entry {
    // Идентификатор задания?
    u16 id;
    // Указатель на функцию.
    PFN_job_on_complete callback;
    // Размер параметов.
    u32 param_size;
    // Парамерты функции.
    void* params;
} job_result_entry;

#define MAX_JOB_RESULTS 512

// Контекст системы заданий.
typedef struct job_system_state {
    // Флаг состояния системы.
    bool running;
    // Количество потоков для выполнения заданий.
    u8 thread_count;
    // Потоки для выполнения заданий.
    job_thread job_threads[32];
    // Очереди.
    ring_queue* low_priority_queue;
    ring_queue* norm_priority_queue;
    ring_queue* high_priority_queue;
    // Мьютексы для доступа к очередям.
    mutex low_pri_queue_mutex;
    mutex norm_pri_queue_mutex;
    mutex high_pri_queue_mutex;
    // Результаты заданий.
    job_result_entry pending_results[MAX_JOB_RESULTS];
    // Мьютекс для поступа к результатам заданий.
    mutex result_mutex;
} job_system_state;

static job_system_state* state_ptr = null;

static bool system_status_valid(const char* func_name)
{
    if(!state_ptr)
    {
        if(func_name)
        {
            kerror(
                "Function '%s' requires the job system to be initialized. Call 'job_system_initialize' first.",
                func_name
            );
        }
        return false;
    }
    return true;
}

void store_result(PFN_job_on_complete callback, void* params, u32 param_size)
{
    job_result_entry entry;
    entry.id = INVALID_ID_U16;
    entry.param_size = param_size;
    entry.callback = callback;

    if(entry.param_size > 0)
    {
        entry.params = kallocate(param_size, MEMORY_TAG_JOB);
        kcopy(entry.params, params, param_size);
    }
    else
    {
        entry.params = null;
    }

    if(!kmutex_lock(&state_ptr->result_mutex))
    {
        kerror("Failed to obtain mutex lock for storing a result! Result storage may be corrupted.");
    }

    for(u16 i = 0; i < MAX_JOB_RESULTS; ++i)
    {
        if(state_ptr->pending_results[i].id == INVALID_ID_U16)
        {
            state_ptr->pending_results[i] = entry;
            state_ptr->pending_results[i].id = i;
            break;
        }
    }

    if(!kmutex_unlock(&state_ptr->result_mutex))
    {
        kerror("Failed to release mutex lock for result storage, storage mey be corrupted.");
    }

    
}

// Выполняет ровно одну задачу поставленную в очередь.
u32 job_thread_run(void* params)
{
    u32 index = *((u32*)params);
    job_thread* thread = &state_ptr->job_threads[index];
    u64 thread_id = thread->thread.thread_id;
    ktrace("Starting job thread #%i (id=%#x, type=%#x).", thread->index, thread_id, thread->type_mask);

    if(!kmutex_create(&thread->job_mutex))
    {
        kerror("Function '%s' failed to create job thread mutex! Aborting thread.");
        return 0;
    }

    // Ожидание выполнения работы.
    while(true)
    {
        if(!state_ptr || !state_ptr->running || !thread)
        {
            break;
        }

        if(!kmutex_lock(&thread->job_mutex))
        {
            kerror("Failed to obtain lock on job thread mutex (thread #%i)!", thread->index);
        }

        // Копирование информации о задаче.
        job job = thread->job;

        if(!kmutex_unlock(&thread->job_mutex))
        {
            kerror("Failed to release lock on job thread mutex (thread #%i)!", thread->index);
        }

        if(job.entry_point)
        {
            bool result = job.entry_point(job.param_data, job.result_data);

            // Сохранение результата.
            if(result && job.on_success)
            {
                store_result(job.on_success, job.result_data, job.result_data_size);
            }
            else if(!result && job.on_fail)
            {
                store_result(job.on_fail, job.result_data, job.result_data_size);
            }

            // Очистка параметров и результата.
            if(job.param_data)
            {
                kfree(job.param_data, MEMORY_TAG_JOB);
                job.param_data = null;
            }

            if(job.result_data)
            {
                kfree(job.result_data, MEMORY_TAG_JOB);
                job.result_data = null;
            }

            if(!kmutex_lock(&thread->job_mutex))
            {
                kerror("Failed to obtain lock on job thread mutex!");
            }

            kzero_tc(&thread->job, job, 1);

            if(!kmutex_unlock(&thread->job_mutex))
            {
                kerror("Failed to release lock on job thread mutex!");
            }
        }

        if(state_ptr->running)
        {
            // TODO: Другой сбособ: блокировка потока, до момента как будет записана задача, после чего послан сигнал на разблокировку!
            kthread_sleep(&thread->thread, 10);
        }
    }

    kmutex_destroy(&thread->job_mutex);
    return 1;
}

bool job_system_initialize(u64* memory_requirement, void* memory, job_system_config* config)
{
    if(state_ptr)
    {
        kwarng("Function '%s' was called more than once!", __FUNCTION__);
        return false;
    }

    if(!memory_requirement || !config)
    {
        kerror("Function '%s' requires a valid pointers to memory_requirement and config.", __FUNCTION__);
        return false;
    }

    if(!config->max_job_thread_count)
    {
        kerror("Function '%s': config.max_job_thread_count must be greater then zero.", __FUNCTION__);
        return false;
    }

    *memory_requirement = sizeof(job_system_state);

    if(!memory)
    {
        return true;
    }

    // Обнуление заголовка системы камер.
    kzero_tc(memory, job_system_state, 1);

    state_ptr = memory;
    state_ptr->running = true;
    state_ptr->thread_count = config->max_job_thread_count;

    ring_queue_create(sizeof(job), 1024, null, null, &state_ptr->low_priority_queue);
    ring_queue_create(sizeof(job), 1024, null, null, &state_ptr->norm_priority_queue);
    ring_queue_create(sizeof(job), 1024, null, null, &state_ptr->high_priority_queue);

    // Отмечает все слоты как недействительные.
    for(u16 i = 0; i < MAX_JOB_RESULTS; ++i)
    {
        state_ptr->pending_results[i].id = INVALID_ID_U16;
    }

    kdebug("Main thread id is: %#x", kthread_get_id());
    kdebug("Spawning %i job threads.", state_ptr->thread_count);

    // Создание потоков для выполнения задач.
    for(u8 i = 0; i < state_ptr->thread_count; ++i)
    {
        state_ptr->job_threads[i].index = i;
        state_ptr->job_threads[i].type_mask = config->type_masks[i];

        if(!kthread_create(job_thread_run, &state_ptr->job_threads[i].index, false, &state_ptr->job_threads[i].thread))
        {
            kerror("Function '%s' failed creating job thread.", __FUNCTION__);
            return false;
        }

        kzero_tc(&state_ptr->job_threads[i].job, job, 1);
    }

    // Создание мтютексов.
    if(!kmutex_create(&state_ptr->result_mutex))
    {
        kerror("Failed to create result mutex.");
        return false;
    }

    if(!kmutex_create(&state_ptr->low_pri_queue_mutex))
    {
        kerror("Failed to create low priority queue mutex.");
        return false;
    }

    if(!kmutex_create(&state_ptr->norm_pri_queue_mutex))
    {
        kerror("Failed to create normal priority queue mutex.");
        return false;
    }

    if(!kmutex_create(&state_ptr->high_pri_queue_mutex))
    {
        kerror("Failed to create high priority queue mutex.");
        return false;
    }

    return true;
}

void job_system_shutdown()
{
    if(!system_status_valid(__FUNCTION__)) return;

    state_ptr->running = false;
    u64 thread_count = state_ptr->thread_count;

    for(u8 i = 0; i < thread_count; ++i)
    {
        kthread_destroy(&state_ptr->job_threads[i].thread);
    }

    ring_queue_destroy(state_ptr->low_priority_queue);
    ring_queue_destroy(state_ptr->norm_priority_queue);
    ring_queue_destroy(state_ptr->high_priority_queue);

    kmutex_destroy(&state_ptr->result_mutex);
    kmutex_destroy(&state_ptr->low_pri_queue_mutex);
    kmutex_destroy(&state_ptr->norm_pri_queue_mutex);
    kmutex_destroy(&state_ptr->high_pri_queue_mutex);

    state_ptr = null;
}

void process_queue(ring_queue* queue, mutex* queue_mutex)
{
    u64 thread_count = state_ptr->thread_count;

    while(ring_queue_length(queue))
    {
        job job;
        bool thread_fount = false;

        if(!ring_queue_peek(queue, &job)) break;

        for(u8 i = 0; i < thread_count; ++i)
        {
            job_thread* thread = &state_ptr->job_threads[i];
            if((thread->type_mask & job.type) == 0) continue;

            if(!kmutex_lock(&thread->job_mutex))
            {
                kerror("Failed to obtain lock on job thread mutex!");
            }

            if(!thread->job.entry_point)
            {
                if(!kmutex_lock(queue_mutex))
                {
                    kerror("Failed to obtain lock on queue mutex!");
                }

                ring_queue_dequeue(queue, &job);

                if(!kmutex_unlock(queue_mutex))
                {
                    kerror("Failed to release lock on queue mutex!");
                }

                thread->job = job;
                // ktrace("Assigning job to thread: %u", thread->index);
                thread_fount = true;
            }

            if(!kmutex_unlock(&thread->job_mutex))
            {
                kerror("Failed to release lock on job thread mutex!");
            }

            // Что бы небыло дедлока, принудительный выход из цикла вынесен.
            if(thread_fount) break;
        }

        // Выход из цикла обработки, т.к. заданий на выполнения не осталось.
        if(!thread_fount) break;
    }
}

void job_system_update()
{
    if(!system_status_valid(__FUNCTION__) || !state_ptr->running) return;

    process_queue(state_ptr->high_priority_queue, &state_ptr->high_pri_queue_mutex);
    process_queue(state_ptr->norm_priority_queue, &state_ptr->norm_pri_queue_mutex);
    process_queue(state_ptr->low_priority_queue, &state_ptr->low_pri_queue_mutex);

    // Обработка результатов.
    for(u16 i = 0; i < MAX_JOB_RESULTS; ++i)
    {
        if(!kmutex_lock(&state_ptr->result_mutex))
        {
            kerror("Failed to obtain lock on result mutex!");
        }

        job_result_entry result_entry = state_ptr->pending_results[i];

        if(!kmutex_unlock(&state_ptr->result_mutex))
        {
            kerror("Failed to release lock on result mutex!");
        }

        if(result_entry.id != INVALID_ID_U16)
        {
            result_entry.callback(result_entry.params);

            if(result_entry.params)
            {
                kfree(result_entry.params, MEMORY_TAG_JOB);
            }

            if(!kmutex_lock(&state_ptr->result_mutex))
            {
                kerror("Failed to obtain lock on result mutex!");
            }

            kzero_tc(&state_ptr->pending_results[i], job_result_entry, 1);
            state_ptr->pending_results[i].id = INVALID_ID_U16;

            if(!kmutex_unlock(&state_ptr->result_mutex))
            {
                kerror("Failed to release lock on result mutex!");
            }
        }
    }
}

void job_system_submit(job* job)
{
    u64 thread_count = state_ptr->thread_count;
    ring_queue* queue = state_ptr->norm_priority_queue;
    mutex* queue_mutex = &state_ptr->norm_pri_queue_mutex;

    // Если задание имеет высокий приоритет, попытаться выполнить немедленно.
    if(job->priority == JOB_PRIORITY_HIGH)
    {
        queue = state_ptr->high_priority_queue;
        queue_mutex = &state_ptr->high_pri_queue_mutex;

        // Проверка, поддерживает ли свободный поток тип задания.
        for(u8 i = 0; i < thread_count; ++i)
        {
            job_thread* thread = &state_ptr->job_threads[i];

            if(state_ptr->job_threads[i].type_mask & job->type)
            {
                bool found = false;

                if(!kmutex_lock(&thread->job_mutex))
                {
                    kerror("Filed to obtain lock on job thread mutex!");
                }

                if(!state_ptr->job_threads[i].job.entry_point)
                {
                    ktrace("Job immediately submitted on thread %i", state_ptr->job_threads[i].index);
                    state_ptr->job_threads[i].job = *job;
                    found = true;
                }

                if(!kmutex_unlock(&thread->job_mutex))
                {
                    kerror("Failed to release lock on job thread mutex!");
                }

                if(found) return;
            }
        }
    }

    // Если выполняется код ниже, то все потоки заняты или ожидают следующего кадра.
    if(job->priority == JOB_PRIORITY_LOW)
    {
        queue = state_ptr->low_priority_queue;
        queue_mutex = &state_ptr->low_pri_queue_mutex;
    }

    // NOTE: Блокировка на случай, если задание будет отправлено из другого задания/потока.
    if(!kmutex_lock(queue_mutex))
    {
        kerror("Failed to obtain lock on queue mutex!");
    }

    ring_queue_enqueue(queue, job);

    if(!kmutex_unlock(queue_mutex))
    {
        kerror("Failed to release lock on queue mutex!");
    }

    // ktrace("Job queued.");
}

job job_create(
    job_type type, job_priority priority, PFN_job_entry entry_point, PFN_job_on_complete on_success, PFN_job_on_complete on_fail,
    void* param_data, u32 param_data_size, u32 result_data_size
)
{
    job job;
    job.entry_point = entry_point;
    job.on_success = on_success;
    job.on_fail = on_fail;
    job.type = type;
    job.priority = priority;

    job.param_data_size = param_data_size;
    if(param_data_size)
    {
        job.param_data = kallocate(param_data_size, MEMORY_TAG_JOB);
        kcopy(job.param_data, param_data, param_data_size);
    }
    else
    {
        job.param_data = null;
    }

    job.result_data_size = result_data_size;
    if(result_data_size)
    {
        job.result_data = kallocate(result_data_size, MEMORY_TAG_JOB);
    }
    else
    {
        job.result_data = null;
    }

    return job;
}
