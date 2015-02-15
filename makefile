TARGET = scsidevinfo
OBJ = main.o command.o
CC = gcc

$(TARGET) : $(OBJ)
	$(CC) -o $(TARGET) $(OBJ)

main.o : main.c command.c
	$(CC) $(CFLAGS) -c main.c

command.o : command.c command.h
	$(CC) $(CFLAGS) -c command.c

clean:
	rm $(TARGET) $(OBJ)
