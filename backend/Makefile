CXX      ?= clang++
CXXSTD   ?= -std=c++17
WARNINGS := -Wall -Wextra -Wpedantic
INCLUDES := -Iinclude
OPT      ?= -O2

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  PLATFORM_LIBS :=
else
  PLATFORM_LIBS := -lcrypto
endif

CXXFLAGS := $(CXXSTD) $(WARNINGS) $(INCLUDES) $(OPT)

SRC_DIR   := src
BUILD_DIR := build
TARGET    := $(BUILD_DIR)/cc-server

SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all run console clean info

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(OBJECTS) -o $@ $(PLATFORM_LIBS)
	@echo ""
	@echo "Built: $(TARGET)"
	@echo "Next : ./$(TARGET) --selftest"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) --selftest

console: $(TARGET)
	./$(TARGET) --console

info:
	@echo "Compiler : $(CXX)"
	@echo "Standard : $(CXXSTD)"
	@echo "Flags    : $(CXXFLAGS)"
	@echo "Platform : $(UNAME_S)"
	@echo "Sources  : $(SOURCES)"

clean:
	rm -rf $(BUILD_DIR)
	@echo "Build folder removed."

$(OBJECTS): $(wildcard include/*.h)
