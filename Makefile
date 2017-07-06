# Defines

LLVM_VERSION = 3.9.1
CMAKE_MM_VERSION = 3.7
CMAKE_VERSION = $(CMAKE_MM_VERSION).2

ifeq ($(OSTYPE),cygwin)
	CLEANUP=rm -f
	CLEANDIR=rm -rf
	MKDIR=mkdir -p
	TARGET_EXTENSION=out
else ifeq ($(OS),Windows_NT)
	CLEANUP=del /F /Q
	CLEANDIR=rd /S /Q
	MKDIR=mkdir
	TARGET_EXTENSION=exe
else
	CLEANUP=rm -f
	CLEANDIR=rm -rf
	MKDIR=mkdir -p
	TARGET_EXTENSION=out
endif

CMAKEGZ = cmake-$(CMAKE_VERSION)-Linux-x86_64
CMAKE = $(abspath ../$(CMAKEGZ)/bin/cmake)
LLVMXZ = llvm-$(LLVM_VERSION).src
LLVMSRC = $(abspath ../$(LLVMXZ))/
LLVMBUILD = $(LLVMSRC)build/
PATHL = $(abspath ../llvm)/
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

GCSRC = $(PATHS)gc-7.2/
GCBUILD = $(abspath $(PATHB)gc)
GCLIB = $(GCBUILD)/lib
GCINC = $(GCBUILD)/include

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
# CFLAGS =		-I. -I$(PATHUS) -I$(LLVMINC) -I$(PATHS) -g -DTEST -Wall -Wextra -pedantic -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS =		-I. -I$(PATHUS) -I$(PATHS) -I$(GCINC) -g -DTEST -Wall -Wextra -pedantic -std=c99 -D_POSIX_C_SOURCE=200809L

# LDFLAGS =		$(shell $(LLVMCONFIG) --ldflags)
# LDLIBS =		$(shell $(LLVMCONFIG) --libs core) -L$(GCLIB) -lpthread -ldl -ltinfo -lgc
LDLIBS =		-L$(GCLIB) -lgc -ldl -Wl,-rpath -Wl,/home/degustaf/lisp-compiler/build/gc/lib

DEPEND =		$(CC) $(CFLAGS) -MM -MG -MF
RESULTS =		$(patsubst $(PATHT)Test%.c,$(PATHR)Test%.txt,$(SRCT))
OBJS =			$(filter-out $(PATHO)lisp.o, $(patsubst $(PATHS)%.c,$(PATHO)%.o,$(SRCS)))

-include $(wildcard $(PATHD)*.txt)

.PHONY: all
.PHONY: clean
.PHONY: clean-recur
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

# $(PATHB)lisp.$(TARGET_EXTENSION): $(OBJS) | $(LLVMCONFIG)
$(PATHB)lisp.$(TARGET_EXTENSION): $(OBJS) $(PATHO)lisp.o | $(GCLIB)
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

# # | $(LLVMCONFIG)
# $(PATHB)TestReader.$(TARGET_EXTENSION): $(PATHO)TestReader.o $(PATHO)Reader.o $(PATHUS)unity.o $(PATHO)Numbers.o $(PATHO)Strings.o $(PATHO)List.o $(PATHO)ASeq.o $(PATHO)Cons.o $(PATHO)Util.o \
# 	$(PATHO)Murmur3.o $(PATHO)Error.o $(PATHO)StringWriter.o $(PATHO)Map.o $(PATHO)Vector.o $(PATHO)AFn.o | $(GCLIB)
# 	$(LINK) -o $@ $^ $(LDLIBS) $(LDFLAGS)
# 
# # | $(LLVMCONFIG)
# $(PATHB)TestList.$(TARGET_EXTENSION): $(PATHO)TestList.o $(PATHO)List.o $(PATHO)ASeq.o $(PATHO)Util.o $(PATHO)Murmur3.o $(PATHO)Numbers.o $(PATHO)Map.o $(PATHO)Cons.o $(PATHO)StringWriter.o $(PATHUS)unity.o | $(GCLIB)
# 	$(LINK) -o $@ $^ $(LDLIBS) $(LDFLAGS)

# | $(LLVMCONFIG)
$(PATHB)Test%.$(TARGET_EXTENSION): $(PATHO)Test%.o $(OBJS) $(PATHUS)unity.o | $(GCLIB)
	$(LINK) -o $@ $^ $(LDLIBS) $(LDFLAGS)

$(PATHB)Test%.$(TARGET_EXTENSION): $(PATHD)Test%.d

# | $(LLVMINC) 
$(PATHO)%.o:: $(PATHT)%.c | $(PATHU) $(GCINC)
	$(COMPILE) $(CFLAGS) $< -o $@

# | $(LLVMINC)
$(PATHO)%.o:: $(PATHS)%.c | $(GCINC)
	$(COMPILE) $(CFLAGS) $< -o $@

$(PATHUS)%.o:: $(PATHUS)%.c $(PATHUS)%.h | $(PATHU) $(GCINC)
	$(COMPILE) $(CFLAGS) $< -o $@ 

$(PATHD)%.d:: $(PATHT)%.c | $(GCLIB)
	# | $(LLVMINC)
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

clean-recur: clean
	$(CLEANDIR) $(GCBUILD)
	cd $(GCSRC) && $(MAKE) clean

$(PATHU):
	cd .. && git clone --depth 1 https://github.com/ThrowTheSwitch/Unity.git

$(LLVMSRC):
	cd .. && curl http://releases.llvm.org/$(LLVM_VERSION)/$(LLVMXZ).tar.xz | tar -xJ

$(CMAKE):
	cd .. && curl https://cmake.org/files/v$(CMAKE_MM_VERSION)/$(CMAKEGZ).tar.gz | tar -xz

$(LLVMBUILD): $(LLVMSRC) $(CMAKE)
	cd $(LLVMSRC) && $(MKDIR) build
	# What follows should be split out, if we can decide what an appropriate target is.
	cd $(LLVMBUILD) && $(CMAKE) -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=$(PATHL) $(LLVMSRC)

$(LLVMCONFIG):  | $(LLVMBUILD) $(PATHL)lib/libLLVMSupport.a $(PATHL)lib/libLLVMCore.a
	cd $(LLVMBUILD) && $(MAKE) install-llvm-config

$(LLVMINC): $(LLVMBUILD)
	cd $(LLVMBUILD) && $(MAKE) installhdrs

$(PATHL)lib/libLLVMSupport.a: | $(LLVMBUILD)
	cd $(LLVMBUILD) && $(MAKE) install-LLVMSupport

$(PATHL)lib/libLLVMCore.a: | $(LLVMBUILD)
	cd $(LLVMBUILD) && $(MAKE) install-LLVMCore

$(GCSRC)Makefile:
	cd $(GCSRC) && ./configure --prefix=$(GCBUILD) --disable-threads

$(GCSRC).libs: $(GCSRC)Makefile
	cd $(GCSRC) && $(MAKE)

$(GCINC): $(GCSRC).libs
	cd $(GCSRC) && $(MAKE) check
	cd $(GCSRC) && $(MAKE) install

$(GCLIB): $(GCSRC).libs
	cd $(GCSRC) && $(MAKE) check
	cd $(GCSRC) && $(MAKE) install

# Additional targets will need to be added for these
# make -j installhdrs 
