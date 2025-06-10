#################################################################################################
### Насторйки сборки.                                                                           #
#################################################################################################

# Предустановки платформы и редации.
#	Можно оставлять пустыми, но при вызове утилиты 'make' необходимо указать редакцию, так как
#	платформу выбрать попытается автоматически.
platform                    ?=
edition                     ?= Debug

# Поддреживаемые проектом платформы и редации.
__platforms                 := Linux Windows
__editions                  := Release Debug

# Отдельные модули проекта (Добавлять новые модули здесь).
#	Порядок сборки важен, если есть зависимости можду модулями. Определен следующий порядок cлева 
#	направо - для модулей одного типа, и сверху вниз для всех типов модулей. Прописываются имена 
#	директорий. Одна директория - один модуль. Исключение: postbuild. 
__libraries                 := engine.core
__applications              := application engine.core.tests
__postbuild                 := assets

# Конечные директории.
#	Директории куда записывать файлы в процессе выполнения и по завершению. Исключение: postbuild.
__binary_directory          := bin/
__object_directory          := bin/objs/

#################################################################################################
###  Настройки модулей по умолчанию.                                                            #
#################################################################################################

# Общие флаги специально для отладки (edition=Debug).
__debug_common_flags        := -g
__debug_define_flags        := -DKDEBUG_FLAG

# Общие флаги для библиотек.
__library_compiler_util     := clang
__library_common_flags      := -fvisibility=hidden
__library_define_flags      := -DKEXPORT_FLAG
__library_object_flags      := -fdeclspec -fPIC -Werror -Werror=vla -Wreturn-type
__library_linker_flags      := -shared

# Общие флаги для приложений.
__application_compiler_util := clang
__application_common_flags  := -fvisibility=hidden
__application_define_flags  := -DKIMPORT_FLAG
__application_object_flags  := -fdeclspec -fPIC -Werror -Werror=vla -Wreturn-type
__application_linker_flags  := -Wl,-rpath,. -L$(__binary_directory)

# Общие флаги для заверщающего этапа (postbuild).
__shader_compiler_util      := glslc
__shader_vert_flags         := -fshader-stage=vert
__shader_frag_flags         := -fshader-stage=frag

#################################################################################################
### Определение платформы и проверка опций (в том числе введенных пользователем).               #
#################################################################################################

# Автоопределение платформы.
#	Произойдет в том случае, если переменная platform не была установлена. Если платформа не будет
#	определена, то в значение platform будет записано 'Unknown'.
ifeq ($(strip $(platform)),)
	ifeq ($(findstring ;,$(PATH)),;)
		platform := Windows
	else
		platform := $(strip $(shell uname 2>/dev/null || echo Unknown))
		platform := $(strip $(patsubst CYGWIN%,Cygwin,$(platform)))
		platform := $(strip $(patsubst MSYS%,MSYS,$(platform)))
		platform := $(strip $(patsubst MINGW%,MSYS,$(platform)))
	endif
endif

# Проверка введенной платформы.
ifeq ($(strip $(filter $(platform),$(__platforms))),)
@$(error 'Указана неизвестная платформа '$(platform)', доступны: $(__platforms)...')
endif

# Проверка введенной редакции.
ifeq ($(strip $(filter $(edition),$(__editions))),)
@$(error 'Указана неизвестная редакция '$(edition)', доступны: $(__editions)...')
endif

#################################################################################################
### Расширенные функции.                                                                        #
#################################################################################################

# Рекурсивный поиск файлов заданного формата.
#	$1 - директория поиска.
#	$2 - формат файла.
__rwildcard = $(strip $(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call __rwildcard,$d/,$2)))

# Выполнение команды или вывод текста в режиме отладки (edition=Debug).
#	$1 - команда на выполнение или текст для вывода.
__ifdebug = $(if $(findstring $(strip $(edition)),Debug),$1)

