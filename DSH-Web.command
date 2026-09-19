#!/bin/bash
#
# Запуск DSH Web (веб-интерфейс DeepSeek Harness) двойным щелчком мыши (macOS).
#
# Файлы с расширением .command открываются в Terminal при двойном клике из
# Finder. Этот файл запускает:
#
#     npx @deepseek-ai/dsh web
#
# из каталога проекта. Любые дополнительные аргументы (например порт) можно
# передать, вызвав файл из терминала: ./DSH-Web.command --port 3080
# После завершения окно НЕ закрывается сразу, чтобы можно было прочитать вывод.

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

# Заголовок окна терминала и очистка экрана.
printf '\033]0;DSH Web\007'
clear

echo "DSH Web — запуск из Finder"
echo "Каталог: $SCRIPT_DIR"
echo

# Проверяем наличие Node.js/npx, иначе даём понятное сообщение вместо
# невнятной ошибки "command not found".
if ! command -v npx >/dev/null 2>&1; then
    echo "Ошибка: npx не найден в PATH."
    echo "Установите Node.js (https://nodejs.org) и повторите запуск."
    echo
    echo "Нажмите Enter, чтобы закрыть окно..."
    read -r _ || true
    exit 127
fi

echo "Команда: npx @deepseek-ai/dsh web $*"
echo "----------------------------------------------------------------"
echo

npx @deepseek-ai/dsh web "$@"
status=$?

echo
echo "----------------------------------------------------------------"
if [ "$status" -ne 0 ]; then
    echo "DSH Web завершён с ошибкой (код $status)."
else
    echo "DSH Web завершён."
fi

echo "Нажмите Enter, чтобы закрыть окно..."
read -r _ || true

exit "$status"
