# VEXcode makefile 2019_03_26_01

# show compiler output
VERBOSE = 0

# include toolchain options
include vex/mkenv.mk

BUILD_MODE := normal
ifneq ($(findstring AI_VISION_TEST,$(CXX_FLAGS) $(CFLAGS)),)
BUILD_MODE := vision-test
endif
ifneq ($(findstring AUTON_SELECTION_TEST,$(CXX_FLAGS) $(CFLAGS)),)
BUILD_MODE := $(BUILD_MODE)-selection-test
endif
ifneq ($(BUILD_MODE),normal)
ifeq ($(abspath $(BUILD)),$(abspath build))
$(error Hardware test builds require a separate BUILD directory; normal build/ is reserved for competition code)
endif
endif

# location of the project source cpp and c files
SRC_C  = $(wildcard src/*.cpp) 
SRC_C += $(wildcard src/*.c)
SRC_C += $(wildcard src/*/*.cpp) 
SRC_C += $(wildcard src/*/*.c)

OBJ = $(addprefix $(BUILD)/, $(addsuffix .o, $(basename $(SRC_C))) )

# location of include files that c and cpp files depend on
SRC_H  = $(wildcard include/*.h)

# additional dependancies
SRC_A  = makefile

# project header file locations
INC_F  = include

# build targets
all: $(BUILD)/$(PROJECT).bin

# include build rules
include vex/mkrules.mk

.PHONY: force-build-mode
force-build-mode:

$(BUILD)/.build-mode: force-build-mode
	@$(MKDIR)
ifeq ($(OS),Windows_NT)
	@echo $(BUILD_MODE)>"$@.tmp"
	@fc /b "$@" "$@.tmp" >nul 2>&1 || copy /y "$@.tmp" "$@" >nul
	@del "$@.tmp"
else
	@printf '%s\n' '$(BUILD_MODE)' | cmp -s - "$@" || printf '%s\n' '$(BUILD_MODE)' > "$@"
endif

$(OBJ): $(BUILD)/.build-mode

ifeq ($(OS),Windows_NT)
PREVIOUS_BUILD_MODE := $(shell type "$(BUILD)/.build-mode" 2>nul)
else
PREVIOUS_BUILD_MODE := $(shell cat "$(BUILD)/.build-mode" 2>/dev/null)
endif
ifneq ($(strip $(PREVIOUS_BUILD_MODE)),$(BUILD_MODE))
$(OBJ): force-build-mode
endif

HOST_CXX ?= c++
TEST_BINARY = build/tests/ai-vision-tests
TEST_SOURCES = src/AIVision.cpp tests/AIVisionTest.cpp
TEST_HEADERS = include/AIVision.h tests/fakes/vex.h
VISION_TEST_BINARY = build/tests/ai-vision-slow-tests

.PHONY: test
test: $(TEST_BINARY) $(VISION_TEST_BINARY)
	@set -e; for test_case in $$("$(TEST_BINARY)" --list); do \
		"$(TEST_BINARY)" "$$test_case"; \
	done
	@set -e; for test_case in $$("$(VISION_TEST_BINARY)" --list); do \
		"$(VISION_TEST_BINARY)" "$$test_case"; \
	done

$(TEST_BINARY): $(TEST_SOURCES) $(TEST_HEADERS) makefile
	@mkdir -p "$(@D)"
	$(HOST_CXX) -std=c++11 -Wall -Wextra -Werror -Itests/fakes -Iinclude $(TEST_SOURCES) -o "$@"

$(VISION_TEST_BINARY): $(TEST_SOURCES) $(TEST_HEADERS) makefile
	@mkdir -p "$(@D)"
	$(HOST_CXX) -std=c++11 -Wall -Wextra -Werror -DAI_VISION_TEST -Itests/fakes -Iinclude $(TEST_SOURCES) -o "$@"
