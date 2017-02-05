# Defines

LLVM_VERSION = 3.9.1

ifeq ($(OSTYPE),cygwin)
	CLEANUP=rm -f
	MKDIR=mkdir -p
	TARGET_EXTENSION=out
else ifeq ($(OS),Windows_NT)
	CLEANUP=del /F /Q
	MKDIR=mkdir
	TARGET_EXTENSION=exe
else
	CLEANUP=rm -f
	MKDIR=mkdir -p
	TARGET_EXTENSION=out
endif

PATHL = ../llvm/
PATHU = ../Unity/
PATHUS = $(PATHU)src/
PATHS = src/
PATHT = unittest/
PATHF = functional/
PATHB = build/
PATHD = build/depends/
PATHO = build/objs/
PATHR = build/results/

BUILD_PATHS = $(PATHB) $(PATHD) $(PATHO) $(PATHR)

ifndef CC
	CC = gcc
endif
ifndef CXX
	CXX = g++
endif

SRCS =			$(wildcard $(PATHS)*.c)
SRCT =			$(wildcard $(PATHT)*.c)
LLVMCONFIG =	$(PATHL)bin/llvm-config
COMPILE =		$(CC) -c
LINK = 			$(CXX)
CFLAGS =		-I. -I$(PATHU) -I$(PATHL)/include/ -I$(PATHS) -DTEST -Wall -Wextra -pedantic -std=c99 -D_POSIX_C_SOURCE=200809L

LDFLAGS =		$(shell $(LLVMCONFIG) --ldflags)
LDLIBS =		$(shell $(LLVMCONFIG) --libs core) -lpthread -ldl -ltinfo

DEPEND =		$(CC) $(CFLAGS) -MM -MG -MF
RESULTS =		$(patsubst $(PATHT)Test%.c,$(PATHR)Test%.txt,$(SRCT))
OBJS =			$(patsubst $(PATHS)%.c,$(PATHO)%.o,$(SRCS))

-include $(wildcard $(PATHD)*.txt)

.PHONY: all
.PHONY: clean
.PHONY: test
.PHONY: functionaltest
.PHONY: unittest
.PHONY: get-depends
.PRECIOUS: $(PATHB)Test%.$(TARGET_EXTENSION)
.PRECIOUS: $(PATHD)%.d
.PRECIOUS: $(PATHO)%.o
.PRECIOUS: $(PATHUS)%.o
.PRECIOUS: $(PATHR)%.txt


# Rules

all: unittest $(PATHB)lisp.$(TARGET_EXTENSION) functionaltest

$(PATHB)lisp.$(TARGET_EXTENSION): $(OBJS)
	$(LINK) -o $@ $^ $(LDLIBS) $(LDFLAGS)

unittest: $(BUILD_PATHS) $(RESULTS)
	@echo "-----------------------\nIGNORES:\n-----------------------"
	@echo `grep -s IGNORE $(PATHR)*.txt`
	@echo "-----------------------\nFAILURES:\n-----------------------"
	@echo `grep -s FAIL $(PATHR)*.txt`
	@echo "-----------------------\nPASSES:\n-----------------------"
	@echo `grep -c PASS $(PATHR)*.txt` tests passed.
	@echo "\nDONE"
	@exit `grep -c FAIL $(PATHR)*.txt`

functionaltest: $(PATHB)lisp.$(TARGET_EXTENSION) 

$(PATHR)%.txt: $(PATHB)%.$(TARGET_EXTENSION)
	-./$< > $@ 2>&1

$(PATHB)Test%.$(TARGET_EXTENSION): $(PATHO)Test%.o $(PATHO)%.o $(PATHUS)unity.o
	$(LINK) -o $@ $^

$(PATHO)%.o:: $(PATHT)%.c
	$(COMPILE) $(CFLAGS) $< -o $@

$(PATHO)%.o:: $(PATHS)%.c
	$(COMPILE) $(CFLAGS) $< -o $@

$(PATHUS)%.o:: $(PATHUS)%.c $(PATHUS)%.h
	$(COMPILE) $(CFLAGS) $< -o $@ 

$(PATHD)%.d:: $(PATHT)%.c
	$(DEPEND) $@ $<

$(PATHB):
	$(MKDIR) $(PATHB)

$(PATHD):
	$(MKDIR) $(PATHD)

$(PATHO):
	$(MKDIR) $(PATHO)

$(PATHR):
	$(MKDIR) $(PATHR)

clean:
	$(CLEANUP) $(PATHO)*.o
	$(CLEANUP) $(PATHB)*.$(TARGET_EXTENSION)
	$(CLEANUP) $(PATHR)*.txt

get-depends: $(PATHU) $(PATHL)

$(PATHU):
	cd .. && git clone https://github.com/ThrowTheSwitch/Unity.git

$(PATHL):
	./install_llvm.sh $(LLVM_VERSION)
