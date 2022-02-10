-include conanbuildinfo.mak

PREFIX ?= /usr/local
EXCEPTIONS ?= 1
COVERAGE ?= 0
DEBUG ?= 0
RELDEBUG ?= 0
PIC ?= 0
LTO ?= 1

GOOGLE_BENCH_CPPFLAGS = $(addprefix -isystem, $(CONAN_INCLUDE_DIRS_BENCHMARK) $(GOOGLE_BENCH_INCLUDE_DIR))
GOOGLE_BENCH_LDFLAGS = $(addprefix -L, $(CONAN_LIB_DIRS_BENCHMARK) $(GOOGLE_BENCH_LIB_DIR))
CATCH2_CPPFLAGS = $(addprefix -isystem, $(CONAN_INCLUDE_DIRS_CATCH2) $(CATCH2_INCLUDE_DIR))
FMT_CPPFLAGS = $(addprefix -isystem, $(CONAN_INCLUDE_DIRS_FMT) $(FMT_INCLUDE_DIR))
FMT_LDFLAGS = $(addprefix -L, $(CONAN_LIB_DIRS_FMT) $(FMT_LIB_DIR))

BUILD_DIR := build/$(notdir $(CXX))

# CXXFLAGS may be set by Conan or other tools, so only set debug/optimization
# flags if CXXFLAGS is not set.
ifeq ($(CXXFLAGS),)
	DEBUG_CXXFLAGS = -O0 -ggdb
	DEBUG_CPPFLAGS = -DXTR_ENABLE_TEST_STATIC_ASSERTIONS
	OPT_CXXFLAGS = -O3 -march=native
	OPT_CPPFLAGS = -DNDEBUG
ifneq (,$(wildcard /usr/lib/x86_64-linux-gnu/liburing.a))
	LDLIBS += -luring
endif
endif

CXXFLAGS += \
	-std=c++20 -Wall -Wextra -Wconversion -Wshadow -Wcast-qual -Wformat=2 \
	-pedantic -pipe -pthread
CPPFLAGS += -MMD -MP -I include $(FMT_CPPFLAGS) -DXTR_FUNC=
LDFLAGS += -fuse-ld=gold
LDLIBS += -lxtr $(addprefix -l, $(CONAN_LIBS_LIBURING))

TEST_CPPFLAGS = $(CATCH2_CPPFLAGS) 
TEST_LDFLAGS = -L $(BUILD_DIR) $(FMT_LDFLAGS)

BENCH_CPPFLAGS = $(GOOGLE_BENCH_CPPFLAGS)
BENCH_LDFLAGS = -L $(BUILD_DIR) $(GOOGLE_BENCH_LDFLAGS) $(FMT_LDFLAGS)
BENCH_LDLIBS = -lbenchmark

XTRCTL_LDFLAGS = -L $(BUILD_DIR) $(FMT_LDFLAGS)

COVERAGE_CXXFLAGS = --coverage -DNDEBUG

# Use the libfmt submodule if it is present and no include directory for
# libfmt has been configured (including via Conan).
ifeq ($(FMT_CPPFLAGS),)
	ifneq ($(wildcard third_party/fmt/include),)
		SUBMODULES_FLAG := 1
	endif
endif

ifneq ($(SUBMODULES_FLAG),)
	FMT_CPPFLAGS += -DFMT_HEADER_ONLY
	CPPFLAGS += -isystem third_party/include
else ifneq ($(CONAN_LIBS_FMT),)
	LDLIBS += -l$(CONAN_LIBS_FMT)
else
	LDLIBS += -lfmt
endif

ifneq (,$(findstring clang,$(CXX)))
	RANLIB ?= llvm-ranlib
	AR ?= llvm-ar
else
	RANLIB ?= gcc-ranlib
	AR ?= gcc-ar
endif

ifeq ($(PIC), 1)
	CXXFLAGS += -fPIC
	BUILD_DIR := $(BUILD_DIR)-pic
endif

ifeq ($(LTO), 1)
	CXXFLAGS += -flto
	BUILD_DIR := $(BUILD_DIR)-lto
endif

ifeq ($(COVERAGE), 1)
	CXXFLAGS += $(COVERAGE_CXXFLAGS)
	BUILD_DIR := $(BUILD_DIR)-coverage
	COVERAGE_DATA = \
		$(SRCS:%=$(BUILD_DIR)/%.gcno) $(SRCS:%=$(BUILD_DIR)/%.gcda) \
		$(TEST_SRCS:%=$(BUILD_DIR)/%.gcno) $(TEST_SRCS:%=$(BUILD_DIR)/%.gcda)
