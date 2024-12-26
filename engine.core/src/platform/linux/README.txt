Для WAYLAND:

1. Файлы сгенерированы следующими командами:
    wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml wayland_xdg.h
    wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml wayland_xdg.c

2. После чего внесены изменения в 'wayland_xdg.c' для компиляции его только тогда, когда используется
   флаг KPLATFORM_LINUX_WAYLAND_FLAG.
