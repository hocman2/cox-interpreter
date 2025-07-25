#ifndef _ANALYSIS_H
#define _ANALYSIS_H

#include <stdio.h>
#include <stdarg.h>

#include "../types/token.h"

enum AnalysisErrorType {ANAL_ERROR_SYNTAX, ANAL_ERROR_INTERNAL_LOGIC};

static void _analysis_error_internal(Token* t, enum AnalysisErrorType err_type, const char* msg, va_list fmt_args) {

  switch (err_type) {
    case ANAL_ERROR_SYNTAX:
      fprintf(stderr, "[Syntax Error] ");
    break;
    case ANAL_ERROR_INTERNAL_LOGIC:
      fprintf(stderr, "[Internal logic Error] ");
    break;
  }

  if (!t) {
    vfprintf(stderr, msg, fmt_args);
    fprintf(stderr, "\n");
    return;
  }

  size_t line = t->line;
  int lexeme_len = t->lexeme.len;
  const char* lexeme = t->lexeme.str;
  
  if (t->type == TOKEN_TYPE_STRING) {
    fprintf(stderr, "Line %zu: %.*s - ", line, lexeme_len, lexeme);
  } else if (t->type == TOKEN_TYPE_EOF) {
    fprintf(stderr, "Line %zu: ", line-1);
  } else {
    fprintf(stderr, "Line %zu: \"%.*s\" - ", line, lexeme_len, lexeme);
  }

  vfprintf(stderr, msg, fmt_args);
  fprintf(stderr, "\n");
}

static void syntax_error(Token* t, const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  _analysis_error_internal(t, ANAL_ERROR_SYNTAX, msg, args);
  va_end(args);
}

static void internal_logic_error(Token* t, const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  _analysis_error_internal(t, ANAL_ERROR_INTERNAL_LOGIC, msg, args);
  va_end(args);
}

#endif
