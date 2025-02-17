#pragma once

#include <defines.h>

// @brief Контекст хэш-таблицы.
typedef struct hashtable hashtable;

// @brief Конфигурация хэш-таблицы.
typedef struct hashtable_config {
    // @brief Размер данных записи в байтах.
    u64 data_size;
    // @brief Количество записей таблицы.
    u64 entry_count;
} hashtable_config;

/*
    @brief Создает хэш-таблицу фиксированного размера.
    @brief memory_requirement Указатель на переменную для получения требований к памяти.
    @param memory Указатель на выделенную память, для получения требований к памяти передать null.
    @param config Конфигурация хэш-таблицы.
    @param out_table Указатель на хэш-таблицу, null если создать не удалось.
    @return True операция завершилась успешно, false операция завершилась неудачей.
*/
KAPI bool hashtable_create(u64* memory_requirement, void* memory, hashtable_config* config, hashtable** out_table);

/*
    @brief Уничтожает хэш-таблицу.
    @param table Указатель на хэш-таблицу.
*/
KAPI void hashtable_destroy(hashtable* table);

/*
    @brief Сохраняет копию данных в хэш-таблицу и привязывает их к ключевому слову.
    @param table Указатель на хэш-таблицу.
    @param name Ключевое слово (должно быть уникальным).
    @param value Данные для сохранения (для указателей использовать указатель на указатель).
    @param update Перезаписать существующих данных, если таковые имеются.
    @return True если данные сохранены успешно, false не удалось сохранить.
*/
KAPI bool hashtable_set(hashtable* table, const char* name, const void* value, bool update);

/*
    @brief Получить копию данных из хэш-таблицы по ключевому слову.
    @param table Указатель на хэш-таблицу.
    @param name Ключевое слово.
    @param out_value Указатель на память, куда скопировать данные (для указателей использовать указатель на указатель).
    @return True если данные получены успешно, false не удалось получить.
*/
KAPI bool hashtable_get(hashtable* table, const char* name, void* out_value);
