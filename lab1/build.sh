#!/bin/bash

# Компиляция с флагами
g++ -Wall -Werror daemon.cpp -o daemon

# Проверка, что компиляция прошла успешно
if [ $? -eq 0 ]; then
    echo "Сборка успешна!"
else
    echo "Ошибка сборки"
    exit 1
fi
