#pragma once

#include <defines.h>

// @brief Определение указателя функции для задания на выполнение.
typedef bool (*PFN_job_entry)(void*, void*);

// @brief Определения указателя функции для события завершения задания.
typedef void (*PFN_job_on_complete)(void*) ;

// @brief Тип задания.
typedef enum job_type {

    /*
        @brief Обычное задание, без каких либо требований к выполнению потока. Это означает,
               что не имеет значения, в каком потоке выполняется эта работа.
    */
    JOB_TYPE_GENERAL = 0x02,

    /*
        @brief Задание по загрузке ресурсов. Они всегда должны загружаться в одном потоке,
               чтобы избежать потенциального переполнения диска.
    */
    JOB_TYPE_RESOURCE_LOAD = 0x04,

    /*
        @brief Задания, использующие ресурсы GPU, должны быть привязаны к потоку, использующему
               этот тип задания. Многопоточные рендереры будут использовать определенный поток
               задания, и этот тип задания будет выполняться в этом потоке. Для однопоточных
               рендереров это будет в основном потоке.
    */
    JOB_TYPE_GPU_RESOURCE = 0x08

} job_type;

// @brief Тип очереди заданий.
typedef enum job_priority {

    /*
        @brief Задание с самым низким приоритетом. Используется для задач, которые могут подождать,
               если это необходимо. Например, логирование.
    */
    JOB_PRIORITY_LOW,

    /*
        @brief Задание с обычным приоритетом. Используется для задач среднего приоритета. Например,
               таких как загрузка ресурсов.
    */
    JOB_PRIORITY_NORMAL,

    /*
        @brief Задание с самым высоким приоритетом. Используется только для срочных операций.
    */
    JOB_PRIORITY_HIGH

} job_priority;

// @brief Контекст задания.
typedef struct job {
    // @brief Тип задания. Используется для определения того, в каком потоке выполняется задание.
    job_type type;
    // @brief Приоритет задания. Более приоритетные задания выполняются быстее.
    job_priority priority;
    // @brief Указатель на функцию с заданием, которая будет вызвана при запуске задания (ОБЯЗАТЕЛЬНО).
    PFN_job_entry entry_point;
    // @brief Указатель на функцию которая будет вызвана при успешном завершении задания (ОПЦИОНАЛЬНО).
    PFN_job_on_complete on_success;
    // @brief Указатель на функцию которая будет вызвана при неудачном завершении задания (ОПЦИОНАЛЬНО).
    PFN_job_on_complete on_fail;
    // @brief Данные, передаваемые в точку входа выполнения задания entry_point (ОПЦИОНАЛЬНО).
    void* param_data;
    // @brief Размер данных, передаваемых в точку входа выполнения задания (ОПЦИОНАЛЬНО).
    u32 param_data_size;
    // @brief Данные, которые необходимо передать в точку завершения задания on_success/on_fail (ОПЦИОНАЛЬНО).
    void* result_data;
    // @brief Размер данных, передаваемых в точку завершения задания (ОПЦИОНАЛЬНО).
    u32 result_data_size;
} job;

// @brief Описывает конфигурацию системы заданий.
typedef struct job_system_config {
    // @brief Максимальное количество потоков, которое необходимо запустить, для выполнения заданий.
    u8 max_job_thread_count;
    // @brief Массив с масками типов для потоков заданий (на каждый поток по одному значению).
    u32* type_masks;
} job_system_config;

/*
    @brief Инициализирует систему заданий. Вызывается дважды: первый раз (memory = null), для получения
           требований к системе, второй раз с указанием участа памяти для инициализации системы.
    @param memory_requirement Указатель на переменную для сохранения требований системы к памяти в байтах.
    @param memory Указатель на выделенный блок памяти, или null для получения требований.
    @param config Конфигурация используемая для инициализации системы; получения требований к памяти.
    @return True в случае успеха, false если есть ошибки.
*/
bool job_system_initialize(u64* memory_requirement, void* memory, job_system_config* config);

/*
    @brief Завершает работу системы заданий.
*/
void job_system_shutdown();

/*
    @brief Обновляет систему заданий (один раз в цикл).
*/
void job_system_update();

/*
    @brief Отправляет предоставленное задание в очередь на выполнение.
    @param job Задание для оправки на выполнение.
*/
KAPI void job_system_submit(job* job);

/*
    @brief Создает новое задание с указанием типа и приоритета.
    @param type Тип задания. Используется для определения того, в каком потоке выполняется задание.
    @param priority Приоритет задания. Более приоритетные задания выполняются быстее.
    @param entry_point Указатель на функцию задачи для выполнения (ОБЯЗАТЕЛЬНО).
    @param on_success Указатель на функицю события об успешном завершении задания (ОПЦИОНАЛЬНО).
    @param on_fail Указатель на функицю события о неудачном завершении задания (ОПЦИОНАЛЬНО).
    @param param_data Указатель на данные для передачи в функцию с заданием (ОПЦИОНАЛЬНО).
    @param param_data_size Размер данных передаваемых в функцию с заданием (ОПЦИОНАЛЬНО).
    @param result_data_size Размер данныех передаваемых в функцию события (ОПЦИОНАЛЬНО).
    @return Новое задания для отправки на выполнение.
*/
KAPI job job_create(
    job_type type, job_priority priority, PFN_job_entry entry_point, PFN_job_on_complete on_success, PFN_job_on_complete on_fail,
    void* param_data, u32 param_data_size, u32 result_data_size
);


/*
    @brief Создает новое задание с типом (GENERIC) и приоритетом (NORMAL) по умолчанию.
    @param entry_point Указатель на функцию задачи для выполнения (ОБЯЗАТЕЛЬНО).
    @param on_success Указатель на функицю события об успешном завершении задания (ОПЦИОНАЛЬНО).
    @param on_fail Указатель на функицю события о неудачном завершении задания (ОПЦИОНАЛЬНО).
    @param param_data Указатель на данные для передачи в функцию с заданием (ОПЦИОНАЛЬНО).
    @param param_data_size Размер данных передаваемых в функцию с заданием (ОПЦИОНАЛЬНО).
    @param result_data_size Размер данныех передаваемых в функцию события (ОПЦИОНАЛЬНО).
    @return Новое задания для отправки на выполнение.
*/
#define job_create_default(entry_point, on_success, on_fail, param_data, param_data_size, result_data_size) \
    job_create(JOB_TYPE_GENERAL, JOB_PRIORITY_NORMAL, entry_point, on_success, on_fail, param_data, param_data_size, result_data_size)

/*
    @brief Создает новое задание с указанием типа и приоритетом (NORMAL) по умолчанию.
    @param type Тип задания. Используется для определения того, в каком потоке выполняется задание.
    @param entry_point Указатель на функцию задачи для выполнения (ОБЯЗАТЕЛЬНО).
    @param on_success Указатель на функицю события об успешном завершении задания (ОПЦИОНАЛЬНО).
    @param on_fail Указатель на функицю события о неудачном завершении задания (ОПЦИОНАЛЬНО).
    @param param_data Указатель на данные для передачи в функцию с заданием (ОПЦИОНАЛЬНО).
    @param param_data_size Размер данных передаваемых в функцию с заданием (ОПЦИОНАЛЬНО).
    @param result_data_size Размер данныех передаваемых в функцию события (ОПЦИОНАЛЬНО).
    @return Новое задания для отправки на выполнение.
*/
#define job_create_type(type, entry_point, on_success, on_fail, param_data, param_data_size, result_data_size) \
    job_create(type, JOB_PRIORITY_NORMAL, entry_point, on_success, on_fail, param_data, param_data_size, result_data_size)

