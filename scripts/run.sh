#!/usr/bin/env bash
#
# Сборка и запуск WorkWatcher.
#
# Обёртка над Makefile для повседневного использования: собирает нужную
# конфигурацию и сразу запускает приложение.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BINARY="$REPO_ROOT/build/workwatcher"

usage() {
    cat <<'EOF'
Сборка и запуск WorkWatcher.

Использование:
  scripts/run.sh            собрать (при необходимости) и запустить
  scripts/run.sh --clean    очистить и собрать заново
  scripts/run.sh --debug    отладочная сборка (-O0 -g3), затем запуск
  scripts/run.sh --help     эта справка

Управление в приложении:
  любая клавиша / кнопка мыши  сменить режим
  Ctrl+Q                        завершить работу
EOF
}

log() {
    printf '==> %s\n' "$*"
}

die() {
    printf 'Ошибка: %s\n' "$*" >&2
    exit 1
}

clean=0
build_type=release

while [ "$#" -gt 0 ]; do
    case "$1" in
        -c|--clean) clean=1 ;;
        -d|--debug) build_type=debug ;;
        -h|--help)  usage; exit 0 ;;
        *)          printf 'Неизвестный аргумент: %s\n\n' "$1" >&2; usage >&2; exit 1 ;;
    esac
    shift
done

cd "$REPO_ROOT"

if [ "$clean" -eq 1 ]; then
    log "make clean"
    make clean
fi

log "make BUILD_TYPE=$build_type"
make BUILD_TYPE="$build_type" all

[ -x "$BINARY" ] || die "исполняемый файл не найден: $BINARY"

log "запуск $BINARY"
exec "$BINARY"
