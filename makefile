# VEXcode makefile 2019_03_26_01

# show compiler output
VERBOSE = 0

# include toolchain options
include vex/mkenv.mk

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

HOST_CXX ?= c++
TEST_BINARY = build/tests/ai-vision-tests
TEST_SOURCES = src/AIVision.cpp tests/AIVisionTest.cpp
TEST_HEADERS = include/AIVision.h tests/fakes/vex.h

.PHONY: test
test: $(TEST_BINARY)
	@set -e; for test_case in $$("$(TEST_BINARY)" --list); do \
		"$(TEST_BINARY)" "$$test_case"; \
	done

$(TEST_BINARY): $(TEST_SOURCES) $(TEST_HEADERS) makefile
	@mkdir -p "$(@D)"
	$(HOST_CXX) -std=c++11 -Wall -Wextra -Werror -Itests/fakes -Iinclude $(TEST_SOURCES) -o "$@"
