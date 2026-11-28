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
ifneq ($(findstring AIM_TEST,$(CXX_FLAGS) $(CFLAGS)),)
BUILD_MODE := $(BUILD_MODE)-aim-test
endif
ifneq ($(BUILD_MODE),normal)
$(error Hardware-test builds belong on the test branch; main is production-only)
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
