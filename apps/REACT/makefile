# REACT - Balance and stoichiometry
# Build: make   (needs CEdev on PATH; output bin/REACT.8xp)
NAME = REACT
DESCRIPTION = "Balance and stoichiometry"
COMPRESSED = YES
ARCHIVED = YES

COMMON = ../../src/common
CFLAGS = -Wall -Wextra -Oz -I$(COMMON)
EXTRA_C_SOURCES = $(wildcard $(COMMON)/*.c)

include $(shell cedev-config --makefile)