endif

ifeq ($(RELDEBUG), 1)
	CXXFLAGS += $(DEBUG_CXXFLAGS) $(OPT_CXXFLAGS)
	CPPFLAGS += $(DEBUG_CPPFLAGS) $(OPT_CPPFLAGS)
	BUILD_DIR := $(BUILD_DIR)-reldebug
else ifeq ($(DEBUG), 1)
	CXXFLAGS += $(DEBUG_CXXFLAGS)
	CPPFLAGS += $(DEBUG_CPPFLAGS)
	BUILD_DIR := $(BUILD_DIR)-debug
else
	CXXFLAGS += $(OPT_CXXFLAGS)
	CPPFLAGS += $(OPT_CPPFLAGS)
	BUILD_DIR := $(BUILD_DIR)-release
endif

ifneq ($(SANITIZER),)
	CXXFLAGS += -fno-omit-frame-pointer -fsanitize=$(SANITIZER)
	BUILD_DIR := $(BUILD_DIR)-$(SANITIZER)-sanitizer
endif

ifeq ($(EXCEPTIONS), 0)
	CXXFLAGS += -fno-exceptions
	BUILD_DIR := $(BUILD_DIR)-no-exceptions
endif

TARGET = $(BUILD_DIR)/libxtr.a
SRCS := \
	src/command_dispatcher.cpp src/command_path.cpp src/consumer.cpp \
	src/fd_storage.cpp src/file_descriptor.cpp src/logger.cpp \
	src/log_level.cpp src/matcher.cpp src/memory_mapping.cpp \
	src/mirrored_memory_mapping.cpp src/pagesize.cpp src/regex_matcher.cpp \
	src/sink.cpp src/throw.cpp src/tsc.cpp \
	src/wildcard_matcher.cpp

OBJS = $(SRCS:%=$(BUILD_DIR)/%.o)

TEST_TARGET = $(BUILD_DIR)/test/test
TEST_SRCS := \
	test/align.cpp test/command_client.cpp test/command_dispatcher.cpp \
	test/file_descriptor.cpp test/logger.cpp test/main.cpp \
	test/memory_mapping.cpp test/mirrored_memory_mapping.cpp test/pagesize.cpp \
	test/synchronized_ring_buffer.cpp test/throw.cpp
TEST_OBJS = $(TEST_SRCS:%=$(BUILD_DIR)/%.o)

BENCH_TARGET = $(BUILD_DIR)/benchmark/benchmark
BENCH_SRCS := benchmark/logger.cpp benchmark/main.cpp
BENCH_OBJS = $(BENCH_SRCS:%=$(BUILD_DIR)/%.o)

XTRCTL_TARGET = $(BUILD_DIR)/xtrctl
XTRCTL_SRCS := src/xtrctl.cpp
XTRCTL_OBJS = $(XTRCTL_SRCS:%=$(BUILD_DIR)/%.o)

DOCS_SRCS := \
	docs-src/index.rst docs-src/quickstart.rst \
	docs-src/guide.rst docs-src/api.rst \
	docs-src/xtrctl.rst docs-src/conf.py

MAN1_PAGES := docs/xtrctl.1
MAN3_PAGES := docs/libxtr.3 docs/libxtr-quickstart.3 docs/libxtr-userguide.3
MAN_PAGES := $(MAN1_PAGES) $(MAN3_PAGES)
HTML_DOC_PAGES := \
	docs/api.html docs/genindex.html docs/guide.html docs/index.html \
	docs/quickstart.html docs/search.html docs/xtrctl.html

DEPS = $(OBJS:.o=.d) $(TEST_OBJS:.o=.d) $(BENCH_OBJS:.o=.d) $(XTRCTL_OBJS:.o=.d)

