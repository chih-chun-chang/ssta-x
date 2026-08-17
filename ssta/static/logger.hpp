#pragma once

#include <ssta/utility/logger.hpp>
#include <ssta/policy.hpp>

// Global declaration and macro usage.
inline Logger logger(log_file());


#define OT_LOG_REMOVE_FIRST_HELPER(N, ...) __VA_ARGS__
#define OT_LOG_GET_FIRST_HELPER(N, ...) N
#define OT_LOG_GET_FIRST(...) OT_LOG_GET_FIRST_HELPER(__VA_ARGS__)
#define OT_LOG_REMOVE_FIRST(...) OT_LOG_REMOVE_FIRST_HELPER(__VA_ARGS__)

#define OT_LOGTO(...) logger.redir  (__VA_ARGS__)
#define OT_LOG(...)   logger.raw    (__VA_ARGS__)
#define OT_LOGD(...)  logger.debug  (__FILE__, __LINE__, __VA_ARGS__, '\n')
#define OT_LOGI(...)  logger.info   (__FILE__, __LINE__, __VA_ARGS__, '\n')
#define OT_LOGW(...)  logger.warning(__FILE__, __LINE__, __VA_ARGS__, '\n')
#define OT_LOGE(...)  logger.error  (__FILE__, __LINE__, __VA_ARGS__, '\n')
#define OT_LOGF(...)  logger.fatal  (__FILE__, __LINE__, __VA_ARGS__, '\n')

#define OT_LOG_IF(...) if(OT_LOG_GET_FIRST(__VA_ARGS__)) {          \
                         OT_LOG(OT_LOG_REMOVE_FIRST(__VA_ARGS__));  \
                       }

#define OT_LOGD_IF(...) if(OT_LOG_GET_FIRST(__VA_ARGS__)) {           \
                          OT_LOGD(OT_LOG_REMOVE_FIRST(__VA_ARGS__));  \
                        }

#define OT_LOGI_IF(...) if(OT_LOG_GET_FIRST(__VA_ARGS__)) {           \
                          OT_LOGI(OT_LOG_REMOVE_FIRST(__VA_ARGS__));  \
                        }

#define OT_LOGW_IF(...) if(OT_LOG_GET_FIRST(__VA_ARGS__)) {           \
                          OT_LOGW(OT_LOG_REMOVE_FIRST(__VA_ARGS__));  \
                        }

#define OT_LOGE_IF(...) if(OT_LOG_GET_FIRST(__VA_ARGS__)) {           \
                          OT_LOGE(OT_LOG_REMOVE_FIRST(__VA_ARGS__));  \
                        }

#define OT_LOGF_IF(...) if(OT_LOG_GET_FIRST(__VA_ARGS__)) {           \
                          OT_LOGF(OT_LOG_REMOVE_FIRST(__VA_ARGS__));  \
                        }

#define OT_LOGW_RIF(...) if(OT_LOG_GET_FIRST(__VA_ARGS__)) {           \
                           OT_LOGW(OT_LOG_REMOVE_FIRST(__VA_ARGS__));  \
                           return;                                     \
                         }

#define OT_LOGE_RIF(...) if(OT_LOG_GET_FIRST(__VA_ARGS__)) {           \
                           OT_LOGE(OT_LOG_REMOVE_FIRST(__VA_ARGS__));  \
                           return;                                     \
                         }







