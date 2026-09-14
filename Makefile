CC      ?= cc
CFLAGS  ?= -Wall -Wextra -Wpedantic -std=c11 -O2
LDFLAGS ?=
LDLIBS  ?=

TARGET  := workwatcher
SRCS    := main.c terminal.c timer.c ui.c
OBJS    := $(SRCS:.c=.o)
DEPS    := $(OBJS:.o=.d)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)
