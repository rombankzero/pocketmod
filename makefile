ifeq ($(OS), Windows_NT)
    LDFLAGS := -lmingw32
endif

demo-sdl: demo-sdl.c pocketmod.h
	$(CC) $(filter %.c, $^) -o $@ $(LDFLAGS) -lSDL2main -lSDL2

.PHONY: clean
clean:
	$(RM) demo-sdl demo-sdl.exe
