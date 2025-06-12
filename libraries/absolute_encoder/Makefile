# Makefile for Absolute Encoder Component
# 生成单个链接文件 absolute_encoder.o

# 编译器设置
CXX = g++
CC = gcc
AR = ar

# 编译选项
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -fPIC
CFLAGS = -std=c99 -Wall -Wextra -O2 -fPIC

# 调试版本编译选项
DEBUG_CXXFLAGS = -std=c++11 -Wall -Wextra -g -DDEBUG -fPIC
DEBUG_CFLAGS = -std=c99 -Wall -Wextra -g -DDEBUG -fPIC

# 头文件路径
INCLUDES = -I.

# 源文件
CXX_SOURCES = \
	encoder_map.cpp \
	smart_search_manager.cpp \
	position_tracker.cpp \
	input_preprocessor.cpp \
	absolute_encoder.cpp

C_SOURCES = 

# 目标文件
CXX_OBJECTS = $(CXX_SOURCES:.cpp=.o)
C_OBJECTS = $(C_SOURCES:.c=.o)
ALL_OBJECTS = $(CXX_OBJECTS) $(C_OBJECTS)

# 临时目录用于存放中间文件
BUILD_DIR = build
DEBUG_BUILD_DIR = build-debug

# 最终目标文件
TARGET = absolute_encoder.o
DEBUG_TARGET = absolute_encoder_debug.o

# 静态库目标（可选）
STATIC_LIB = libabsolute_encoder.a
DEBUG_STATIC_LIB = libabsolute_encoder_debug.a

# 默认目标
.PHONY: all clean debug release help install

all: release

# 发布版本
release: $(TARGET)

# 调试版本
debug: CXXFLAGS = $(DEBUG_CXXFLAGS)
debug: CFLAGS = $(DEBUG_CFLAGS)
debug: BUILD_DIR = $(DEBUG_BUILD_DIR)
debug: TARGET = $(DEBUG_TARGET)
debug: $(TARGET)

# 创建最终的单个目标文件
$(TARGET): $(addprefix $(BUILD_DIR)/, $(ALL_OBJECTS))
	@echo "Creating combined object file: $@"
	@mkdir -p $(BUILD_DIR)/temp
	@# 提取所有目标文件到临时目录
	@for obj in $^; do \
		cd $(BUILD_DIR)/temp && $(AR) x ../../$$obj 2>/dev/null || cp ../../$$obj . ; \
	done
	@# 将所有目标文件重新打包成单个.o文件
	$(CXX) -r -o $@ $(BUILD_DIR)/temp/*.o
	@rm -rf $(BUILD_DIR)/temp
	@echo "Build complete: $@"
	@echo "File size: $$(du -h $@ | cut -f1)"

# 编译C++源文件
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling C++: $<"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# 编译C源文件  
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling C: $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 创建静态库（可选目标）
lib: $(STATIC_LIB)

$(STATIC_LIB): $(addprefix $(BUILD_DIR)/, $(ALL_OBJECTS))
	@echo "Creating static library: $@"
	$(AR) rcs $@ $^
	@echo "Static library created: $@"

# 调试版静态库
debug-lib: debug
debug-lib: STATIC_LIB = $(DEBUG_STATIC_LIB)
debug-lib: $(STATIC_LIB)

# 清理
clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR) $(DEBUG_BUILD_DIR)
	@rm -f $(TARGET) $(DEBUG_TARGET)
	@rm -f $(STATIC_LIB) $(DEBUG_STATIC_LIB)
	@echo "Clean complete"

# 安装到系统目录（可选）
install: $(TARGET)
	@echo "Installing absolute_encoder.o..."
	@mkdir -p ../lib
	@cp $(TARGET) ../lib/
	@mkdir -p ../include/absolute_encoder
	@cp *.hpp *.h ../include/absolute_encoder/
	@echo "Installation complete"

# 显示帮助信息
help:
	@echo "Absolute Encoder Component Makefile"
	@echo "======================================"
	@echo "Available targets:"
	@echo "  all        - Build release version (default)"
	@echo "  release    - Build optimized release version"
	@echo "  debug      - Build debug version with symbols"
	@echo "  lib        - Build static library (.a file)"
	@echo "  debug-lib  - Build debug static library"
	@echo "  clean      - Remove all build files"
	@echo "  install    - Install to ../lib and ../include"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Output files:"
	@echo "  Release: absolute_encoder.o"
	@echo "  Debug:   absolute_encoder_debug.o"
	@echo "  Library: libabsolute_encoder.a"

# 显示编译信息
info:
	@echo "Compilation Info:"
	@echo "================="
	@echo "CXX Sources: $(CXX_SOURCES)"
	@echo "C Sources:   $(C_SOURCES)"
	@echo "Build Dir:   $(BUILD_DIR)"
	@echo "Target:      $(TARGET)"
	@echo "Compiler:    $(CXX)"
	@echo "Flags:       $(CXXFLAGS)"

# 依赖关系
$(BUILD_DIR)/absolute_encoder.o: absolute_encoder.cpp absolute_encoder.h encoder_map.hpp smart_search_manager.hpp position_tracker.hpp input_preprocessor.hpp
$(BUILD_DIR)/encoder_map.o: encoder_map.cpp encoder_map.hpp absolute_encoder_logger.h
$(BUILD_DIR)/smart_search_manager.o: smart_search_manager.cpp smart_search_manager.hpp encoder_map.hpp absolute_encoder_logger.h
$(BUILD_DIR)/position_tracker.o: position_tracker.cpp position_tracker.hpp encoder_map.hpp absolute_encoder_logger.h
$(BUILD_DIR)/input_preprocessor.o: input_preprocessor.cpp input_preprocessor.hpp absolute_encoder_logger.h

# 检查源文件是否存在
check-sources:
	@echo "Checking source files..."
	@for src in $(CXX_SOURCES) $(C_SOURCES); do \
		if [ ! -f $$src ]; then \
			echo "Error: Source file $$src not found"; \
			exit 1; \
		fi; \
	done
	@echo "All source files found"

# 验证构建结果
verify: $(TARGET)
	@echo "Verifying build result..."
	@if [ -f $(TARGET) ]; then \
		echo "✓ Target file exists: $(TARGET)"; \
		file $(TARGET); \
		size $(TARGET); \
	else \
		echo "✗ Target file not found: $(TARGET)"; \
		exit 1; \
	fi

.PHONY: info check-sources verify