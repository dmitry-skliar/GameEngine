// Собственные подключения.
#include "hashtable.h"

// Внутренние подключения.
#include "logger.h"
#include "memory/memory.h"
#include "kstring.h"

// TODO: Можно улучшить, добавив ссылку на следующий элемент и дистанцию
//       получивший такой же ключ! Это сокращает количество прыжков при
//       поиске! Так же, можно улучшить сделав несколько хэш-функций для
//       строки.

// NOTE: Каждая запись содержит hashentry + память для хранения данных.
typedef struct hashentry {
    const char* key;
} hashentry;

struct hashtable {
    u64 size;
    u64 capacity;
    u64 data_size;
    u64 entry_size;
};

u64 hashtable_get_hash(const char* key);
bool hashtable_found_entry(hashtable* table, const char* str, hashentry** out_entry);

hashtable* hashtable_create(u64 data_size, u64 data_count)
{
    if(!data_size || !data_count)
    {
        kerror("Function '%s' requires data size and count. Return null!", __FUNCTION__);
        return null;
    }

    u64 entry_size = sizeof(hashentry) + data_size;
    u64 total_size = sizeof(hashtable) + entry_size * data_count;
    hashtable* table = kallocate(total_size, MEMORY_TAG_HASHTABLE);

    if(!table)
    {
        kerror("Function '%s': Failed to allocate memory. Return null!", __FUNCTION__);
        return null;
    }

    kzero(table, total_size);
    table->capacity = data_count;
    table->data_size = data_size;
    table->entry_size = entry_size;

    return table;
}

void hashtable_destroy(hashtable* table)
{
    if(!table)
    {
        kerror("Function '%s' requires a valid pointer to hashtable. Just return!", __FUNCTION__);
        return;
    }

    // Удаление строк.
    u8* first = (u8*)table + sizeof(hashtable);

    for(u32 i = 0; i < table->capacity; ++i)
    {
        hashentry* entry = (hashentry*)(first + i * table->entry_size);
        if(entry->key) string_free(entry->key);
    }

    // Освобождение памяти.
    u64 total_size = sizeof(hashtable) + table->entry_size * table->capacity;
    kfree(table, total_size, MEMORY_TAG_HASHTABLE);
}

bool hashtable_set(hashtable* table, const char* name, const void* value, bool update)
{
    if(!table || !name || !value)
    {
        kerror("Function '%s' requires a valid pointer to hashtable, name and value. Return false!", __FUNCTION__);
        return false;
    }

    if(table->size >= table->capacity)
    {
        kwarng("Function '%s': hashtable is crowded. Return false!", __FUNCTION__);
        return false;
    }

    hashentry* entry = null;

    // Ничего не найдено.
    if(!hashtable_found_entry(table, name, &entry))
    {
        // NOTE: Этой ситуации вообще не должно возникнуть!
        kerror("Function '%s': free entry or duplicate not found. Return false!", __FUNCTION__);
        return false;
    }

    // Найден дубликат и запрещена перезапись.
    if(entry->key && !update)
    {
        kwarng("Function '%s': entry already exist. Return false!", __FUNCTION__);
        return false;
    }

    // Запись строки, если запись пустая.
    if(!entry->key) entry->key = string_duplicate(name);

    // Получение смещения данных записи, т.к. данные идут после hashentry.
    void* data = (u8*)entry + sizeof(hashentry);
    kcopy(data, value, table->data_size);
    table->size++;

    return true;
}

bool hashtable_get(hashtable* table, const char* name, void* out_value)
{
    if(!table || !name || !out_value)
    {
        kerror("Function '%s' requires a valid pointer to hashtable, name and value. Return false!", __FUNCTION__);
        return false;
    }

    hashentry* entry = null;

    // Ничего не найдено или найдена пустая строка.
    if(!table->size || !hashtable_found_entry(table, name, &entry) || !entry->key)
    {
        kwarng("Function '%s': entry not found. Return false!", __FUNCTION__);
        return false;
    }

    void* data = (u8*)entry + sizeof(hashentry);
    kcopy(out_value, data, table->data_size);

    return true;
}

u64 hashtable_get_hash(const char* key)
{
    u64 hash = 0;

    while(*key)
    {
        hash += *key;
        key++;
    }

    return hash;
}

// Ищет дубликат или пустую строку или возарвщвет false.
bool hashtable_found_entry(hashtable* table, const char* str, hashentry** out_entry)
{
    // Начальная инициализация.
    const u8* entry_first = (u8*)table + sizeof(hashtable);
    u64 index = hashtable_get_hash(str) % table->capacity;
    hashentry* entry = (void*)(entry_first + index * table->entry_size);
    u64 distance = 0;

    // Поиск пустой строки или дубликата.
    while(entry->key && distance < table->capacity)
    {
        if(string_equal(entry->key, str)) break;

        distance++;
        index++;
        index %= table->capacity;
        entry = (void*)(entry_first + index * table->entry_size);
    }

    // Ничего не найдено.
    if(distance >= table->capacity)
    {
        *out_entry = null;
        return false;
    }

    *out_entry = entry;
    return true;
}
