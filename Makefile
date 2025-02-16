CC             = gcc
COMPILER_FLAGS = -Wall -Wfatal-errors
SRC_FILES      = *.c
OBJ_NAME       = prog
INCLUDE_PATH   = -I/usr/local/include
LINKER_FLAGS   = -L/usr/local/lib -Wl,-rpath,/usr/local/lib -Wl,--enable-new-dtags -lSDL3

# include_path and linker_flags copied from `pkg-config sdl3 --cflags --libs`

build:
	$(CC) $(COMPILER_FLAGS) $(SRC_FILES) $(INCLUDE_PATH) $(LINKER_FLAGS) -o $(OBJ_NAME)

run:
	./$(OBJ_NAME)

clean:
	rm ./$(OBJ_NAME)