# Выполнение команды или вывод текста для приложения.
__ifapp = $(if $(findstring $(strip $(module_type)),Application),$1)

# Выполнение команды или вывод текста для библиотеки.
__iflib = $(if $(findstring $(strip $(module_type)),Library),$1)

#################################################################################################
### Платформозависимые настройки модулей.                                                       #
#################################################################################################

# Linux платформа.
ifeq ($(platform),Linux)
__platform_mkdir_util         := mkdir -p
__platform_rm_util            := rm -rf
__platform_library_prefix     := lib
__platform_library_format     := .so
__platform_application_format :=
__platform_define_flags       := -DKPLATFORM_LINUX_FLAG -DKPLATFORM_LINUX_WAYLAND_FLAG
__platform_define_flags       += -DKVULKAN_USE_CUSTOM_ALLOCATOR_FLAG
# __platform_define_flags       += -DKVULKAN_ALLOCATOR_TRACE_FLAG
__platform_linker_flags       := -lwayland-client -lxkbcommon
endif

# Windows платформа.
ifeq ($(platform),Windows)
__platform_mkdir_util         :=
__platform_rm_util            :=
__platform_library_prefix     :=
__platform_library_format     := .dll
__platform_application_format := .exe
__platform_define_flags       := -DKPLATFORM_WINDOWS_FLAG
__platform_linker_flags       :=
endif

#################################################################################################
### Временные переменные.                                                                       #
#################################################################################################

ifneq ($(strip $(mkfile)),)

# Подключаем переменные выбранного модуля.
include $(mkfile)

__module_root_directory       := $(strip $(dir $(mkfile)))
__module_source_directory     := $(strip $(__module_root_directory)$(module_source_directory))
__module_include_directory    := $(strip $(__module_root_directory)$(if $(strip $(module_include_directory)),$(module_include_directory),$(module_source_directory)))

# Блок для библиотек и приложений.
__module_common_flags         := $(strip $(call __ifdebug,$(__debug_common_flags)) $(call __ifapp,$(__application_common_flags)) $(call __iflib,$(__library_common_flags)) $(module_common_flags))
__module_define_flags         := $(strip $(call __ifdebug,$(__debug_define_flags)) $(call __ifapp,$(__application_define_flags)) $(call __iflib,$(__library_define_flags)) $(__platform_define_flags) $(module_define_flags))
__module_include_flags        := $(strip -I$(__module_include_directory) $(module_include_flags))
__module_object_flags         := $(strip $(call __ifapp,$(__application_object_flags)) $(call __iflib,$(__library_object_flags)) $(module_object_flags))
__module_linker_flags         := $(strip $(call __ifapp,$(__application_linker_flags)) $(call __iflib,$(__library_linker_flags)) $(call __iflib,$(__platform_linker_flags)) $(module_linker_flags))

__module_library_filename     := $(strip $(__platform_library_prefix)$(module_filename)$(__platform_library_format))
__module_application_filename := $(strip $(module_filename)$(__platform_application_format))

__module_source_files         := $(strip $(call __rwildcard,$(__module_source_directory),*.c))
__module_object_files         := $(strip $(__module_source_files:$(__module_source_directory)%.c=$(__object_directory)$(__module_root_directory)%.o))

__module_compiler_util        := $(strip $(call __ifapp,$(__application_compiler_util)) $(call __iflib,$(__library_compiler_util)))

# Блок для шейдеров.
__module_vert_flags           := $(strip $(__shader_vert_flags) $(module_vert_flags))
__module_frag_flags           := $(strip $(__shader_frag_flags) $(module_frag_flags))

__module_source_vert_files    := $(strip $(call __rwildcard,$(__module_source_directory),*.vert.glsl))
__module_source_frag_files    := $(strip $(call __rwildcard,$(__module_source_directory),*.frag.glsl))

