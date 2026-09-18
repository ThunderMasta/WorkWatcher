# =============================================================================
# WorkWatcher — сборка, тесты, проверки качества.
#
# Основные цели:  make            собрать (release)
#                 make test       собрать и прогнать юнит-тесты
#                 make help       список всех целей
#
# Переменные:     BUILD_TYPE=release|debug   тип сборки (по умолчанию release)
#                 SANITIZE=1                 включить ASan + UBSan
#                 WERROR=1                   считать предупреждения ошибками
#                 CC=clang                   компилятор
#                 V=1                        подробный вывод команд
# =============================================================================

PROJECT   := workwatcher
SRC_DIR   := src
INC_DIR   := include
TEST_DIR  := tests
BUILD_DIR ?= build

BUILD_TYPE ?= release
SANITIZE   ?= 0
WERROR     ?= 0
V          ?= 0

CC           ?= cc
# Требуется clang-format 21.x: 18.x и 21.x по-разному обрабатывают выравнивание
# хвостовых комментариев с не-ASCII текстом (см. CONTRIBUTING.md).
CLANG_FORMAT ?= clang-format
CLANG_TIDY   ?= clang-tidy

# --- Флаги компиляции --------------------------------------------------------

CSTD     := -std=c11
WARNINGS := -Wall -Wextra -Wpedantic \
            -Wshadow -Wconversion -Wsign-conversion \
            -Wstrict-prototypes -Wmissing-prototypes \
            -Wcast-qual -Wundef -Wformat=2 -Wvla
CPPFLAGS += -I$(INC_DIR) -D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE

ifeq ($(BUILD_TYPE),release)
  OPTFLAGS := -O2 -DNDEBUG
else ifeq ($(BUILD_TYPE),debug)
  OPTFLAGS := -O0 -g3 -fno-omit-frame-pointer
else
  $(error Неизвестный BUILD_TYPE='$(BUILD_TYPE)'; допустимо: release, debug)
endif

ifeq ($(SANITIZE),1)
  SANFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer -g
endif

ifeq ($(WERROR),1)
  WARNINGS += -Werror
endif

CFLAGS  += $(CSTD) $(WARNINGS) $(OPTFLAGS) $(SANFLAGS)
LDFLAGS += $(SANFLAGS)
LDLIBS  ?=

# clang-tidy, установленный не из Xcode, не знает путь к SDK macOS.
ifeq ($(shell uname -s),Darwin)
  LINT_SYSROOT := -isysroot $(shell xcrun --show-sdk-path 2>/dev/null)
endif

# --- Файлы -------------------------------------------------------------------

OBJ_DIR := $(BUILD_DIR)/obj
BIN     := $(BUILD_DIR)/$(PROJECT)

SRCS     := $(wildcard $(SRC_DIR)/*.c)
OBJS     := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
LIB_OBJS := $(filter-out $(OBJ_DIR)/main.o,$(OBJS))

TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS := $(TEST_SRCS:$(TEST_DIR)/%.c=$(OBJ_DIR)/tests/%.o)
TEST_BIN  := $(BUILD_DIR)/$(PROJECT)_tests

HEADERS := $(wildcard $(INC_DIR)/$(PROJECT)/*.h) $(wildcard $(TEST_DIR)/*.h)
DEPS    := $(OBJS:.o=.d) $(TEST_OBJS:.o=.d)

ifeq ($(V),1)
  Q :=
else
  Q := @
endif

# --- Цели --------------------------------------------------------------------

.PHONY: all test run debug sanitize check format format-check lint clean help

all: $(BIN) ## Собрать приложение

$(BIN): $(OBJS)
	@echo "  LD      $@"
	$(Q)$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/tests/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(TEST_BIN): $(LIB_OBJS) $(TEST_OBJS)
	@echo "  LD      $@"
	$(Q)$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test: $(TEST_BIN) ## Собрать и запустить юнит-тесты
	@echo "  RUN     $<"
	$(Q)$<

run: $(BIN) ## Собрать и запустить приложение
	$(Q)$<

debug: ## Отладочная сборка (-O0 -g3)
	$(Q)$(MAKE) --no-print-directory BUILD_TYPE=debug all

sanitize: ## Тесты под ASan/UBSan
	$(Q)$(MAKE) --no-print-directory BUILD_TYPE=debug SANITIZE=1 BUILD_DIR=$(BUILD_DIR)/sanitize test

check: ## Полная проверка: сборка с -Werror, тесты, санитайзеры, формат
	$(Q)$(MAKE) --no-print-directory WERROR=1 all test
	$(Q)$(MAKE) --no-print-directory sanitize
	$(Q)$(MAKE) --no-print-directory format-check

format: ## Отформатировать исходники (clang-format)
	$(Q)$(CLANG_FORMAT) -i $(SRCS) $(TEST_SRCS) $(HEADERS)

format-check: ## Проверить форматирование без изменений
	$(Q)$(CLANG_FORMAT) --dry-run --Werror $(SRCS) $(TEST_SRCS) $(HEADERS)

lint: ## Статический анализ (clang-tidy)
	$(Q)$(CLANG_TIDY) --quiet $(SRCS) -- $(CPPFLAGS) $(CSTD) $(LINT_SYSROOT)

clean: ## Удалить артефакты сборки
	$(Q)rm -rf $(BUILD_DIR)

help: ## Показать эту справку
	@grep -hE '^[a-zA-Z_-]+:.*?## ' $(MAKEFILE_LIST) | \
	  awk 'BEGIN {FS = ":.*?## "}; {printf "  %-14s %s\n", $$1, $$2}'

-include $(DEPS)
