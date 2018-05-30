DEMO_SDL := demo-sdl
DEMO_WAV := demo-wav

ifeq ($(OS), Windows_NT)
    LDFLAGS := -lmingw32
    DEMO_SDL := $(DEMO_SDL).exe
    DEMO_WAV := $(DEMO_WAV).exe
endif

.PHONY: help
help:
	@ echo "choose one:"
	@ echo "  'make demo-sdl' to build the SDL2-based demo"
	@ echo "  'make demo-wav' to build the MOD to WAV demo"
	@ echo "  'make clean' to remove build artifacts"

.PHONY: clean
clean:
	$(RM) $(DEMO_SDL)
	$(RM) $(DEMO_WAV)

demo-sdl: demo-sdl.c pocketmod.h
	$(CC) $(filter %.c, $^) -o $@ $(LDFLAGS) -lSDL2main -lSDL2

demo-wav: demo-wav.c pocketmod.h
	$(CC) $(filter %.c, $^) -o $@
