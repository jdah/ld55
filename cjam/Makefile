print-%:; @echo $($*)

ifndef CONFIG
	$(error please define CONFIG)
endif

# config defines...
# - PATH_SRC
# - PATH_SHADER
# - PATH_ASSETS
# - PATH_BIN
# - (optional) RELEASE
# - (optional) SANITIZE
include $(CONFIG)

ifndef PATH_SRC
	$(error please define PATH_SRC)
endif

ifndef PATH_SHADER
	$(error please define PATH_SHADER)
endif

ifndef PATH_ASSETS
	$(error please define PATH_ASSETS)
endif

ifndef PATH_BIN
	$(error please define PATH_BIN)
endif

CJAM_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
BASE_DIR := $(realpath $(CJAM_DIR)/..)

UNAME = $(shell uname -s)

ifdef EMSCRIPTEN
	TARGET_PLATFORM = emscripten
	TARGET_ARCH = emscripten
endif

ifndef $(HOST_PLATFORM)
	ifeq ($(UNAME),Darwin)
	HOST_PLATFORM = macos
	HOST_ARCH = $(shell uname -m)
else ifeq ($(UNAME),Linux)
	HOST_PLATFORM = linux
	HOST_ARCH = x86_64 # TODO: assert
else
	HOST_PLATFORM = windows
	HOST_ARCH = x86_64 # TODO: assert
endif
endif

ifndef $(HOST_ARCH)
	ifeq ($(HOST_PLATFORM),macos)
	HOST_ARCH = $(shell uname -m)
else
	HOST_ARCH = x86_64 # TODO: assert
endif
endif

ifndef $(TARGET_PLATFORM)
	TARGET_PLATFORM = $(HOST_PLATFORM)
endif

ifndef $(TARGET_ARCH)
	TARGET_ARCH = $(HOST_ARCH)
endif

# library paths
PATH_LIB         = $(CJAM_DIR)/lib
PATH_SOLOUD      = $(PATH_LIB)/soloud
PATH_CGLM        = $(PATH_LIB)/cglm
PATH_SOKOL       = $(PATH_LIB)/sokol
PATH_SOKOL_TOOLS = $(PATH_LIB)/sokol-tools-bin

# paths/globs
SRC  	  = $(shell find $(PATH_SRC) -name "*.c")
OBJ 	  = $(SRC:%.c=$(PATH_BIN)/%.o)

# add soloud sources
SRC_SOLOUD  = $(shell find $(PATH_SOLOUD)/src/audiosource/wav -name "*.cpp")
SRC_SOLOUD += $(shell find $(PATH_SOLOUD)/src/backend/sdl2_static -name "*.cpp")
SRC_SOLOUD += $(shell find $(PATH_SOLOUD)/src/core -name "*.cpp")
SRC_SOLOUD += $(shell find $(PATH_SOLOUD)/src/c_api -name "*.cpp")
SRC_SOLOUD += $(shell find $(PATH_SOLOUD)/src/filter -name "*.cpp")

OBJ_SOLOUD += $(SRC_SOLOUD:$(CJAM_DIR)/%=$(PATH_BIN)/%.o)

DEP = $(SRC:%.c=$(PATH_BIN)/%.d)

OUT_NATIVE = $(PATH_BIN)/game
OUT_SHARED = $(PATH_BIN)/game.$(DYLIB)
OUT_WEB    = $(PATH_BIN)/index.html

SRC_SHADER = $(shell find $(PATH_SHADER) -name "*.glsl")
OUT_SHADER = $(SRC_SHADER:%.glsl=%.glsl.h)

# flags
CCFLAGS =
LDFLAGS =
EMCCFLAGS =

# determine compiler(s), platform flags
ifeq ($(HOST_PLATFORM),macos)
	CC   = LIBRARY_PATH=/opt/homebrew/lib $(shell brew --prefix llvm)/bin/clang
	LD   = LIBRARY_PATH=/opt/homebrew/lib $(shell brew --prefix llvm)/bin/clang
	DB   = $(shell brew --prefix llvm)/bin/lldb
	EMCC = emcc
	EMAR = emar

CCFLAGS += -I$(shell brew --prefix)/include
CCFLAGS += -I$(shell brew --prefix llvm)/include
LDFLAGS += -L$(shell brew --prefix llvm)/lib

DYLIB = dylib
else
	CC = clang
	LD = clang
	DB = lldb
	EMCC = emcc
	EMAR = emar
	DYLIB = so
endif

ifeq ($(HOST_PLATFORM),macos)
	ifeq ($(HOST_ARCH),arm64)
	SHDC = $(PATH_SOKOL_TOOLS)/bin/osx_arm64/sokol-shdc
else
	SHDC = $(PATH_SOKOL_TOOLS)/bin/osx/sokol-shdc
endif
else ifeq ($(HOST_PLATFORM),linux)
	SHDC = $(PATH_SOKOL_TOOLS)/bin/linux/sokol-shdc
else
	SHDC = $(PATH_SOKOL_TOOLS)/bin/win32/sokol-shdc.exe
endif

OBJ_CCFLAGS =

INCFLAGS = -iquote$(PATH_SRC) -iquote$(CJAM_DIR)

CCFLAGS += -std=gnu2x

ifdef RELEASE
	CCFLAGS += -O2
else
	CCFLAGS += -O0 -g -fno-omit-frame-pointer
	OBJ_CCFLAGS += -DRELOADHOST_CLIENT_ENABLED

ifdef SANITIZE
	CCFLAGS += -fsanitize=undefined,address -fno-sanitize=function
	LDFLAGS += -fsanitize=undefined,address -fno-sanitize=function
endif
endif

