#!/usr/bin/env bash
#
# Сборка и запуск WorkWatcher.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

usage() {
    cat <<'EOF'
Сборка и запуск WorkWatcher.

Использование:
  ./run.sh            собрать (при необходимости) и запустить
  ./run.sh --clean    очистить и собрать заново
  ./run.sh --debug    отладочная сборка (-g -O0), затем запуск
  ./run.sh --help     эта справка
EOF
}

clean=0
debug=0

while [ "$#" -gt 0 ]; do
    case "$1" in
        -c|--clean) clean=1 ;;
        -d|--debug) debug=1 ;;
        -h|--help)  usage; exit 0 ;;
        *) echo "Неизвестный аргумент: $1" >&2; usage >&2; exit 1 ;;
    esac
    shift
done

if [ "$clean" -eq 1 ]; then
    echo "==> make clean"
    make clean
fi

echo "==> make"
if [ "$debug" -eq 1 ]; then
    make CFLAGS="-Wall -Wextra -Wpedantic -std=c11 -g -O0"
else
    make
fi

if [ ! -x ./workwatcher ]; then
    echo "Ошибка: исполняемый файл ./workwatcher не найден" >&2
    exit 1
fi

echo "==> запуск ./workwatcher (нажатие любой клавиши переключает фазу, Ctrl+D/Ctrl+\\ для выхода)"
exec ./workwatcher
