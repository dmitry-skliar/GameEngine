// Собственные подключения.
#include "platform/mutex.h"
#include "platform/memory.h"

#if KPLATFORM_LINUX_FLAG

    // Внешние подключения.
    #include <logger.h>
    #include <errno.h>
    #include <pthread.h>

    bool platform_mutex_create(mutex* out_mutex)
    {
        if(!out_mutex)
        {
            kerror("Function '%s' required non-null pointer to memory.", __FUNCTION__);
            return false;
        }

        pthread_mutex_t mutex;
        i32 result = pthread_mutex_init(&mutex, null);
        if(result != 0)
        {
            kerror("Function '%s' failed to create.", __FUNCTION__);
            return false;
        }

        out_mutex->internal_data = platform_memory_allocate(sizeof(pthread_mutex_t));
        *(pthread_mutex_t*)out_mutex->internal_data = mutex;

        return true;
    }

    void platform_mutex_destroy(mutex* mutex)
    {
        if(!mutex)
        {
            kerror("Function '%s' required a valid pointer to mutex.", __FUNCTION__);
            return;
        }

        i32 result = pthread_mutex_destroy((pthread_mutex_t*)mutex->internal_data);
        switch(result)
        {
            case 0:
                // ktrace("Mutex destroyed.");
                break;
            case EBUSY:
                kerror("Function '%s' unable to destroy mutex: mutex is locked or referenced.", __FUNCTION__);
                break;
            case EINVAL:
                kerror("Function '%s' unable to destroy mutex: the value specified by mutex is invalid.", __FUNCTION__);
                break;
            default:
                kerror("Function '%s' an handled error has occurred while destroy a mutex (errno = %i).", __FUNCTION__, result);
                break;
        }

        platform_memory_free(mutex->internal_data);
        mutex->internal_data = null;
    }

    bool platform_mutex_lock(mutex* mutex)
    {
        if(!mutex || !mutex->internal_data)
        {
            // TODO: Заменить прохожие проверки на утверждения (assertion).
            kerror("Function '%s' required a valid pointer to mutex.", __FUNCTION__);
            return false;
        }

        i32 result = pthread_mutex_lock((pthread_mutex_t*)mutex->internal_data);
        switch(result)
        {
            case 0:
                // ktrace("Obtained mutex lock.");
                return true;
            case EOWNERDEAD:
                kerror("Function '%s': Obtained thread terminated while mutex still active.", __FUNCTION__);
                return false;
            case EAGAIN:
                kerror("Function '%s' unable to obtain mutex lock: the maximum number of recursive mutex locks has been reached.", __FUNCTION__);
                return false;
            case EBUSY:
                kerror("Function '%s' unable to obtain mutex lock: a mutex lock already exists.", __FUNCTION__);
                return false;
            case EDEADLK:
                kerror("Function '%s' unable to obtain mutex lock: a mutex deadlock was detected..", __FUNCTION__);
                return false;
            default:
                kerror("Function '%s' an handled error has occurred while obtaining a mutex lock (errno = %i).", __FUNCTION__, result);
                return false;
        }
    }

    bool platform_mutex_unlock(mutex* mutex)
    {
        if(!mutex || !mutex->internal_data)
        {
            kerror("Function '%s' required a valid pointer to mutex.", __FUNCTION__);
            return false;
        }

        i32 result = pthread_mutex_unlock((pthread_mutex_t*)mutex->internal_data);
        switch(result)
        {
            case 0:
                // ktrace("Freed mutex lock.");
                return true;
            case EOWNERDEAD:
                kerror("Function '%s' unable to unlock mutex: owning thread terminated while mutex still active.", __FUNCTION__);
                return false;
            case EPERM:
                kerror("Function '%s' unable to unlock mutex: mutex not owned by current thread.", __FUNCTION__);
                return false;
            default:
                kerror("Function '%s' an handled error has occurred while unlocking a mutex lock (errno = %i).", __FUNCTION__, result);
                return false;
        }
    }

#endif
