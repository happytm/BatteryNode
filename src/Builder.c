#include "JsonLogger.h"

#define concat(src, src_size)               \
  do {                                      \
    if (json_len + src_size > buf_size) {   \
      return JSON_ERR_BUF_SIZE;             \
    }                                       \
    memcpy(&json[json_len], src, src_size); \
    json_len += src_size - 1;               \
  } while (0)

#define concat_const(src) concat(src, sizeof(src))
#define concat_var(src) concat(src, strlen(src) + 1)

#define addKey(key)      \
  do {                   \
    concat_var(key);     \
    concat_const("\":"); \
  } while (0)

#define addInt(value)                 \
  do {                                \
    sprintf(temp, "%" PRId32, value); \
    concat_var(temp);                 \
  } while (0)

#define addDouble(value, precisionChar)                                                     \
  do {                                                                                      \
    uint8_t digits = precisionChar >= 'a' ? precisionChar - 'a' + 10 : precisionChar - '0'; \
    if (digits > 17) {                                                                      \
      digits = 17;                                                                          \
    }                                                                                       \
    sprintf(temp, "%.*g", digits, value);                                                   \
    concat_var(temp);                                                                       \
  } while (0)

#define addBool(value)       \
  do {                       \
    if (value) {             \
      concat_const("true");  \
    } else {                 \
      concat_const("false"); \
    }                        \
  } while (0)

#define addOther(value)     \
  do {                      \
    if (value) {            \
      concat_var(value);    \
    } else {                \
      concat_const("null"); \
    }                       \
  } while (0)

#define addStr(value)                                         \
  do {                                                        \
    char* escaped1 = str_replace((char*)value, "\\", "\\\\"); \
    char* source1 = escaped1 ? escaped1 : (char*)value;       \
    char* escaped2 = str_replace(source1, "\n", "\\n");       \
    char* source2 = escaped2 ? escaped2 : source1;            \
    char* escaped3 = str_replace(source2, "\"", "\\\"");      \
    char* source3 = escaped3 ? escaped3 : source2;            \
    char* escaped4 = str_replace(source3, "\b", "\\b");       \
    char* source4 = escaped4 ? escaped4 : source3;            \
    char* escaped5 = str_replace(source4, "\f", "\\f");       \
    char* source5 = escaped5 ? escaped5 : source4;            \
    char* escaped6 = str_replace(source5, "\r", "\\r");       \
    char* source6 = escaped6 ? escaped6 : source5;            \
    char* escaped7 = str_replace(source6, "\t", "\\t");       \
    char* source7 = escaped7 ? escaped7 : source6;            \
    concat_var(source7);                                      \
    if (escaped1) {                                           \
      free(escaped1);                                         \
    }                                                         \
    if (escaped2) {                                           \
      free(escaped2);                                         \
    }                                                         \
    if (escaped3) {                                           \
      free(escaped3);                                         \
    }                                                         \
    if (escaped4) {                                           \
      free(escaped4);                                         \
    }                                                         \
    if (escaped5) {                                           \
      free(escaped5);                                         \
    }                                                         \
    if (escaped6) {                                           \
      free(escaped6);                                         \
    }                                                         \
    if (escaped7) {                                           \
      free(escaped7);                                         \
    }                                                         \
  } while (0)

enum ArrayType {
  INT_ARRAY,
  DOUBLE_ARRAY,
  BOOL_ARRAY,
  STRING_ARRAY,
  OTHER_ARRAY,
};

