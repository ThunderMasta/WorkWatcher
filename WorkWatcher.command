#!/bin/bash
#
# Запуск WorkWatcher двойным щелчком (macOS Finder / Terminal.app).
#
# Без предисловий и без «нажмите Enter»: штатный выход скрывает окно
# из самого workwatcher (Apple Terminal).
#
set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_TYPE=release

fail() {
    echo
    echo "Ошибка: $1" >&2
    if [ -n "${2:-}" ]; then
        echo "$2" >&2
    fi
    echo
    echo "Нажмите Enter, чтобы закрыть окно..."
    read -r _ || true
    exit "${3:-1}"
}

printf '\033]0;WorkWatcher\007'
printf '\033[H\033[2J\033[3J'

cd "$SCRIPT_DIR" || fail "не удалось перейти в $SCRIPT_DIR"

if ! command -v make > /dev/null 2>&1; then
    fail "make не найден в PATH." \
        "Установите инструменты разработчика: xcode-select --install" 127
fi

if ! make BUILD_TYPE="$BUILD_TYPE" all > /dev/null 2>&1; then
    fail "сборка не удалась." "Запустите вручную: make BUILD_TYPE=$BUILD_TYPE all"
fi

BINARY="$SCRIPT_DIR/build/workwatcher"
if [ ! -x "$BINARY" ]; then
    fail "исполняемый файл не найден: $BINARY"
fi

exec "$BINARY"
