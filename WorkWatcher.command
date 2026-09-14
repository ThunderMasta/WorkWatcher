#!/bin/bash
#
# Запуск WorkWatcher двойным щелчком мыши.
#
# На macOS файлы с расширением .command открываются в Terminal при
# двойном клике. Этот файл — обёртка над run.sh: переходит в свой
# каталог, собирает проект и запускает его, а после завершения НЕ
# закрывает окно сразу, чтобы можно было прочитать сообщения.

set -u

# Перейти в каталог, где лежит этот файл (независимо от того, откуда запущен).
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

# Заголовок окна терминала и очистка экрана.
printf '\033]0;WorkWatcher\007'
clear

echo "WorkWatcher — запуск из Finder"
echo "Каталог: $SCRIPT_DIR"
echo

# Собрать и запустить (передаём аргументы: --clean / --debug / --help).
bash "$SCRIPT_DIR/run.sh" "$@"
status=$?

echo
if [ "$status" -ne 0 ]; then
    echo "WorkWatcher завершён с ошибкой (код $status)."
else
    echo "WorkWatcher завершён."
fi

# Не закрывать окно сразу — дать прочитать вывод / ошибки.
echo "Нажмите Enter, чтобы закрыть окно..."
read -r _ || true

exit "$status"