__module_object_vert_files    := $(strip $(__module_source_vert_files:%.vert.glsl=%.vert.spv))
__module_object_frag_files    := $(strip $(__module_source_frag_files:%.frag.glsl=%.frag.spv))

endif

#################################################################################################
### Основные цели.                                                                              #
#################################################################################################

.PHONY: help build rebuild clean $(__libraries) $(__applications) $(__postbuild)

help:
	@echo ""
	@echo "Используй так: make [target/module] [platform=] [edition=]"
	@echo ""
	@echo "Цели:"
	@echo "    build   - сборка проекта."
	@echo "    rebuild - пересборка проекта."
	@echo "    clean   - очистка директории '$(__binary_directory)'."
	@echo "    help    - для вывода этой помощи."
	@echo ""
	@echo "Модули    : $(__libraries) $(__applications) $(__postbuild)"
	@echo "Редакции  : $(__editions)"
	@echo "Платформы : $(__platforms)"
	@echo ""

build: $(__libraries) $(__applications) $(__postbuild)
	@echo "Сборка проекта завершена."

clean:
	@$(__platform_rm_util) $(__binary_directory)*
	@echo "Очистка проекта завершена."

rebuild: clean build

$(__libraries):
	$(if $(strip $(wildcard $@/module.mk)),\
	@make --no-print-directory __build_library module_type=Library mkfile=$@/module.mk,\
	@echo "Модуль $@ пропускается, файл проекта не найден.")

$(__applications):
	$(if $(strip $(wildcard $@/module.mk)),\
	@make --no-print-directory __build_application module_type=Application mkfile=$@/module.mk,\
	@echo "Модуль $@ пропускается, файл проекта не найден.")

$(__postbuild):
	$(if $(strip $(wildcard $@/module-shaders.mk)),\
	@make --no-print-directory __build_shaders module_type=Shaders mkfile=$@/module-shaders.mk,\
	@echo "Модуль $@ пропускается, файл проекта не найден.")

#################################################################################################
### Приватные промежуточные цели (не использовать напрямую).                                    #
#################################################################################################

.PHONY: __build_library __build_application __build_shaders

__build_library: $(__module_object_files)
	@$(__module_compiler_util) $(__module_common_flags) $(__module_linker_flags) $(__module_object_files) -o $(__binary_directory)$(__module_library_filename)
	@$(if $(strip $(__module_object_files)),\
	@echo "Сборка библиотеки $(__module_library_filename) завершена.",\
	@echo "Сборка библиотеки $(__module_library_filename) пропущена.")

__build_application: $(__module_object_files)
	@$(if $(strip $(__module_object_files)),\
	@$(__module_compiler_util) $(__module_common_flags) $(__module_linker_flags) $(__module_object_files) -o $(__binary_directory)$(__module_application_filename)\
	&& echo "Сборка приложения $(__module_application_filename) завершена.",\
	@echo "Сборка приложения $(__module_application_filename) пропущена.")

__build_shaders: $(__module_object_vert_files) $(__module_object_frag_files)
	@$(if $(strip $(__module_object_vert_files) $(__module_object_frag_files)),\
	@echo "Сборка шейдеров завершена.",\
	@echo "Сборка шейдеров пропущена.")

#################################################################################################
### Приватные конечные цели (не использовать напрямую).                                         #
#################################################################################################

$(__object_directory)$(__module_root_directory)%.o: $(__module_source_directory)%.c
	@$(__platform_mkdir_util) $(dir $@)
	@echo "Компиляция $<"
	@$(__module_compiler_util) $(__module_common_flags) $(__module_define_flags) $(__module_include_flags) $(__module_object_flags) -c $< -o $@

%.vert.spv: %.vert.glsl
	@echo "Компиляция $<"
	@$(__shader_compiler_util) $(__shader_vert_flags) $< -o $@

%.frag.spv: %.frag.glsl
	@echo "Компиляция $<"
	@$(__shader_compiler_util) $(__shader_frag_flags) $< -o $@
