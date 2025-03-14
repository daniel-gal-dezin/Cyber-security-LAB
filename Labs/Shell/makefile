BIN_DIR := bin
OBJ_DIR := obj

all: $(BIN_DIR)/myshell $(BIN_DIR)/Looper $(BIN_DIR)/mypipe $(BIN_DIR)/mypipeline

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $(BIN_DIR) $(OBJ_DIR)

$(BIN_DIR)/myshell: $(OBJ_DIR)/myshell.o $(OBJ_DIR)/LineParser.o | $(BIN_DIR)
	gcc -g -Wall -o $(BIN_DIR)/myshell $(OBJ_DIR)/myshell.o $(OBJ_DIR)/LineParser.o

$(OBJ_DIR)/myshell.o: myshell.c | $(OBJ_DIR)
	gcc -g -Wall -c -o $(OBJ_DIR)/myshell.o myshell.c

$(OBJ_DIR)/LineParser.o: LineParser.c LineParser.h | $(OBJ_DIR)
	gcc -g -Wall -c -o $(OBJ_DIR)/LineParser.o LineParser.c

$(OBJ_DIR)/Looper.o: Looper.c | $(OBJ_DIR)
	gcc -g -Wall -c -o $(OBJ_DIR)/Looper.o Looper.c

$(BIN_DIR)/Looper: $(OBJ_DIR)/Looper.o | $(BIN_DIR)
	gcc -g -Wall -o $(BIN_DIR)/Looper $(OBJ_DIR)/Looper.o

$(OBJ_DIR)/mypipe.o: mypipe.c | $(OBJ_DIR)
	gcc -g -Wall -c -o $(OBJ_DIR)/mypipe.o mypipe.c

$(BIN_DIR)/mypipe: $(OBJ_DIR)/mypipe.o | $(BIN_DIR)
	gcc -g -Wall -o $(BIN_DIR)/mypipe $(OBJ_DIR)/mypipe.o

$(OBJ_DIR)/mypipeline.o: mypipeline.c | $(OBJ_DIR)
	gcc -g -Wall -c -o $(OBJ_DIR)/mypipeline.o mypipeline.c

$(BIN_DIR)/mypipeline: $(OBJ_DIR)/mypipeline.o | $(BIN_DIR)
	gcc -g -Wall -o $(BIN_DIR)/mypipeline $(OBJ_DIR)/mypipeline.o

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)


