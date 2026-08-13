CXX ?= g++
PREFIX ?= /usr/local
EXCEPTIONS ?= 1
COVERAGE ?= 0
DEBUG ?= 0
RELDEBUG ?= 0
PIC ?= 0
LTO ?= 1
URING ?= auto

BUILD_TAG := $(notdir $(CXX))
BUILD_DIR = build/$(BUILD_TAG)
VENV_DIR = build/.venv
PKG_CONFIG_DIR = build/pkgconfig
CONAN ?= $(VENV_DIR)/bin/conan
PIP ?= $(VENV_DIR)/bin/pip
SPHINX ?= $(VENV_DIR)/bin/sphinx-build
PIP_COMPILE ?= $(VENV_DIR)/bin/pip-compile
# Note that anything using $(PKG_CONFIG) must use recursive assignment,
# i.e. "FOO = x". This is because the build itself produces pkg-config
# .pc files, so non-recursive assignment would run pkg-config before
# the files exist.
PKG_CONFIG ?= PKG_CONFIG_PATH=$(PKG_CONFIG_DIR) pkg-config
PKG_CONFIG_CFLAGS = $(patsubst -I%,-isystem%,$(shell $(PKG_CONFIG) --cflags $(1)))

PKG_CONFIG_LIBS := fmt
TEST_PKG_CONFIG_LIBS := catch2
BENCH_PKG_CONFIG_LIBS := benchmark

PKG_CONFIG_FILES = \
	$(PKG_CONFIG_LIBS:%=$(PKG_CONFIG_DIR)/%.pc) \
	$(TEST_PKG_CONFIG_LIBS:%=$(PKG_CONFIG_DIR)/%.pc) \
	$(BENCH_PKG_CONFIG_LIBS:%=$(PKG_CONFIG_DIR)/%.pc)

DEBUG_CXXFLAGS = -O0 -ggdb
DEBUG_CPPFLAGS = -DXTR_ENABLE_TEST_STATIC_ASSERTIONS
OPT_CXXFLAGS = -O3 -march=native
OPT_CPPFLAGS = -DNDEBUG
COVERAGE_CXXFLAGS = --coverage -DNDEBUG

CXXFLAGS = \
	-std=c++20 -Wall -Wextra -Wconversion -Wshadow -Wcast-qual -Wformat=2 \
	-pedantic -pipe -pthread
CPPFLAGS = -MMD -MP -I include -DXTR_FUNC= \
	$(call PKG_CONFIG_CFLAGS,$(PKG_CONFIG_LIBS))
LDFLAGS = -L $(BUILD_DIR) \
	$(shell $(PKG_CONFIG) --libs-only-L $(PKG_CONFIG_LIBS))
LDLIBS = -lxtr $(shell $(PKG_CONFIG) --libs-only-l $(PKG_CONFIG_LIBS))

TEST_CPPFLAGS = $(call PKG_CONFIG_CFLAGS,$(TEST_PKG_CONFIG_LIBS))
TEST_LDFLAGS = $(shell $(PKG_CONFIG) --libs-only-L $(TEST_PKG_CONFIG_LIBS))
TEST_LDLIBS = -ldl $(shell $(PKG_CONFIG) --libs-only-l $(TEST_PKG_CONFIG_LIBS))

BENCH_CPPFLAGS = $(call PKG_CONFIG_CFLAGS,$(BENCH_PKG_CONFIG_LIBS))
BENCH_LDFLAGS = $(shell $(PKG_CONFIG) --libs-only-L $(BENCH_PKG_CONFIG_LIBS))
BENCH_LDLIBS = $(shell $(PKG_CONFIG) --libs-only-l $(BENCH_PKG_CONFIG_LIBS))

# AR is compared to 'ar' rather than set via ?= because make defines a default
# value for it, which ?= would not override.
ifneq (,$(findstring clang,$(CXX)))
	RANLIB ?= llvm-ranlib
	ifeq ($(AR),ar)
		AR := llvm-ar
	endif
else
	RANLIB ?= gcc-ranlib
	ifeq ($(AR),ar)
		AR := gcc-ar
	endif
endif

ifeq ($(PIC), 1)
	CXXFLAGS += -fPIC
	BUILD_TAG := $(BUILD_TAG)-pic
endif

ifeq ($(LTO), 1)
	CXXFLAGS += -flto=auto
	BUILD_TAG := $(BUILD_TAG)-lto
endif

# liburing is only available on Linux, so 'auto' resolves to enabled there and
# disabled everywhere else, matching the __has_include check in config.hpp.
ifeq ($(URING), auto)
	ifeq ($(shell uname -s), Linux)
		URING_ENABLED := 1
	else
		URING_ENABLED := 0
	endif
else
	URING_ENABLED := $(URING)
	CXXFLAGS += -DXTR_USE_IO_URING=$(URING)
	ifeq ($(URING), 1)
		BUILD_TAG := $(BUILD_TAG)-uring
	else
		BUILD_TAG := $(BUILD_TAG)-no-uring
	endif
