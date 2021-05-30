#ifndef XTR_DETAIL_CONFIG_HPP
#define XTR_DETAIL_CONFIG_HPP

#if defined(__SANITIZE_THREAD__)
#define XTR_THREAD_SANITIZER_ENABLED
#elif defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define XTR_THREAD_SANITIZER_ENABLED
#endif
#endif

#endif
