# CONVERT - Units, moles, molarity
# Build: make   (needs CEdev on PATH; output bin/CONVERT.8xp)
NAME = CONVERT
DESCRIPTION = "Units, moles, molarity"
COMPRESSED = NO
ARCHIVED = NO

COMMON = ../../src/common
CFLAGS = -Wall -Wextra -Oz -I$(COMMON)
EXTRA_C_SOURCES = $(wildcard $(COMMON)/*.c)

include $(shell cedev-config --makefile)