endif

ifeq ($(URING_ENABLED), 1)
	PKG_CONFIG_LIBS += liburing
endif

ifeq ($(COVERAGE), 1)
	CXXFLAGS += $(COVERAGE_CXXFLAGS)
	BUILD_TAG := $(BUILD_TAG)-coverage
	COVERAGE_DATA = \
		$(SRCS:%=$(BUILD_DIR)/%.gcno) $(SRCS:%=$(BUILD_DIR)/%.gcda) \
		$(TEST_SRCS:%=$(BUILD_DIR)/%.gcno) $(TEST_SRCS:%=$(BUILD_DIR)/%.gcda)
endif

ifeq ($(RELDEBUG), 1)
	CXXFLAGS += $(DEBUG_CXXFLAGS) $(OPT_CXXFLAGS)
	CPPFLAGS += $(DEBUG_CPPFLAGS) $(OPT_CPPFLAGS)
	BUILD_TAG := $(BUILD_TAG)-reldebug
else ifeq ($(DEBUG), 1)
	CXXFLAGS += $(DEBUG_CXXFLAGS)
	CPPFLAGS += $(DEBUG_CPPFLAGS)
	BUILD_TAG := $(BUILD_TAG)-debug
else
	CXXFLAGS += $(OPT_CXXFLAGS)
	CPPFLAGS += $(OPT_CPPFLAGS)
	BUILD_TAG := $(BUILD_TAG)-release
endif

ifneq ($(SANITIZER),)
	CXXFLAGS += -fno-omit-frame-pointer -fsanitize=$(SANITIZER) -fno-sanitize-recover=all
	CPPFLAGS += -DXTR_SANITIZER_ENABLED
	LDFLAGS += -fsanitize=$(SANITIZER)
	BUILD_TAG := $(BUILD_TAG)-$(SANITIZER)-sanitizer
endif

ifeq ($(EXCEPTIONS), 0)
	CXXFLAGS += -fno-exceptions
	BUILD_TAG := $(BUILD_TAG)-no-exceptions
endif

TARGET = $(BUILD_DIR)/libxtr.a
SRCS := \
	src/command_dispatcher.cpp src/command_path.cpp src/consumer.cpp \
	src/buffer.cpp src/fd_storage.cpp src/fd_storage_base.cpp \
	src/file_descriptor.cpp src/io_uring_fd_storage.cpp src/logger.cpp \
	src/log_level.cpp src/matcher.cpp src/memory_mapping.cpp \
	src/mirrored_memory_mapping.cpp src/open.cpp src/pagesize.cpp \
	src/posix_fd_storage.cpp src/prefault.cpp src/regex_matcher.cpp \
	src/sink.cpp src/throw.cpp src/tsc.cpp \
	src/wildcard_matcher.cpp

OBJS = $(SRCS:%=$(BUILD_DIR)/%.o)

