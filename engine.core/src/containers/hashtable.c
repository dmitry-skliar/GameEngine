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
    // Размер данных записи в байтах.
    u64 data_size;
    // Размер записи в байтах.
    u64 entry_size;
    // Количество записей в таблице.
    u64 entry_count_total;
    // Текущее количество записей в таблице.
    u64 entry_count_current;
};

u64 hashtable_get_hash(const char* key);
bool hashtable_found_entry(hashtable* table, const char* str, hashentry** out_entry);

bool hashtable_create(u64* memory_requirement, void* memory, hashtable_config* config, hashtable** out_table)
{
    if(!memory_requirement || !config)
    {
        kerror("Function '%s' requires a valid pointers to memory_requirement and config.", __FUNCTION__);
        return false;
    }

    if(!config->data_size || !config->entry_count)
    {
        kerror("Function '%s' requires data size and entry count.", __FUNCTION__);
        return false;
    }

    u64 entry_size = sizeof(hashentry) + config->data_size;
    *memory_requirement = sizeof(hashtable) + entry_size * config->entry_count;

    if(!memory)
    {
        return true;
    }

    if(!out_table)
    {
        kerror("Function '%s' requires a valid pointer to save pointer of hashtable.", __FUNCTION__);
        return false;
    }

    kzero(memory, *memory_requirement);
    hashtable* table = memory;

    table->data_size = config->data_size;
    table->entry_size = entry_size;
    table->entry_count_total = config->entry_count;

    *out_table = table;
    return true;
}

void hashtable_destroy(hashtable* table)
{
    if(!table)
    {
        kerror("Function '%s' requires a valid pointer to hashtable.", __FUNCTION__);
        return;
    }

    if(!table->entry_count_total)
    {
        kerror("Function '%s': hashtable is invalid or destroyed.", __FUNCTION__);
        return;
    }

    // Удаление строк.
    u8* first = (u8*)table + sizeof(hashtable);

    for(u32 i = 0; i < table->entry_count_total; ++i)
    {
        hashentry* entry = (void*)(first + i * table->entry_size);
        if(entry->key) string_free(entry->key);
    }

    // Освобождение памяти (где table->entry_count == 0 делает ее невозможной к использованию).
    kzero_tc(table, hashtable, 1);
}

bool hashtable_set(hashtable* table, const char* name, const void* value, bool update)
{
    if(!table || !name || !value)
    {
        kerror("Function '%s' requires a valid pointer to hashtable, name and value.", __FUNCTION__);
        return false;
    }

    if(!table->entry_count_total)
    {
        kerror("Function '%s': hashtable is invalid or destroyed.", __FUNCTION__);
        return false;
    }

    if(table->entry_count_current >= table->entry_count_total)
    {
        kerror("Function '%s': hashtable is crowded.", __FUNCTION__);
        return false;
    }

    hashentry* entry = null;

    // Ничего не найдено.
    if(!hashtable_found_entry(table, name, &entry))
    {
        // NOTE: Этой ситуации вообще не должно возникнуть!
        kerror("Function '%s': free entry or duplicate not found.", __FUNCTION__);
        return false;
    }

    // Найден дубликат и запрещена перезапись.
    if(entry->key && !update)
    {
        kwarng("Function '%s': entry already exist.", __FUNCTION__);
        return false;
    }

    // Запись строки, если запись пустая.
    if(!entry->key) entry->key = string_duplicate(name);

    // Получение смещения данных записи, т.к. данные идут после hashentry.
    void* data = (u8*)entry + sizeof(hashentry);
    kcopy(data, value, table->data_size);
    table->entry_count_current++;

    return true;
}

bool hashtable_get(hashtable* table, const char* name, void* out_value)
{
    if(!table || !name || !out_value)
    {
        kerror("Function '%s' requires a valid pointer to hashtable, name and value.", __FUNCTION__);
        return false;
    }

    if(!table->entry_count_total)
    {
        kerror("Function '%s': Hashtable is invalid or destroyed.", __FUNCTION__);
        return false;
    }

    hashentry* entry = null;

    // Ничего не найдено или найдена пустая строка.
    if(!table->entry_count_current || !hashtable_found_entry(table, name, &entry) || !entry->key)
    {
        kwarng("Function '%s': Entry not found.", __FUNCTION__);
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
    u64 index = hashtable_get_hash(str) % table->entry_count_total;
    hashentry* entry = (void*)(entry_first + index * table->entry_size);
    u64 distance = 0;

    // Поиск пустой строки или дубликата.
    while(entry->key && distance < table->entry_count_total)
    {
        if(string_equal(entry->key, str)) break;

        distance++;
        index++;
        index %= table->entry_count_total;
        entry = (void*)(entry_first + index * table->entry_size);
    }

    // Ничего не найдено.
    if(distance >= table->entry_count_total)
    {
        *out_entry = null;
        return false;
    }

    *out_entry = entry;
    return true;
}
