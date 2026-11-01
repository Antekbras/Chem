# NAMES - Formula and name
# Build: make   (needs CEdev on PATH; output bin/NAMES.8xp)
NAME = NAMES
DESCRIPTION = "Formula and name"
COMPRESSED = YES
ARCHIVED = YES

COMMON = ../../src/common
CFLAGS = -Wall -Wextra -Oz -I$(COMMON)
EXTRA_C_SOURCES = $(wildcard $(COMMON)/*.c)

include $(shell cedev-config --makefile)