// inspired by https://stackoverflow.com/questions/779875/what-is-the-function-to-replace-string-in-c
// You must free the result if result is non-NULL.
char* str_replace(char* orig, const char* rep, const char* with) {
  char* result;   // the return string
  char* ins;      // the next insert point
  char* tmp;      // varies
  int len_rep;    // length of rep (the string to remove)
  int len_with;   // length of with (the string to replace rep with)
  int len_front;  // distance between rep and end of last rep
  int count;      // number of replacements
  int first;

  // sanity checks and initialization
  if (!orig || !rep)
    return NULL;
  len_rep = strlen(rep);
  if (len_rep == 0)
    return NULL;  // empty rep causes infinite loop during count
  if (!with)
    with = "";
  len_with = strlen(with);

  // count the number of replacements needed
  if (!(ins = strstr(orig, rep))) {
    return NULL;  // no replacing needed
  }

  int expand = len_with > len_rep;

  if (expand) {
    char* moreIns = ins + len_rep;
    for (count = 1; (tmp = strstr(moreIns, rep)); ++count) {
      moreIns = tmp + len_rep;
    }
    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);
    if (!result)
      return NULL;
  } else {  // inplace
    tmp = ins;
    result = NULL;
    first = 1;
  }

  // first time through the loop, all the variable are set correctly
  // from here on,
  //    tmp points to the end of the result string
  //    ins points to the next occurrence of rep in orig
  //    orig points to the remainder of orig after "end of rep"
  do {
    len_front = ins - orig;
    if (expand) {
      tmp = memcpy(tmp, orig, len_front) + len_front;
      tmp = memcpy(tmp, with, len_with) + len_with;
    } else {
      if (first) {
        first = 0;
      } else {
        tmp = memmove(tmp, orig, len_front) + len_front;
      }
      tmp = memmove(tmp, with, len_with) + len_with;
    }
    orig += len_front + len_rep;  // move to next "end of rep"
  } while ((ins = strstr(orig, rep)));

  size_t len = strlen(orig) + 1;
  if (expand) {
    memcpy(tmp, orig, len);
  } else {
    memmove(tmp, orig, len);
  }
  return result;
}

int build_json(char* json, size_t buf_size, const char* item, ...) {
  va_list args;
  va_start(args, item);
  int ret = vbuild_json(json, buf_size, item, args);
  va_end(args);
  return ret;
}

