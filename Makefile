# ============================================================
#  Makefile — gold-smc
#
#  Convenience wrapper around the CMake build.
#  CMakeLists.txt remains the source of truth for
#  compiler flags, dependencies, and targets.
#
#  Requires a POSIX shell — works out of the box in:
#    * MSYS2 (UCRT64 / MINGW64)
#    * Git Bash
#    * WSL
#    * Linux / macOS
#
#  On plain cmd.exe, use build.bat instead
#  (mingw32-make alone is not enough — the recipes below
#   need sh, rm, nproc, mkdir -p).
#
#  Quick reference:
#    make             configure (if needed) + build + test   [Release]
#    make build       configure (if needed) + build only
#    make test        run ctest
#    make run         build + run gold_smc.exe once
#    make clean       remove build dir
#    make rebuild     clean + build + test
#    make debug       Debug build (separate build-debug/ dir)
#    make release     Release build (build/ dir)
#    make dist        stage the static exe into dist/
#    make help        show this
#
#  Overridable variables (env or CLI):
#    make MINGW_BIN=C:/msys64/ucrt64/bin
#    make VCPKG_ROOT=C:/tools/vcpkg
#    make JOBS=8
#    make BUILD_DIR=build-custom
# ============================================================

# ---------- user-configurable ----------
MINGW_BIN  ?= C:/mingw64/bin
VCPKG_ROOT ?= C:/dev/vcpkg
GENERATOR  ?= Ninja
BUILD_TYPE ?= Release
JOBS       ?= $(shell nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)

# Keep Debug and Release caches separate so `make debug` doesn't
# silently reconfigure an existing Release cache (and vice versa).
ifeq ($(BUILD_TYPE),Debug)
    BUILD_DIR ?= build-debug
else
    BUILD_DIR ?= build
endif

# ---------- derived ----------
GXX       := $(MINGW_BIN)/g++.exe
TOOLCHAIN := $(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake
CACHE     := $(BUILD_DIR)/CMakeCache.txt
BINARY    := $(BUILD_DIR)/gold_smc.exe

CMAKE_ARGS := -G "$(GENERATOR)" \
              -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
              -DCMAKE_CXX_COMPILER="$(GXX)" \
              -DCMAKE_TOOLCHAIN_FILE="$(TOOLCHAIN)" \
              -DVCPKG_TARGET_TRIPLET=x64-mingw-static \
              -DVCPKG_HOST_TRIPLET=x64-mingw-static

# ---------- make behavior ----------
.DEFAULT_GOAL := all
.DELETE_ON_ERROR:
.PHONY: all help check-tools configure build test run clean rebuild debug release dist

# ---------- default: build + test ----------
all: build test
	@echo ""
	@echo "============================================================"
	@echo "  Build succeeded: $(BINARY)"
	@echo "============================================================"

# ---------- help ----------
help:
	@echo "gold-smc Makefile — targets:"
	@echo ""
	@echo "  make           Configure (if needed) + build + test   [default]"
	@echo "  make configure Force-run the CMake configure step"
	@echo "  make build     Configure (if needed) + build only"
	@echo "  make test      Run ctest"
	@echo "  make run       Build + run $(BINARY)"
	@echo "  make clean     Delete $(BUILD_DIR)/"
	@echo "  make rebuild   clean + build + test"
	@echo "  make debug     Same as 'make', but Debug into build-debug/"
	@echo "  make release   Same as 'make', but explicit Release"
	@echo "  make dist      Stage $(BINARY) into dist/"
	@echo ""
	@echo "Variables (override on CLI or via env):"
	@echo "  MINGW_BIN    default $(MINGW_BIN)"
	@echo "  VCPKG_ROOT   default $(VCPKG_ROOT)"
	@echo "  BUILD_DIR    default $(BUILD_DIR)"
	@echo "  BUILD_TYPE   default $(BUILD_TYPE)"
	@echo "  GENERATOR    default $(GENERATOR)"
	@echo "  JOBS         default $(JOBS)"

# ---------- prerequisites ----------
check-tools:
	@test -x "$(GXX)"       || { echo "ERROR: g++ not found at $(GXX)";         echo "  Fix: make MINGW_BIN=/path/to/mingw/bin"; exit 1; }
	@test -f "$(TOOLCHAIN)" || { echo "ERROR: vcpkg toolchain not found at $(TOOLCHAIN)"; echo "  Fix: make VCPKG_ROOT=/path/to/vcpkg"; exit 1; }
	@command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake not on PATH"; exit 1; }
	@command -v ctest >/dev/null 2>&1 || { echo "ERROR: ctest not on PATH"; exit 1; }

# ---------- configure (idempotent) ----------
configure: check-tools
	@if [ ! -f "$(CACHE)" ]; then \
		echo ">>> [1/3] Configuring $(BUILD_TYPE) into $(BUILD_DIR)/ ..."; \
		cmake -B "$(BUILD_DIR)" -S . $(CMAKE_ARGS); \
	else \
		echo ">>> [1/3] $(BUILD_DIR)/ already configured (reusing)."; \
		echo "        Fresh configure: make clean && make"; \
	fi

# ---------- build ----------
build: configure
	@echo ">>> [2/3] Building ($(BUILD_TYPE), -j$(JOBS)) ..."
	@cmake --build "$(BUILD_DIR)" --config $(BUILD_TYPE) -j $(JOBS)

# ---------- test ----------
test: build
	@echo ">>> [3/3] Running tests ..."
	@ctest --test-dir "$(BUILD_DIR)" -C $(BUILD_TYPE) --output-on-failure

# ---------- run ----------
run: build
	@echo ""
	@echo ">>> Running $(BINARY) ..."
	@$(BINARY)

# ---------- clean ----------
clean:
	@echo ">>> Removing $(BUILD_DIR)/ ..."
	@rm -rf "$(BUILD_DIR)"

# ---------- rebuild ----------
rebuild: clean all

# ---------- shortcuts for build type ----------
debug:
	@$(MAKE) --no-print-directory BUILD_TYPE=Debug
release:
	@$(MAKE) --no-print-directory BUILD_TYPE=Release

# ---------- stage distributable ----------
dist: build
	@echo ">>> Staging static binary into dist/ ..."
	@mkdir -p dist
	@cp -f "$(BINARY)" dist/
	@echo "    dist/gold_smc.exe ready — static build, no DLLs needed."