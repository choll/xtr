#ifndef XTR_DETAIL_CONFIG_HPP
#define XTR_DETAIL_CONFIG_HPP

#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define XTR_THREAD_SANITIZER_ENABLED
#endif
#endif

#endif