TEST_TARGET = $(BUILD_DIR)/test/test
TEST_SRCS := \
	test/align.cpp test/command_client.cpp test/command_dispatcher.cpp \
	test/fd_storage.cpp test/file_descriptor.cpp test/logger.cpp \
	test/main.cpp test/memory_mapping.cpp test/mirrored_memory_mapping.cpp \
	test/pagesize.cpp test/synchronized_ring_buffer.cpp test/throw.cpp
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
	$(wildcard include/xtr/detail/commands/*.hpp) \
	$(wildcard include/xtr/io/*.hpp) \
	$(wildcard include/xtr/io/detail/*.hpp)

$(TARGET): $(OBJS)
	$(AR) rc $@ $^
	$(RANLIB) $@

$(TEST_TARGET): $(TARGET) $(TEST_OBJS)
	$(LINK.cc) -o $@ $(TEST_OBJS) $(TEST_LDFLAGS) $(LDLIBS) $(TEST_LDLIBS)

$(BENCH_TARGET): $(TARGET) $(BENCH_OBJS)
	$(LINK.cc) -o $@ $(BENCH_OBJS) $(BENCH_LDFLAGS) $(LDLIBS) $(BENCH_LDLIBS)

$(XTRCTL_TARGET): $(TARGET) $(XTRCTL_OBJS)
	$(LINK.cc) -o $@ $(XTRCTL_OBJS) $(LDLIBS)

$(OBJS): $(BUILD_DIR)/%.cpp.o: %.cpp $(PKG_CONFIG_FILES)
	@mkdir -p $(@D)
	$(CXX) -o $@ -c $(CPPFLAGS) $(CXXFLAGS) $<

$(TEST_OBJS): $(BUILD_DIR)/%.cpp.o: %.cpp $(PKG_CONFIG_FILES)
	@mkdir -p $(@D)
	$(CXX) -o $@ -c $(CPPFLAGS) $(TEST_CPPFLAGS) $(CXXFLAGS) $<

$(BENCH_OBJS): $(BUILD_DIR)/%.cpp.o: %.cpp $(PKG_CONFIG_FILES)
	@mkdir -p $(@D)
	$(CXX) -o $@ -c $(CPPFLAGS) $(BENCH_CPPFLAGS) $(CXXFLAGS) $<

$(XTRCTL_OBJS): $(BUILD_DIR)/%.cpp.o: %.cpp $(PKG_CONFIG_FILES)
	@mkdir -p $(@D)
	$(CXX) -o $@ -c $(CPPFLAGS) $(CXXFLAGS) $<

$(CONAN) $(SPHINX) $(PIP_COMPILE) &: requirements.txt
	python3 -m venv $(VENV_DIR)
	$(PIP) install --force-reinstall -r $<

$(PKG_CONFIG_FILES) &: conanfile.py $(CONAN) conan.lock
	@mkdir -p $(PKG_CONFIG_DIR)
	$(CONAN) install --envs-generation=false --build=missing \
		--output-folder=$(PKG_CONFIG_DIR) --lockfile=conan.lock \
		--lockfile-partial $<

conan-profile: $(CONAN)
	$(CONAN) profile detect --exist-ok

pip-lock: $(PIP_COMPILE)
	$(PIP_COMPILE) --quiet --strip-extras requirements.in

conan-lock: $(CONAN)
	$(CONAN) lock create conanfile.py --lockfile-out=conan.lock

all: $(TARGET) $(TEST_TARGET) $(BENCH_TARGET) $(XTRCTL_TARGET) single_include

check: $(TEST_TARGET)
	$< --order rand

benchmark: $(BENCH_TARGET)
	$<

benchmark_cpu: $(BENCH_TARGET)
	@gov=$$(cat /sys/devices/system/cpu/cpu$(PRODUCER_CPU)/cpufreq/scaling_governor); \
	sudo cpupower --cpu $(PRODUCER_CPU),$(CONSUMER_CPU) frequency-set --governor performance; \
	$< ; \
	sudo cpupower --cpu $(PRODUCER_CPU),$(CONSUMER_CPU) frequency-set --governor $$gov

xtrctl: $(XTRCTL_TARGET)

single_include/xtr/logger.hpp: $(SRCS) $(INCLUDES)
	scripts/make_single_include.sh

single_include: single_include/xtr/logger.hpp

install: $(TARGET) $(XTRCTL_TARGET) docs
	mkdir -p $(PREFIX)/lib $(PREFIX)/bin $(PREFIX)/include/xtr/detail/commands $(PREFIX)/include/xtr/io/detail $(PREFIX)/man/man1 $(PREFIX)/man/man3
	install $(TARGET) $(PREFIX)/lib
	install $(XTRCTL_TARGET) $(PREFIX)/bin
	install -m 644 include/xtr/*.hpp $(PREFIX)/include/xtr/
	install -m 644 include/xtr/detail/*.hpp $(PREFIX)/include/xtr/detail/
	install -m 644 include/xtr/detail/commands/*.hpp $(PREFIX)/include/xtr/detail/commands/
	install -m 644 include/xtr/io/*.hpp $(PREFIX)/include/xtr/io/
	install -m 644 include/xtr/io/detail/*.hpp $(PREFIX)/include/xtr/io/detail/
	install -m 644 $(MAN3_PAGES) $(PREFIX)/man/man3
	install -m 644 $(MAN1_PAGES) $(PREFIX)/man/man1

clean:
	$(RM) $(TARGET) $(TEST_TARGET) $(BENCH_TARGET) $(XTRCTL_TARGET) \
	$(OBJS) $(TEST_OBJS) $(BENCH_OBJS) $(XTRCTL_OBJS) \
	$(DEPS) $(COVERAGE_DATA)

distclean:
	$(RM) -r build

coverage_report: $(BUILD_DIR)/coverage_report/index.html
	xdg-open $< 2> /dev/null

build/doxygen/xml/index.xml: docs-src/Doxyfile $(INCLUDES)
	@mkdir -p $(@D)
	doxygen $<

$(HTML_DOC_PAGES) &: $(DOCS_SRCS) build/doxygen/xml/index.xml $(SPHINX)
	$(SPHINX) -W -b html docs-src docs

$(MAN_PAGES) &: $(DOCS_SRCS) build/doxygen/xml/index.xml $(SPHINX)
	$(SPHINX) -W -b man docs-src docs

docs: $(HTML_DOC_PAGES) $(MAN_PAGES)

clean-docs:
	$(RM) build/doxygen/xml/index.xml $(HTML_DOC_PAGES) $(MAN_PAGES)

$(BUILD_DIR)/coverage_report/index.html: $(TEST_TARGET)
ifeq ($(COVERAGE), 0)
	$(error COVERAGE=1 option required)
endif
	$<
	@mkdir -p $(@D)
	gcovr --exclude test --html-details $@ -r .

-include $(DEPS)

.PHONY: all check benchmark benchmark_cpu conan-profile conan-lock pip-lock \
	single_include install clean clean-docs distclean coverage_report docs \
	xtrctl
