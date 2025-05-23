CC = gcc
CFLAGS = -Wall
SRC = ppos-core-aux.c pingpong-preempcao.c ppos-all.o queue.o
TARGET = ppos

all: $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

run:
	./$(TARGET)