INCLUDES = \
	$(wildcard include/xtr/*.hpp) \
	$(wildcard include/xtr/detail/*.hpp) \
	$(wildcard include/xtr/detail/commands/*.hpp \
	$(wildcard include/xtr/io/*.hpp) \
	$(wildcard include/xtr/io/detail/*.hpp))

$(TARGET): $(OBJS)
	$(AR) rc $@ $^
	$(RANLIB) $@

$(TEST_TARGET): $(TARGET) $(TEST_OBJS)
	$(LINK.cc) -o $@ $(TEST_LDFLAGS) $(TEST_OBJS) $(LDLIBS)

$(BENCH_TARGET): $(TARGET) $(BENCH_OBJS)
	$(LINK.cc) -o $@ $(BENCH_LDFLAGS) $(BENCH_OBJS) $(LDLIBS) $(BENCH_LDLIBS)

$(XTRCTL_TARGET): $(TARGET) $(XTRCTL_OBJS)
	$(LINK.cc) -o $@ $(XTRCTL_LDFLAGS) $(XTRCTL_OBJS) $(LDLIBS)

$(OBJS): $(BUILD_DIR)/%.cpp.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) -o $@ -c $(CPPFLAGS) $(CXXFLAGS) $<

$(TEST_OBJS): $(BUILD_DIR)/%.cpp.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) -o $@ -c $(CPPFLAGS) $(TEST_CPPFLAGS) $(CXXFLAGS) $<

$(BENCH_OBJS): $(BUILD_DIR)/%.cpp.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) -o $@ -c $(CPPFLAGS) $(BENCH_CPPFLAGS) $(CXXFLAGS) $<

$(XTRCTL_OBJS): $(BUILD_DIR)/%.cpp.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) -o $@ -c $(CPPFLAGS) $(CXXFLAGS) $<

all: $(TARGET) $(TEST_TARGET) $(BENCH_TARGET) $(XTRCTL_TARGET) single_include

check: $(TEST_TARGET)
	$< --order rand

benchmark: $(BENCH_TARGET)
	$<

benchmark_cpu: $(BENCH_TARGET)
	sudo cpupower --cpu $(PRODUCER_CPU),$(CONSUMER_CPU) frequency-set --governor performance
	$<
	sudo cpupower --cpu $(PRODUCER_CPU),$(CONSUMER_CPU) frequency-set --governor schedutil

xtrctl: $(XTRCTL_TARGET)

single_include/xtr/logger.hpp: $(SRCS) $(INCLUDES)
	scripts/make_single_include.sh

single_include: single_include/xtr/logger.hpp

install: $(TARGET) $(XTRCTL_TARGET) docs
	mkdir -p $(PREFIX)/lib $(PREFIX)/bin $(PREFIX)/include/xtr/detail $(PREFIX)/man/man1 $(PREFIX)/man/man3
	install $(TARGET) $(PREFIX)/lib
	install $(XTRCTL_TARGET) $(PREFIX)/bin
	install include/xtr/*.hpp $(PREFIX)/include/xtr/
	install include/xtr/detail/*.hpp $(PREFIX)/include/xtr/detail/
	install $(MAN3_PAGES) $(PREFIX)/man/man3
	install $(MAN1_PAGES) $(PREFIX)/man/man1

clean:
	$(RM) $(TARGET) $(TEST_TARGET) $(BENCH_TARGET) $(XTRCTL_TARGET) \
	$(OBJS) $(TEST_OBJS) $(BENCH_OBJS) $(XTRCTL_OBJS) \
	$(DEPS) $(COVERAGE_DATA)

coverage_report: $(BUILD_DIR)/coverage_report/index.html
	xdg-open $< 2> /dev/null

build/doxygen/xml/index.xml: docs-src/Doxyfile $(INCLUDES)
	@mkdir -p $(@D)
	doxygen $<

$(HTML_DOC_PAGES): $(DOCS_SRCS) build/doxygen/xml/index.xml
	sphinx-build -W -b html docs-src docs

$(MAN_PAGES): $(DOCS_SRCS) build/doxygen/xml/index.xml
	sphinx-build -W -b man docs-src docs

docs: $(HTML_DOC_PAGES) $(MAN_PAGES)

clean-docs:
	$(RM) build/doxygen/xml/index.xml $(HTML_DOC_PAGES) $(MAN_PAGES)

$(BUILD_DIR)/coverage_report/index.html: $(TEST_TARGET)
ifeq ($(COVERAGE), 0)
	$(error COVERAGE=1 option required)
endif
	$<
	@mkdir -p $(@D)
	gcovr --exclude test --exclude third_party --html-details $@ -r .

-include $(DEPS)

.PHONY: all check benchmark single_include install clean coverage_report docs
