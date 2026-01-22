SOURCES := $(shell find src include -name '*.h' -o -name '*.cc' -o -name '*.cu')

.PHONY: format format_check

format:
	clang-format -i $(SOURCES)
format_check:
	clang-format --dry-run --Werror $(SOURCES)
