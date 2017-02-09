# This makefile builds the demo programs and their dependencies.
# 
# Condition for Windows taken from http://stackoverflow.com/a/12099167 by Trevor Robinson.
# make -p lists all defined variables and implicit rules. Useful to see what we start out with.
# $(warning $(VAR)) is usefull to output the value of a variable for debugging.



# Setup implicit rule to build object files
CC  = gcc
CFLAGS = -std=c99   -Ideps/include   -Werror -Wall -Wextra -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-parameter

# Choose between debug and release build. 
ifdef RELEASE
	CFLAGS += -O3 -s
	# Don't open a console window in the background for release builds. This sends all stdout and stderr output to oblivion.
	ifeq ($(OS),Windows_NT)
		CFLAGS += -mwindows
	endif
else
	CFLAGS += -g
endif



#
# Demo applications, object files are created by implicit rules
#
DEMOS = $(patsubst %.c,%,$(wildcard *.c))
all: $(DEMOS)

$(DEMOS): deps/libSDL2.a $(PLATFORM_DEPS)
wireframe framebuffer cube: ../math_3d.h

# Clean all files in the .gitignore list, ensures that the ignore file
# is properly maintained.
clean:
	cat .gitignore | xargs rm -fr



#
# Fetch SDL2 library
#
ifeq ($(OS),Windows_NT)

# On Windows download the MinGW development files and extract the static libraries and headers
# from it. The basic utilities in the win directory are taken from http://unxutils.sourceforge.net/.
# Old but probably sufficient.

# Add the windows directory to the PATH so we can use the basic UNIX utilities there.
export PATH := .\windows;$(PATH)

# Compiler flags for SDL
CFLAGS += -Dmain=SDL_main
LDLIBS += -Ldeps -lmingw32 -lSDL2main -lSDL2 -Wl,--no-undefined -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -luuid -static-libgcc -lopengl32
PLATFORM_DEPS = windows/gl_3_1_core.o

deps/libSDL2-devel.tar.gz:
	wget http://libsdl.org/release/SDL2-devel-2.0.3-mingw.tar.gz -O deps/libSDL2-devel.tar.gz

# tar --ungzip doesn't work. The zcat/tar combination by akira is taken from http://superuser.com/a/124728
# mkdir is an CMD buildin and needs a backslash path without -p to work properly.
deps/libSDL2.a: deps/libSDL2-devel.tar.gz
	zcat deps/libSDL2-devel.tar.gz | tar --extract --directory deps
	mv deps/SDL* deps/SDL2
	mv deps/SDL2/x86_64-w64-mingw32/lib/libSDL2.a deps/SDL2/x86_64-w64-mingw32/lib/libSDL2main.a deps
	mkdir deps\include
	mv deps/SDL2/include deps/include/SDL
	rm -rf deps/SDL2
	touch deps/libSDL2.a

else

# On Linux download and build SDL2 as static library.

# Compiler flags for SDL
CFLAGS += `pkg-config --cflags gl` -pthread
LDLIBS += `pkg-config --libs gl` -ldl -lrt -lm
PLATFORM_DEPS = 

deps/libSDL2.tar.gz:
	wget http://libsdl.org/release/SDL2-2.0.3.tar.gz -O deps/libSDL2.tar.gz

deps/libSDL2.a: deps/libSDL2.tar.gz
	cd deps;       tar -xaf libSDL2.tar.gz
	mv deps/SDL*   deps/SDL2
	cd deps/SDL2;  ./configure --disable-shared
	cd deps/SDL2;  make -j
	mv deps/SDL2/build/.libs/libSDL2.a deps/
	mkdir -p deps/include
	mv deps/SDL2/include deps/include/SDL
	rm -rf deps/SDL2

endif