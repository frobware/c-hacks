CMAKE_GENERATOR		:= "Unix Makefiles"
BUILD_DIR               := build

CMAKE_BUILD_TYPE	?= Debug
CMAKE_C_COMPILER        ?= gcc

.PHONY: all rclean $(BUILD_DIR)

all: $(BUILD_DIR)/Makefile
	@$(MAKE) --no-print-directory -C $(BUILD_DIR) all

# Argument 1 is the build directory.
#
# Argument 2 is specific arguments to pass to the cmake invocation.
#
# Argument 3 is the directory containing the CMakeLists.txt.
#
define regenerate_cmake_rules
	@echo "Regenerating CMake build in $(1)"
	cd $(1) ;					\
	cmake -G $(CMAKE_GENERATOR)			\
		-DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE)	\
		-DCMAKE_C_COMPILER=$(CMAKE_C_COMPILER)	\
		$(2) $(3)
endef

$(BUILD_DIR)/Makefile: | $(BUILD_DIR)
	$(call regenerate_cmake_rules, $(BUILD_DIR), $(CURDIR))

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/Makefile: Makefile		\
	CMakeLists.txt				\
	src/CMakeLists.txt			\
	tests/CMakeLists.txt			\
	| $(BUILD_DIR)

rclean:
	$(RM) -r $(BUILD_DIR)

.DEFAULT:
	@$(MAKE) --no-print-directory -C $(BUILD_DIR) $@
