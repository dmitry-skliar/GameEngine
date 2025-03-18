#pragma once

#include <defines.h>
#include <math/math_types.h>

// TODO: FOV и scissor.
// TODO: Инкапсулировать данные и закрыть доступ к ним.
// @brief Представляет собой камеру (Не читать и не писать данные камеры напрямую).
typedef struct camera {
    // @brief Позиция камеры.
    vec3 position;
    // @brief Повороты камеры (используеются углы Эйлера).
    vec3 euler_rotation;
    // @brief Указывает на необходимость обновить матрицу вида.
    bool is_dirty;
    // @brief Матрица вида камеры.
    mat4 view_matrix;
} camera;

/*
    @brief Создает новую камеру со позицией и вращением по умолчанию.
    @return Возвращает новую камеру.
*/
KAPI camera camera_create();

/*
    @brief Сбрасывает позицию и вращение камеры на значения по умолчанию.
    @param c Указатель на камеру для сброса настроек.
*/
KAPI void camera_reset(camera* c);

/*
    @brief Получение позиции камеры.
    @param c Указатель на камеру для получение позиции.
    @return Позиция камеры.
*/
KAPI vec3 camera_position_get(const camera* c);

/*
    @brief Устанавливает позицию предоставленной камеры.
    @param c Указатель на камеру для установеи позиции.
    @param position Новая позиция камеры.
*/
KAPI void camera_position_set(camera* c, vec3 position);

/*
    @brief Получение вращений камеры в углах Эйлера.
    @param c Указатель на камеру для получение вращений.
    @return Вращения камеры в углах Эйлера.
*/
KAPI vec3 camera_rotation_euler_get(const camera* c);

/*
    @brief Устанавливает вращения предоставленной камеры в углах Эйлера.
    @param c Указатель на камеру для установеи вращений.
    @param rotation Новые вращения камеры в углах Эйлера.
*/
KAPI void camera_rotation_euler_set(camera* c, vec3 rotation);

/*
    @brief Получение матрицы вида камеры.
    @param c Указатель на камеру для получения матрицы вида.
    @return Матрица вида камеры.
*/
mat4 camera_view_get(camera* c);

/*
    @brief Перемешает камеру вперед на заданное значение.
    @param c Указатель на камеру для перемещения.
    @param amount Значение для перемещения.
*/
KAPI void camera_move_forward(camera* c, f32 amount);

/*
    @brief Перемешает камеру назад на заданное значение.
    @param c Указатель на камеру для перемещения.
    @param amount Значение для перемещения.
*/
KAPI void camera_move_backward(camera* c, f32 amount);

/*
    @brief Перемешает камеру влево на заданное значение.
    @param c Указатель на камеру для перемещения.
    @param amount Значение для перемещения.
*/
KAPI void camera_move_left(camera* c, f32 amount);

/*
    @brief Перемешает камеру вправо на заданное значение.
    @param c Указатель на камеру для перемещения.
    @param amount Значение для перемещения.
*/
KAPI void camera_move_right(camera* c, f32 amount);

/*
    @brief Перемешает камеру вверх на заданное значение.
    @param c Указатель на камеру для перемещения.
    @param amount Значение для перемещения.
*/
KAPI void camera_move_up(camera* c, f32 amount);

/*
    @brief Перемешает камеру вниз на заданное значение.
    @param c Указатель на камеру для перемещения.
    @param amount Значение для перемещения.
*/
KAPI void camera_move_down(camera* c, f32 amount);

/*
    @brief Вращает камеру по оси Y (по горизонтали; рыскание) на заданное значение.
    @param c Указатель на камеру для вращения.
    @param amount Значение для поворота.
*/
KAPI void camera_yaw(camera* c, f32 amount);

/*
    @brief Вращает камеру по оси X (по вертикали; тангаж) на заданное значение.
    @param c Указатель на камеру для вращения.
    @param amount Значение для поворота.
*/
KAPI void camera_pitch(camera* c, f32 amount);

/*
    @brief Вращает камеру по оси Z (переворот; крен) на заданное значение.
    @param c Указатель на камеру для вращения.
    @param amount Значение для поворота.
*/
KAPI void camera_roll(camera* c, f32 amount);

/*
    @brief Вращает камеру обычным способом по горизонтали.
    @param c Указатель на камеру для вращения.
    @param amount Значение для поворота.
*/
KAPI void camera_move_horizontal(camera* c, f32 amount);

/*
    @brief Вращает камеру обычным способом по вертикали.
    @param c Указатель на камеру для вращения.
    @param amount Значение для поворота.
*/
KAPI void camera_move_vertical(camera* c, f32 amount);
