#ifndef LOG_H
#define LOG_H

#include <stdio.h>

/* Program logging */
#define LOG(level, fmt, ...) \
    do { \
        if (level <= CURRENT_LOG_LEVEL) { \
            fprintf(stderr, "[%s] %s:%d: " fmt "\n", \
                    log_level_str(level), __FILE__, __LINE__, ##__VA_ARGS__); \
        } \
    } while (0)

#define LOG_ERR(fmt, ...)   LOG(L_ERROR, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  LOG(L_WARN,  fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  LOG(L_INFO,  fmt, ##__VA_ARGS__)
#define LOG_DBG(fmt, ...)   LOG(L_DEBUG, fmt, ##__VA_ARGS__)

#define CURRENT_LOG_LEVEL L_DEBUG

/* Compiler logging */
#define EMIT(tag, line, fmt, ...) \
    do { \
        fprintf(stderr, "[%s] Line %d: " fmt "\n", \
                emit_str(tag), line, ##__VA_ARGS__); \
    } while (0)

#define EMIT_SYNTAX_ERR(line, fmt, ...) EMIT(E_SYNTAX_ERR, line, fmt, ##__VA_ARGS__)
#define EMIT_SEMANTIC_ERR(line, fmt, ...) EMIT(E_SEMANTIC_ERR, line, fmt, ##__VA_ARGS__)

typedef enum {
    L_ERROR = 0,
    L_WARN,
    L_INFO,
    L_DEBUG,
} LogLevel;

typedef enum {
    E_SYNTAX_ERR = 100,
    E_SEMANTIC_ERR
} EmitTag;

static inline const char *log_level_str(LogLevel level) {
    switch (level) {
        case L_ERROR: return "ERROR";
        case L_WARN:  return "WARN";
        case L_INFO:  return "INFO";
        case L_DEBUG: return "DEBUG";
        default:      return "LOG";
    }
}

static inline const char *emit_str(EmitTag tag) {
    switch (tag) {
        case E_SYNTAX_ERR:    return "Syntax error";
        case E_SEMANTIC_ERR:  return "Semantic error";
        default:              return "Compile error";
    }
}

#endif
