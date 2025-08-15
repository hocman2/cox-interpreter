#ifndef _LEXER_H
#define _LEXER_H

#include <stdlib.h>
#include "types/token.h"

typedef struct {
  const char* cursor;
  const char* end;
  size_t line;
} Tokenizer;

Tokenizer tokenizer_new(const char* file_contents, size_t file_sz);
int tokenizer_get_next(Tokenizer* t, Token* o_token);
int tokenizer_scan_file(Tokenizer* t, Token** tokens, size_t* num_tokens);
#endif
