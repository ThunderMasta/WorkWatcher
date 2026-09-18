#!/bin/bash
#
# Запуск WorkWatcher двойным щелчком мыши (macOS).
#
# Файлы с расширением .command открываются в Terminal при двойном клике из
# Finder. Этот файл — обёртка над scripts/run.sh: переходит в каталог
# проекта, собирает его и запускает, а после завершения НЕ закрывает окно
# сразу, чтобы можно было прочитать сообщения.

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

# Заголовок окна терминала и очистка экрана.
printf '\033]0;WorkWatcher\007'
clear

echo "WorkWatcher — запуск из Finder"
echo "Каталог: $SCRIPT_DIR"
echo

bash "$SCRIPT_DIR/scripts/run.sh" "$@"
status=$?

echo
if [ "$status" -ne 0 ]; then
    echo "WorkWatcher завершён с ошибкой (код $status)."
else
    echo "WorkWatcher завершён."
fi

echo "Нажмите Enter, чтобы закрыть окно..."
read -r _ || true

exit "$status"
