CC = gcc
LANG = c
STD = c99
OBJ_DIR = objs/
SRC_DIR = src/
OBJS = $(OBJ_DIR)arbol.o

CFLAGS= -x $(LANG) --std=$(STD) -Wall -Wextra -O3
PROG_NAME = arbol

$(PROG_NAME) : $(OBJS)
	@$(CC) -o $(PROG_NAME) $(OBJS) $(CFLAGS)

$(OBJ_DIR)arbol.o : $(SRC_DIR)arbol.c
	@$(CC) $(CFLAGS) -c $(SRC_DIR)arbol.c -o $(OBJ_DIR)arbol.o

run:
	./$(PROG_NAME)

.PHONY : clean
clean :
	rm $(PROG_NAME) $(OBJS)
