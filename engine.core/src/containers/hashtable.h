#pragma once

#include <defines.h>

// @brief Экземпляр хэш-таблицы.
typedef struct hashtable hashtable;

/*
    @brief Создает хэш-таблицу фиксированного размера.
    @param data_size Размер элемента в байтах.
    @param data_count Количество элементов.
    @return Указатель на хэш-таблицу, null если создать не удалось.
*/
KAPI hashtable* hashtable_create(u64 data_size, u64 data_count);

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
