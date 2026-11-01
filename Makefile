# Host build: unit tests for src/common (gcc). Calculator apps: see apps/*/makefile.
CC      ?= gcc
CFLAGS  ?= -std=c99 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined
LDLIBS  = -lm
COMMON  = $(filter-out %_ce.c,$(wildcard src/common/*.c))
TESTS   = $(patsubst tests/%.c,build/%,$(wildcard tests/test_*.c))
APPS    = SIGFIG CONVERT ATOM NAMES REACT

.PHONY: test apps clean
test: $(TESTS)
	@set -e; cd build; for t in $(notdir $(TESTS)); do ./$$t; done

build/test_%: tests/test_%.c $(COMMON) tests/plat_mock.c tests/*.h src/common/*.h $(wildcard apps/*/src/*.[ch]) | build
	$(CC) $(CFLAGS) -Isrc/common -o $@ $< $(COMMON) tests/plat_mock.c $(LDLIBS)

build:
	mkdir -p build

apps:
	@set -e; for a in $(APPS); do $(MAKE) -C apps/$$a; done

clean:
	rm -rf build apps/*/bin apps/*/obj
