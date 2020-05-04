#ifndef json_builder_h
#define json_builder_h

#define json(buf, ...) build_json(buf, sizeof(buf), __VA_ARGS__, NULL)     // returns json length if all good, negative number if error
#define jsonHeap(buf, size, ...) build_json(buf, size, __VA_ARGS__, NULL)  // same as json() but user supplies buffer size for malloc-ed buffer

#define logFatal(...) logJson(LEVEL_FATAL, __VA_ARGS__)
#define logError(...) logJson(LEVEL_ERROR, __VA_ARGS__)
#define logWarn(...) logJson(LEVEL_WARN, __VA_ARGS__)
#define logInfo(...) logJson(LEVEL_INFO, __VA_ARGS__)
#define logDebug(...) logJson(LEVEL_DEBUG, __VA_ARGS__)
#define logTrace(...) logJson(LEVEL_TRACE, __VA_ARGS__)
#define logLevel(level, ...) logJson(level, __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

// define LOG_ID_KEY (e.g. -D LOG_ID_KEY="i") and implement getLogId() if you want to log id
//#define LOG_ID_KEY "i"
#ifdef LOG_ID_KEY
extern const char* getLogId();
#endif

// define LOG_TIME_KEY (e.g. -D LOG_TIME_KEY="t") and implement getLogTime() if you want to log id
//#define LOG_TIME_KEY "t"
#ifdef LOG_TIME_KEY
extern const char* getLogTime();
#endif

// define LOG_SOURCE_KEY (e.g. -D LOG_SOURCE_KEY="s") if you want to log source file, line # and function name
//#define LOG_SOURCE_KEY "s"

void logAddSender(void (*sender)(int level, const char* json, int len));
void logModifyForHuman(int level, char* json);

#ifdef __cplusplus
}
#endif

#define JSON_ERR_BUF_SIZE -1
#define JSON_ERR_BRACES_MISMATCH -2

#ifndef LOG_MIN_LEVEL
#define LOG_MIN_LEVEL 1
#endif

#ifndef LOG_MAX_LEN
#define LOG_MAX_LEN 512
#endif

#ifndef EMPTY_KEY
#define EMPTY_KEY "_"
#endif

#ifndef LOG_FUNC_KEY
#define LOG_FUNC_KEY "f"
#endif

#ifndef LOG_LEVEL_KEY
#define LOG_LEVEL_KEY "l"
#endif

#define LEVEL_FATAL 5
#define LEVEL_ERROR 4
#define LEVEL_WARN 3
#define LEVEL_INFO 2
#define LEVEL_DEBUG 1
#define LEVEL_TRACE 0

#ifdef LOG_SOURCE_KEY
#define logJson(level, ...) \
  if (level >= LOG_MIN_LEVEL) log_json(level, "", LOG_SOURCE_KEY, __FILE__ ":" TOSTRING(__LINE__), LOG_FUNC_KEY, __func__, __VA_ARGS__, NULL)
#else
#define logJson(level, ...) \
  if (level >= LOG_MIN_LEVEL) log_json(level, "", __VA_ARGS__, NULL)
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int build_json(char* json, size_t buf_size, const char* item, ...);
int vbuild_json(char* json, size_t buf_size, const char* item, va_list args);

void log_json(int level, const char* placeholder, ...);

char* str_replace(char* orig, const char* rep, const char* with);

#ifdef __cplusplus
}
#endif

#endif  // json_builder_h