int vbuild_json(char* json, size_t buf_size, const char* item, va_list arg) {
  if (!json) {
    return JSON_ERR_BUF_SIZE;
  }
  char temp[25];  // enough for int, double conversion
  json[0] = '\0';
  int json_len = 0;
  int8_t buildFragment = 0;
  int8_t firstItem = 1;
  int8_t lastValueNeedsQuote = 0;
  int8_t isNoKeyArray = 0;
  int braceDiff = 0;

  if (item[0] == '-' && item[1] == '{') {
    buildFragment = 1;
    concat_const("+|");
    item = va_arg(arg, const char*);
  }

  while (item) {
    int8_t isFragment = (item[0] == '+' && item[1] == '|');
    if (isFragment && item[2] == '\0') {
      item = va_arg(arg, const char*);
      continue;
    }
    int8_t isEndObject = (item[0] == '}' && item[1] == '|');
    int8_t isArray = (item[1] == '[' && (item[0] == 'i' || item[0] == 'b' || item[0] == 'o' || item[0] == 's')) ||
                     (item[2] == '[' && item[0] == 'f');
    enum ArrayType array = STRING_ARRAY;
    const char* arrayKey = &item[2];
    if (isArray) {
      switch (item[0]) {
        case 'i':
          array = INT_ARRAY;
          break;
        case 'f':
          array = DOUBLE_ARRAY;
          arrayKey = &item[3];
          break;
        case 'b':
          array = BOOL_ARRAY;
          break;
        case 'o':
          array = OTHER_ARRAY;
          break;
      }

      if (*arrayKey == '\0') {
        isNoKeyArray = 1;
      }
    }

    if (firstItem) {
      if (isFragment || isEndObject) {
        if (!buildFragment) {
          concat_const("{");
        }
      } else {
        if (buildFragment) {
          concat_const("\"");
        } else if (!isNoKeyArray) {
          concat_const("{\"");
        }
      }
      firstItem = 0;
    } else if (!isEndObject) {
      if (lastValueNeedsQuote) {
        if (isFragment) {
          if (item[2] != '\0') {
            concat_const("\",");
          }
        } else {
          concat_const("\",\"");
        }
        lastValueNeedsQuote = 0;
      } else {
        if (isFragment) {
          if (item[2] != '\0') {
            concat_const(",");
          }
        } else {
          concat_const(",\"");
        }
      }
    }
    if (item[1] == '|' && item[0] == 'i') {  // integer
      addKey(&item[2]);
      int32_t value = va_arg(arg, int32_t);
      addInt(value);
    } else if (item[2] == '|' && item[0] == 'f') {  // double
      addKey(&item[3]);
      double value = va_arg(arg, double);
      addDouble(value, item[1]);
    } else if (item[1] == '|' && item[0] == 'b') {  // boolean
      addKey(&item[2]);
      int32_t value = va_arg(arg, int32_t);
      addBool(value);
    } else if (item[1] == '|' && item[0] == 'o') {  // others (no adding quotes or conversion)
      addKey(&item[2]);
      const char* value = va_arg(arg, const char*);
      addOther(value);
    } else if (item[1] == '|' && item[0] == '{') {  // begin object
      addKey(&item[2]);
      firstItem = 1;
      braceDiff += 1;
    } else if (isEndObject) {  // end object
      if (braceDiff < 1) {
        return JSON_ERR_BRACES_MISMATCH;
      }
      if (lastValueNeedsQuote) {
        concat_const("\"}");
        lastValueNeedsQuote = 0;
      } else {
        concat_const("}");
      }
      braceDiff -= 1;
    } else if (isFragment) {  // insert fragment
      concat_var(&item[2]);
    } else if (isArray) {
      if (!isNoKeyArray) {
        addKey(arrayKey);
      }

      int32_t numOfArrayItems = va_arg(arg, int32_t);

      void** list;
      switch (array) {
        case INT_ARRAY: {
          list = (void**)va_arg(arg, int32_t*);
          break;
        }
        case DOUBLE_ARRAY: {
          list = (void**)va_arg(arg, double*);
          break;
        }
        case BOOL_ARRAY: {
          list = (void**)va_arg(arg, int32_t*);
          break;
        }
        case OTHER_ARRAY: {
          list = (void**)va_arg(arg, const char**);
          break;
        }
        default: {
          list = (void**)va_arg(arg, const char**);
          break;
        }
      }

      int8_t arrayItemsNeedsQuote = 0;
      if (array == STRING_ARRAY && numOfArrayItems > 0) {
        concat_const("[\"");
        arrayItemsNeedsQuote = 1;
      } else {
        concat_const("[");
      }

      for (int32_t i = 0; i < numOfArrayItems; i++) {
        if (i != 0) {
          if (arrayItemsNeedsQuote) {
            concat_const("\",\"");
          } else {
            concat_const(",");
          }
        }
        switch (array) {
          case INT_ARRAY: {
            addInt(((int32_t*)list)[i]);
            break;
          }
          case DOUBLE_ARRAY: {
            addDouble(((double*)list)[i], item[1]);
            break;
          }
          case BOOL_ARRAY: {
            addBool(((int32_t*)list)[i]);
            break;
          }
          case OTHER_ARRAY: {
            addOther(((const char**)list)[i]);
            break;
          }
          default: {
            addStr(((const char**)list)[i]);
            break;
          }
        }
      }

      if (arrayItemsNeedsQuote) {
        concat_const("\"]");
      } else {
        concat_const("]");
      }
    } else {  // string
      int itemIsValue = 0;
      const char* value = va_arg(arg, const char*);
      if (!value) {
        value = item;
        concat_const(EMPTY_KEY "\":\"");
        itemIsValue = 1;
      } else {
        const char* key = (item[1] == '|' && item[0] == 's') ? item + 2 : item;
        concat_var(key);
        concat_const("\":\"");
      }
      addStr(value);
      lastValueNeedsQuote = 1;
      if (itemIsValue) {
        break;
      }
    }
    item = va_arg(arg, const char*);
  }

  if (buildFragment) {
    if (lastValueNeedsQuote) {
      concat_const("\"");
    }
  } else if (!isNoKeyArray) {
    if (lastValueNeedsQuote) {
      concat_const("\"}");
    } else {
      concat_const("}");
    }
  }

  if (braceDiff != 0) {
    return JSON_ERR_BRACES_MISMATCH;
  }
  return json_len;
}