CCFLAGS += -Wall
CCFLAGS += -Wextra
CCFLAGS += -Wno-unused-parameter

# for soloud backend
CPPFLAGS += -DWITH_SDL2_STATIC -iquote$(PATH_SOLOUD)/include

LDFLAGS += -lm -lstdc++

ifeq ($(TARGET_PLATFORM),macos)
	LDFLAGS += -lSDL2
	LDFLAGS += -framework OpenGL
	LDFLAGS += -framework AudioUnit
	LDFLAGS += -framework AudioToolbox
else
	CCFLAGS += -D_GNU_SOURCE
	LDFLAGS += -lGL -lasound
endif

CCFLAGS += -DTARGET_PLATFORM_$(TARGET_PLATFORM)
CCFLAGS += -DTARGET_ARCH_$(TARGET_ARCH)

CPPFLAGS = -std=c++17 -O2 -fno-rtti -fno-exceptions -fPIC

ifndef RELEASE
	CPPFLAGS += -g -fno-omit-frame-pointer
endif

EMCCFLAGS += -std=gnu2x
EMCCFLAGS += -g1
EMCCFLAGS += -O2
EMCCFLAGS += -Wall
EMCCFLAGS += -Wextra
EMCCFLAGS += -Wno-unused-parameter
EMCCFLAGS += -Wno-missing-field-initializers
EMCCFLAGS += -Wno-gnu

# TODO: experiment with this/memory leaks
# EMCCLFAGS += -s INITIAL_MEMORY=256MB
# EMCCLFAGS += -s TOTAL_MEMORY=256MB
EMCCFLAGS += -s ALLOW_MEMORY_GROWTH
EMCCFLAGS += --preload-file $(PATH_ASSETS)
EMCCFLAGS += -s USE_SDL=2
EMCCFLAGS += -s FULL_ES3
EMCCFLAGS += -s ABORT_ON_WASM_EXCEPTIONS

ifdef RELEASE
	EMCCFLAGS += --shell-file $(CJAM_DIR)/emscripten/shell-release.html
else
	EMCCFLAGS += --shell-file $(CJAM_DIR)/emscripten/shell.html

ifdef SANITIZE
	EMCCFLAGS += -fsanitize=undefined,address,leak
endif
endif

all: dirs libs shaders build-native build-web build-shared

$(OUT_SHADER): %.glsl.h: %.glsl
	$(SHDC) --input $^ --output $@ --slang glsl300es:glsl330 --format=sokol_impl

shaders: $(OUT_SHADER)

-include $(DEP)

dirs: FORCE
	$(shell mkdir -p $(PATH_BIN))
	$(shell mkdir -p $(PATH_BIN)/$(PATH_SRC))
	$(shell mkdir -p $(PATH_BIN)/reloadhost)
	rsync -a --include '*/' --exclude '*' "$(PATH_LIB)" "$(PATH_BIN)"
	rsync -a --include '*/' --exclude '*' "$(PATH_SRC)" "$(PATH_BIN)"

$(OBJ_SOLOUD): $(PATH_BIN)/%.o: $(CJAM_DIR)/%
	$(CC) -c -o $@                    \
		-D WITH_SDL2_STATIC           \
		-iquote$(PATH_SOLOUD)/include \
		$(shell sdl2-config --cflags) \
		$<

soloud: dirs $(OBJ_SOLOUD)
	$(EMCC) -r -o $(PATH_BIN)/soloud.o \
		-s USE_SDL=2                   \
		-D WITH_SDL2_STATIC            \
		-s FULL_ES2=1                  \
		-iquote$(PATH_SOLOUD)/include  \
		$(SRC_SOLOUD)
	$(EMAR) rcs $(PATH_BIN)/soloud_emscripten.a $(PATH_BIN)/soloud.o
	ar rcs bin/soloud_native.a $(OBJ_SOLOUD)

$(OBJ): $(PATH_BIN)/%.o: %.c
	$(CC) -o $@ -MMD -c $(CCFLAGS) $(OBJ_CCFLAGS) $(INCFLAGS) $<

build-shared: dirs shaders reloadhost $(OBJ)
	$(LD) -shared -o $(OUT_SHARED) $(filter %.o,$^) $(PATH_BIN)/soloud_native.a $(LDFLAGS)

build-native: dirs shaders $(OBJ)
	$(LD) -o $(OUT_NATIVE) $(filter %.o,$^) $(PATH_BIN)/soloud_native.a $(LDFLAGS)

build-web: dirs shaders
	$(EMCC) -o $(OUT_WEB) -MMD $(EMCCFLAGS) $(INCFLAGS) $(PATH_BIN)/soloud_emscripten.a \
		-DTARGET_PLATFORM_emscripten \
		-DTARGET_ARCH_emscripten \
		$(SRC)

package-web: build-web
	cd $(PATH_BIN) && zip index.zip index.data index.html index.js index.wasm

reloadhost: dirs $(CJAM_DIR)/reloadhost/reloadhost.c
	$(CC) -o $(PATH_BIN)/reloadhost/reloadhost -MMD $(CCFLAGS) $(INCFLAGS) $(LDFLAGS) $(CJAM_DIR)/reloadhost/reloadhost.c

reloadhost-run: reloadhost build-shared
	$(PATH_BIN)/reloadhost/reloadhost $(OUT_SHARED)

reloadhost-debug: reloadhost build-shared
	$(DB) $(PATH_BIN)/reloadhost/reloadhost -o 'run $(OUT_SHARED)'

clean:
	find $(PATH_SHADER) -name "*.glsl.h" -type f -delete
	rm -rf $(DEP)
	rm -rf $(PATH_BIN)

FORCE: ;
