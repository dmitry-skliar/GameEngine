// Cобственные подключения.
#include "platform/thread.h"
#include "platform/memory.h"

#if KPLATFORM_LINUX_FLAG

    // Внешние подключения.
    #include <logger.h>
    #include <time.h>
    #include <errno.h>
    #include <pthread.h>
    #include <sys/sysinfo.h>

    void platform_thread_sleep(u64 time_ms)
    {
        struct timespec ts;
        ts.tv_sec  = time_ms / 1000;
        ts.tv_nsec = (time_ms % 1000) * 1000000;
        nanosleep(&ts, null);
    }

    i32 platform_thread_get_processor_count()
    {
        // i32 processor_count = get_nprocs_conf();
        i32 processors_available = get_nprocs();

        // kinfor("%i precessor cores detected, %i cores available.", processor_count, processors_available);

        return processors_available;
    }

    bool platform_thread_create(PFN_thread_entry* func, void* params, bool auto_detach, thread* out_thread)
    {
        if(!func)
        {
            kwarng("Function '%s' required a pointer to function.", __FUNCTION__);
            return false;
        }

        i32 result = pthread_create((pthread_t*)&out_thread->thread_id, 0, (void* (*)(void*))func, params);

        if(result != 0)
        {
            switch(result)
            {
                case EAGAIN:
                    kerror("Function '%s' failed to create thread: insufficient resources to create another thread.", __FUNCTION__);
                    return false;
                case EINVAL:
                    kerror("Function '%s' failed to create thread: invalid settings were passed in attributes.", __FUNCTION__);
                    return false;
                default:
                    kerror("Function '%s' failed to create thread: an unhandled error has occurred (errno = %i).", __FUNCTION__, result);
                    return false;
            }
        }

        kdebug("Starting process on thread id: %#x", out_thread->thread_id);

        if(!auto_detach)
        {
            out_thread->internal_data = platform_memory_allocate(sizeof(u64));
            *(u64*)out_thread->internal_data = out_thread->thread_id;
        }
        else
        {
            result = pthread_detach(out_thread->thread_id);

            if(result != 0)
            {
                switch(result)
                {
                    case EINVAL:
                        kerror("Function '%s' failed to detach newly-created thead: thread is not a joinable thread.", __FUNCTION__);
                        return false;
                    case ESRCH:
                        kerror("Function '%s' failed to detach newly-created thead: no thread with the id %#x could be found.", __FUNCTION__, out_thread->thread_id);
                        return false;
                    default:
                        kerror("Function '%s' failed to detach newly-created thead: an unknown error has occurred (errno = %i).", __FUNCTION__, result);
                        return false;
                }
            }
        }

        return true;
    }

    void platform_thread_destroy(thread* thread)
    {
        if(!thread || !thread->internal_data)
        {
            kerror("Function '%s' required a valid pointer to thread.", __FUNCTION__);
            return;
        }

        platform_thread_cancel(thread);
    }

    void platform_thread_detach(thread* thread)
    {
        if(!thread || !thread->internal_data)
        {
            kerror("Function '%s' required a valid pointer to thread.", __FUNCTION__);
            return;
        }

        i32 result = pthread_detach(*(pthread_t*)thread->internal_data);
        if(result != 0)
        {
            switch(result)
            {
                case EINVAL:
                    kerror("Function '%s' failed to detach thead: thread is not a joinable thread.", __FUNCTION__);
                    break;
                case ESRCH:
                    kerror("Function '%s' failed to detach thead: no thread with the id %#x could be found.", __FUNCTION__, thread->thread_id);
                    break;
                default:
                    kerror("Function '%s' failed to detach thead: an unknown error has occurred (errno = %i).", __FUNCTION__, result);
                    break;
            }
        }

        platform_memory_free(thread->internal_data);
        thread->internal_data = null;
    }

    void platform_thread_cancel(thread* thread)
    {
        if(!thread || !thread->internal_data)
        {
            kerror("Function '%s' required a valid pointer to thread.", __FUNCTION__);
            return;
        }

        i32 result = pthread_cancel(*(pthread_t*)thread->internal_data);
        if(result != 0)
        {
            switch(result)
            {
                case ESRCH:
                    kerror("Function '%s' failed to cancel thead: no thread with the id %#x could be found.", __FUNCTION__, thread->thread_id);
                    break;
                default:
                    kerror("Function '%s' failed to cancel thead: an unknown error has occurred (errno = %i).", __FUNCTION__, result);
                    break;
            }
        }

        platform_memory_free(thread->internal_data);
        thread->internal_data = null;
        thread->thread_id = 0;
    }

    bool platform_thread_is_active(thread* thread)
    {
        if(!thread || !thread->internal_data)
        {
            kerror("Function '%s' required a valid pointer to thread.", __FUNCTION__);
            return false;
        }

        // TODO: Реализовать иначе!
        return thread->internal_data != null;
    }

    u64 platform_thread_get_id()
    {
        return (u64)pthread_self();
    }

#endif
