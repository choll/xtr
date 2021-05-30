#!/bin/sh

target=single_include/xtr/logger.hpp

echo '/*' > $target
cat LICENSE >> $target
echo '*/' >> $target
echo '\n#ifndef XTR_LOGGER_HPP' >> $target
echo '#define XTR_LOGGER_HPP\n' >> $target

for file in \
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
    include/xtr/detail/strzcpy.hpp \
    include/xtr/log_level.hpp \
    include/xtr/detail/commands/frame.hpp \
    include/xtr/detail/commands/pattern.hpp \
    include/xtr/detail/commands/message_id.hpp \
    include/xtr/detail/commands/requests.hpp \
    include/xtr/detail/commands/responses.hpp \
    include/xtr/detail/commands/connect.hpp \
    include/xtr/detail/commands/send.hpp \
    include/xtr/detail/commands/recv.hpp \
    include/xtr/detail/commands/command_dispatcher_fwd.hpp \
    include/xtr/detail/commands/command_dispatcher.hpp \
    include/xtr/detail/commands/matcher.hpp \
    include/xtr/detail/commands/regex_matcher.hpp \
    include/xtr/detail/commands/wildcard_matcher.hpp \
    include/xtr/logger.hpp;
do
    sed '$d' $file|grep -Ev '^ *//|^#include "|^#ifndef XTR_.*HPP|^#define XTR_.*HPP' >> $target
done

grep -hEv '^ *//|^#include "' src/*.cpp  >> $target
echo '#endif' >> $target

sed -i 's/XTR_FUNC/inline/g' $target

clang-format-12 -i single_include/xtr/logger.hpp
