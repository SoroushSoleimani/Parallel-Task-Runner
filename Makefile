CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
TARGET = task_executor
SRCS = src/main.c src/process_pool.c src/log_manager.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET) 3 commands.txt

.PHONY: all clean run