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

LLVMSRC = ../llvm-$(LLVM_VERSION).src/
LLVMBUILD = $(LLVMSRC)build/
PATHL = ../llvm/
LLVMINC = $(PATHL)include/
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
CFLAGS =		-I. -I$(PATHUS) -I$(LLVMINC) -I$(PATHS) -g -DTEST -Wall -Wextra -pedantic -std=c99 -D_POSIX_C_SOURCE=200809L

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
.PRECIOUS: $(PATHB)Test%.$(TARGET_EXTENSION)
.PRECIOUS: $(PATHD)%.d
.PRECIOUS: $(PATHO)%.o
.PRECIOUS: $(PATHUS)%.o
.PRECIOUS: $(PATHR)%.txt


# Rules

all: unittest $(PATHB)lisp.$(TARGET_EXTENSION) functionaltest

$(PATHB)lisp.$(TARGET_EXTENSION): $(OBJS) | $(LLVMCONFIG)
	$(LINK) -o $@ $^ $(LDLIBS) $(LDFLAGS)

unittest: $(BUILD_PATHS) $(RESULTS)
	@echo "-----------------------\nIGNORES:\n-----------------------"
	@echo `grep --no-messages IGNORE $(PATHR)*.txt`
	@echo "-----------------------\nFAILURES:\n-----------------------"
	@echo `grep --no-messages FAIL $(PATHR)*.txt`
	@echo `grep --no-messages --with-filename 'Segmentation fault' $(PATHR)*.txt`
	@echo `grep --no-messages --with-filename 'Assertion' $(PATHR)*.txt`
	@echo "-----------------------\nPASSES:\n-----------------------"
	@echo `cat $(PATHR)*.txt | grep --count PASS` tests passed.
	@echo "\nDONE"
	@exit $$(( `cat $(PATHR)*.txt | grep --count FAIL` + `cat $(PATHR)*.txt | grep --count 'Segmentation fault'` + `cat $(PATHR)*.txt | grep --count 'Assertion'` ))

functionaltest: $(PATHB)lisp.$(TARGET_EXTENSION) 

$(PATHR)%.txt: $(PATHB)%.$(TARGET_EXTENSION)
	-./$< > $@ 2>&1

$(PATHB)TestReader.$(TARGET_EXTENSION): $(PATHO)TestReader.o $(PATHO)Reader.o $(PATHUS)unity.o $(PATHO)Numbers.o $(PATHO)Strings.o $(PATHO)List.o $(PATHO)ASeq.o $(PATHO)Cons.o $(PATHO)Util.o \
	$(PATHO)Murmur3.o $(PATHO)Error.o $(PATHO)StringWriter.o $(PATHO)Map.o | $(LLVMCONFIG)
	$(LINK) -o $@ $^ $(LDLIBS) $(LDFLAGS)

$(PATHB)TestList.$(TARGET_EXTENSION): $(PATHO)TestList.o $(PATHO)List.o $(PATHO)ASeq.o $(PATHO)Util.o $(PATHO)Murmur3.o $(PATHO)Numbers.o $(PATHO)Cons.o $(PATHO)StringWriter.o $(PATHUS)unity.o \
	| $(LLVMCONFIG)
	$(LINK) -o $@ $^ $(LDLIBS) $(LDFLAGS)

$(PATHB)Test%.$(TARGET_EXTENSION): $(PATHO)Test%.o $(PATHO)%.o $(PATHUS)unity.o | $(LLVMCONFIG)
	$(LINK) -o $@ $^ $(LDLIBS) $(LDFLAGS)

$(PATHB)Test%.$(TARGET_EXTENSION): $(PATHD)Test%.d

$(PATHO)%.o:: $(PATHT)%.c | $(LLVMINC)
	$(COMPILE) $(CFLAGS) $< -o $@

$(PATHO)%.o:: $(PATHS)%.c | $(LLVMINC)
	$(COMPILE) $(CFLAGS) $< -o $@

$(PATHUS)%.o:: $(PATHUS)%.c $(PATHUS)%.h | $(PATHU)
	$(COMPILE) $(CFLAGS) $< -o $@ 

$(PATHD)%.d:: $(PATHT)%.c | $(LLVMINC)
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

$(PATHU):
	cd .. && git clone https://github.com/ThrowTheSwitch/Unity.git

$(PATHL):
	./install_llvm.sh $(LLVM_VERSION)

$(LLVMSRC):
	cd .. && curl -O http://releases.llvm.org/$(LLVM_VERSION)/$(LLVMSRC).tar.xz | tar -xJf

$(LLVMBUILD): $(LLVMSRC)
	cd $(LLVMSRC) && $(MKDIR) build
	# What follows should be split out, if we can decide what an appropriate target is.
	cd $(LLVMBUILD) && cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=$(PATHL) $(LLVMSRC)

$(LLVMCONFIG):  | $(LLVMBUILD) $(PATHL)lib/libLLVMSupport.a $(PATHL)lib/libLLVMCore.a
	cd $(LLVMBUILD) && $(MAKE) install-llvm-config

$(LLVMINC): $(LLVMBUILD)
	cd $(LLVMBUILD) && $(MAKE) installhdrs

$(PATHL)lib/libLLVMSupport.a: | $(LLVMBUILD)
	cd $(LLVMBUILD) && $(MAKE) install-LLVMSupport

$(PATHL)lib/libLLVMCore.a: | $(LLVMBUILD)
	cd $(LLVMBUILD) && $(MAKE) install-LLVMCore

# Additional targets will need to be added for these
# make -j installhdrs 
