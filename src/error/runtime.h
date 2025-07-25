#ifndef _RUNTIME_H
#define _RUNTIME_H

#include <stdio.h>
#include <stdarg.h>

#include "../types/token.h"

static void runtime_error(Token* token, const char* msg, ...) {
  fprintf(stderr, "[Runtime Error] ");

  va_list args;
  va_start(args, msg);

  if (!token) {
    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
  } else {
    size_t line = token->line;
    int lexeme_len = token->lexeme.len;
    const char* lexeme = token->lexeme.str;

    // slightly different formatting, logic is the same
    if (token->type == TOKEN_TYPE_STRING) {
      fprintf(stderr, "Line %zu: %.*s - ", line, lexeme_len, lexeme);
    } else {
      fprintf(stderr, "Line %zu: \"%.*s\" - ", line, lexeme_len, lexeme);
    }

    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
  }

  va_end(args);
}

#endif
