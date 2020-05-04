#include "JsonLogger.h"

const char* LOG_LEVELS[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

static void (*senders[5])(int level, const char* json, int len);
static int number_of_senders = 0;

void logAddSender(void (*sender)(int level, const char* json, int len)) {
  senders[number_of_senders] = sender;
  number_of_senders++;
}

void logModifyForHuman(int level, char* mod) {
#ifdef LOG_TIME_KEY
  str_replace(mod, "\"" LOG_TIME_KEY "\":\"", "");
#endif

#ifdef LOG_ID_KEY
#ifdef LOG_TIME_KEY
  str_replace(mod, "\",\"" LOG_ID_KEY "\":", "");
#else
  str_replace(mod, "\"" LOG_ID_KEY "\":", "");
#endif
  char buf[64];
  sprintf(buf, "\"%s", getLogId());
  str_replace(mod, buf, "");
#endif

  if (level < sizeof(LOG_LEVELS) / sizeof(LOG_LEVELS[0])) {
    char buf[64];
#if defined(LOG_TIME_KEY) || defined(LOG_ID_KEY)
    sprintf(buf, ",\"" LOG_LEVEL_KEY "\":%d,", level);
#else
    sprintf(buf, "\"" LOG_LEVEL_KEY "\":%d,", level);
#endif
    str_replace(mod, buf, LOG_LEVELS[level]);
  }

#ifdef LOG_SOURCE_KEY
  str_replace(mod, "\"" LOG_SOURCE_KEY "\":\"", " ");
  str_replace(mod, "\",\"" LOG_FUNC_KEY "\":\"", " ");
#endif
  str_replace(mod, "\\\"", "'");
  str_replace(mod, "\"", " ");
}

void log_json(int level, const char* placeholder, ...) {
  char fragment[64], json[LOG_MAX_LEN];
  json(fragment, "-{",
#ifdef LOG_TIME_KEY
       LOG_TIME_KEY, getLogTime(),
#endif
#ifdef LOG_ID_KEY
       LOG_ID_KEY, getLogId(),
#endif
       "i|" LOG_LEVEL_KEY, level);
  va_list args;
  va_start(args, placeholder);
  int len = vbuild_json(json, LOG_MAX_LEN, fragment, args);
  va_end(args);

  for (int i = 0; i < number_of_senders; i++) {
    if (len < 0) {
      char error[64];
      len = json(error, "i|len", len, "vbuild_json() failed in log_json()");
      senders[i](LEVEL_ERROR, error, len);
    }
    senders[i](level, json, len);
  }
}

#ifdef LOGGER_TEST

// gcc -Os -DLOGGER_TEST '-DLOG_ID_KEY="i"' '-DLOG_TIME_KEY="t"' '-DLOG_SOURCE_KEY="s"' src/*.c; ./a.out; rm ./a.out
// gcc -Os -DLOGGER_TEST '-DLOG_TIME_KEY="t"' src/*.c; ./a.out; rm ./a.out
// gcc -Os -DLOGGER_TEST '-DLOG_ID_KEY="i"' src/*.c; ./a.out; rm ./a.out
// gcc -Os -DLOGGER_TEST '-DLOG_SOURCE_KEY="s"' src/*.c; ./a.out; rm ./a.out
// gcc -Os -DLOGGER_TEST '-DLOG_MIN_LEVEL=0' src/*.c; ./a.out; rm ./a.out

const char* getLogTime() {
  return "1970-01-01T00:00:00Z";
}

const char* getLogId() {
  return "DEVICE UUID";
}

void send_console(int level, const char* json, int len) {
  char mod[LOG_MAX_LEN];
  memcpy(mod, json, len + 1);

  logModifyForHuman(level, mod);

  printf("terminal: %s\n", mod);
}

void send_file(int level, const char* json, int len) {
  if (level >= LEVEL_INFO) {
    printf("mqtt    : %s\n", json);
  }
}

int main() {
  logAddSender(send_console);
  logAddSender(send_file);

  logTrace("should not be logged at all if LOG_MIN_LEVEL is not changed to 0");
  printf("\n");
  logDebug("log to terminal, but not to mqtt");
  printf("\n");
  logInfo("i|status", -1, "f5|pi", 3.14159, "log to both \"terminal\" and \"mqtt\"");
  printf("\n");
  logWarn("Warning");
  printf("\n");
  logError("Error");
  printf("\n");
  logFatal("Fatal");
  printf("\n");
  logLevel(8, "DATA");

  return 0;
}

#endif