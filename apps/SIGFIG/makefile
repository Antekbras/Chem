# SIGFIG - Sig-fig calculator
# Build: make   (needs CEdev on PATH; output bin/SIGFIG.8xp)
NAME = SIGFIG
DESCRIPTION = "Sig-fig calculator"
COMPRESSED = NO
ARCHIVED = NO

COMMON = ../../src/common
CFLAGS = -Wall -Wextra -Oz -I$(COMMON)
EXTRA_C_SOURCES = $(wildcard $(COMMON)/*.c)

include $(shell cedev-config --makefile)
