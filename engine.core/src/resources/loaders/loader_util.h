#pragma once

#include <defines.h>
#include <memory/memory.h>
#include <resources/resource_types.h>
#include <systems/resource_system.h>

/*
*/
void resource_unload(resource_loader* self, resource* resource, memory_tag tag, const char* func_name);

/*
    @brif Выполняет проверку переданых параметров функции загрузке ресурсов.
    @param self Указатель на экземпляр загрузчика ресурса.
    @param name Указатель на строку с именем ресурса для загрузки (путь к файлу).
    @param resource Указатель экземпляр ресурса содержащего запрашиваемый ресурс.
    @param func_name Указатель на строку с именем функции для которой выполняется проверка параметров.
    @return True параметры введены верно, false есть ошибки в веденных параметрах. 
*/
bool resource_loader_load_valid(resource_loader* self, const char* name, resource* resource, const char* func_name);