#ifdef JSON_BUILDER_TEST
// gcc -Os -DJSON_BUILDER_TEST src/*.c; ./a.out; rm ./a.out

#include <assert.h>

int main() {
  char buf256[256], buf128[128], buf64[64];

  int len = json(buf256, "StrK", "StrV", "{|ObjK", "i|IntK", 0xffffffff, "f7|FloatK", 1.234567890,
                 "}|", "b|BoolK", 1, "o|NullK", "null", "ValueOnly");
  printf("%s len=%d\n", buf256, len);
  assert(!strcmp(buf256,
                 "{\"StrK\":\"StrV\",\"ObjK\":{\"IntK\":-1,\"FloatK\":1.234568},\
\"BoolK\":true,\"NullK\":null,\"_\":\"ValueOnly\"}"));
  assert(len == strlen(buf256));

  len = json(buf64, "-{", "s|i|StrK2", "StrV2", "i|IntK2", 8);
  printf("%s\n", buf64);
  assert(!strcmp(buf64, "+|\"i|StrK2\":\"StrV2\",\"IntK2\":8"));
  assert(len == strlen(buf64));

  char* buf512 = (char*)malloc(512);
  len = jsonHeap(buf512, 512, "o|ObjK2", buf256, buf64);
  printf("%s\n", buf512);
  assert(!strcmp(buf512,
                 "{\"ObjK2\":{\"StrK\":\"StrV\",\"ObjK\":{\"IntK\":-1,\"FloatK\":1.234568},\
\"BoolK\":true,\"NullK\":null,\"_\":\"ValueOnly\"},\"i|StrK2\":\"StrV2\",\"IntK2\":8}"));
  assert(len == strlen(buf512));

  len = jsonHeap(buf512, 512, "+|", "o|ObjK2", buf256, "+|");
  printf("%s\n", buf512);
  assert(!strcmp(buf512,
                 "{\"ObjK2\":{\"StrK\":\"StrV\",\"ObjK\":{\"IntK\":-1,\"FloatK\":1.234568},\
\"BoolK\":true,\"NullK\":null,\"_\":\"ValueOnly\"}}"));
  assert(len == strlen(buf512));

  free(buf512);

  char* strArray[] = {"StrV3", "Str\"V4\""};
  int32_t intArray[] = {0, -2147483648, 2147483647};
  double floatArray[] = {-0x1.fffffffffffffp+1023, -2.2250738585072014e-308};
  int32_t boolArray[] = {0, 1};

  len = json(buf256, "s[StrArrayK", 2, strArray, "i[IntArrayK", 3, intArray,
             "fh[FloatArrayK", 2, floatArray, "b[BoolArrayK", 2, boolArray);
  printf("%s\n", buf256);
  assert(!strcmp(buf256,
                 "{\"StrArrayK\":[\"StrV3\",\"Str\\\"V4\\\"\"],\"IntArrayK\":[0,-2147483648,2147483647],\
\"FloatArrayK\":[-1.7976931348623157e+308,-2.2250738585072014e-308],\"BoolArrayK\":[false,true]}"));
  assert(len == strlen(buf256));

  len = json(buf256, "s[StrArrayK", 0, NULL, "i[IntArrayK", 0, NULL,
             "fh[FloatArrayK", 0, NULL, "b[BoolArrayK", 0, NULL);
  printf("%s\n", buf256);
  assert(!strcmp(buf256,
                 "{\"StrArrayK\":[],\"IntArrayK\":[],\
\"FloatArrayK\":[],\"BoolArrayK\":[]}"));
  assert(len == strlen(buf256));

  char* otherArray[] = {"\"NoKeyArray\"", "[]", "{}", "null", "40", "5.55", "false"};
  len = json(buf128, "o[", 7, otherArray);
  printf("%s\n", buf128);
  assert(!strcmp(buf128, "[\"NoKeyArray\",[],{},null,40,5.55,false]"));
  assert(len == strlen(buf128));

  len = json(buf64, "\\\n\b\t\r\f\"");
  printf("%s\n", buf64);
  assert(!strcmp(buf64, "{\"_\":\"\\\\\\n\\b\\t\\r\\f\\\"\"}"));
  assert(len == strlen(buf64));

  len = json(buf64, "-{");
  printf("%s\n", buf64);
  assert(!strcmp(buf64, "+|"));
  assert(len == strlen(buf64));

  // corner cases

  len = json(buf64, "");
  printf("%s\n", buf64);
  assert(!strcmp(buf64, "{\"_\":\"\"}"));
  assert(len == strlen(buf64));

  len = json(buf64, "a", "");
  printf("%s\n", buf64);
  assert(!strcmp(buf64, "{\"a\":\"\"}"));
  assert(len == strlen(buf64));

  len = json(buf64, "x|");
  printf("%s\n", buf64);
  assert(!strcmp(buf64, "{\"_\":\"x|\"}"));
  assert(len == strlen(buf64));

  len = json(buf64, "x|", "value");
  printf("%s\n", buf64);
  assert(!strcmp(buf64, "{\"x|\":\"value\"}"));
  assert(len == strlen(buf64));

  len = json(buf64, "s|i|", "value");
  printf("%s\n", buf64);
  assert(!strcmp(buf64, "{\"i|\":\"value\"}"));
  assert(len == strlen(buf64));

  len = json(buf64, "x[");
  printf("%s\n", buf64);
  assert(!strcmp(buf64, "{\"_\":\"x[\"}"));
  assert(len == strlen(buf64));

  len = json(buf64, "{|key", "+|", "}|");
  printf("%s\n", buf64);
  assert(!strcmp(buf64, "{\"key\":{}}"));
  assert(len == strlen(buf64));

  len = json(buf64, "-{", "+|\"k1\":\"v1\"", "k2", "v2");
  printf("%s\n", buf64);
  assert(!strcmp(buf64, "+|\"k1\":\"v1\",\"k2\":\"v2\""));
  assert(len == strlen(buf64));

  len = json(buf64, "k1", "v2", "+|\"i|k2\":1234");
  printf("%s\n", buf64);
  assert(!strcmp(buf64, "{\"k1\":\"v2\",\"i|k2\":1234}"));
  assert(len == strlen(buf64));

  // error conditions
  len = json(NULL, "k", "v");
  assert(len == JSON_ERR_BUF_SIZE);

  len = json(buf64, "01234567890123456789012345678901234567890123456789012345678901234567890123456789", "v");
  printf("%s\n", buf64);
  assert(len == JSON_ERR_BUF_SIZE);

  len = json(buf64, "k", "01234567890123456789012345678901234567890123456789012345678901234567890123456789");
  printf("%s\n", buf64);
  assert(len == JSON_ERR_BUF_SIZE);

  len = json(buf64, "{|ObjK1", "{|ObjK2", "}|");
  printf("%s\n", buf64);
  assert(len == JSON_ERR_BRACES_MISMATCH);

  len = json(buf64, "}|", "{|ObjK2");
  printf("%s\n", buf64);
  assert(len == JSON_ERR_BRACES_MISMATCH);

  return 0;
}
#endif