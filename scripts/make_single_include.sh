#!/bin/sh

echo '/*' > single_include/xtr/logger.hpp
cat LICENSE >> single_include/xtr/logger.hpp
echo '*/' >> single_include/xtr/logger.hpp
grep -hEv '^//|^#include "' \
    include/xtr/timespec.hpp \
    include/xtr/tags.hpp \
    include/xtr/detail/throw.hpp \
    include/xtr/detail/retry.hpp \
    include/xtr/detail/align.hpp \
    include/xtr/detail/pagesize.hpp \
    include/xtr/detail/cpuid.hpp \
    include/xtr/detail/is_c_string.hpp \
    include/xtr/detail/file_descriptor.hpp \
    include/xtr/detail/memory_mapping.hpp \
    include/xtr/detail/mirrored_memory_mapping.hpp \
    include/xtr/detail/pause.hpp \
    include/xtr/detail/sanitize.hpp \
    include/xtr/detail/string_ref.hpp \
    include/xtr/detail/tags.hpp \
    include/xtr/detail/synchronized_ring_buffer.hpp \
    include/xtr/detail/tsc.hpp \
    include/xtr/detail/clock_ids.hpp \
    include/xtr/detail/get_time.hpp \
    include/xtr/detail/print.hpp \
    include/xtr/detail/string.hpp \
    include/xtr/detail/string_table.hpp \
    include/xtr/detail/trampolines.hpp \
    include/xtr/logger.hpp >> single_include/xtr/logger.hpp
echo '#define XTR_FUNC inline' >> single_include/xtr/logger.hpp
grep -hEv '^//|^#include "'  src/*.cpp  >> single_include/xtr/logger.hpp

