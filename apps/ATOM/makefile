# ATOM - Atoms, configs, light
# Build: make   (needs CEdev on PATH; output bin/ATOM.8xp)
NAME = ATOM
DESCRIPTION = "Atoms, configs, light"
COMPRESSED = YES
ARCHIVED = YES

COMMON = ../../src/common
CFLAGS = -Wall -Wextra -Oz -I$(COMMON)
EXTRA_C_SOURCES = $(wildcard $(COMMON)/*.c)

include $(shell cedev-config --makefile)
