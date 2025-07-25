#include "lexer.h"

#include <stdlib.h>
#include <stdint.h>

#include "types/token.h"


// Also increments cursor if the next character matches the target
static bool next_is(char target, const char** cursor) {
  char c = **cursor;
  char next = *(*cursor+1);
  if (c == '\0' || next == '\0') return false;
  if (next != target) return false;

  ++(*cursor);

  return true;
}

// Advances start while the provided test function returns true or null terminator is reached.
// Returns the character pointer where the test failed or null terminator.
const char* advance_while(const char* start, bool (*test)(char)) {
  // might not be too memory safe lol
  if (*start == '\0') return start;

  size_t i = 1;  
  for (; start[i] != '\0'; ++i) {
    if (!test(start[i]))
      break;
  }

  return start + i;
}

// filters for advancing cursor //

bool not_newline(char c) {
  return c != '\n'; 
}

bool not_doublequote_nor_newline(char c) {
  return c != '\n' && c != '"';
}

bool alpha_or_underscore_or_digit(char c) {
  return isalpha(c) || isdigit(c) || c == '_';
}

// -- //
Tokenizer tokenizer_new(const char* file_contents) {
  Tokenizer t;
  t.cursor = file_contents;
  t.line = 1;
  return t;
}

// if tokenizer's cursor is on a fixed token, writes the token type to tt and returns true.
// In that case the tokenizer's cursor will be advanced by the appropriate length
// Otherwise returns false and does not move the cursor
bool find_token_type(Tokenizer *t, enum TokenType* tt, size_t* token_len) {
  char c = *(t->cursor);
  char c_next = *(t->cursor + 1);
  uint16_t code2 = ((uint16_t)c) << 8 | c_next;
  uint16_t code1 = ((uint16_t)c) << 8 | 0;

  for (size_t i = 0; i < TOKEN_TYPE_NUM_FIXED_TOKENS; ++i) {
    TokenLexeme tl = token_lexemes[i];

    if (code2 == tl.lexeme_code || (tl.lexeme_len == 1 && code1 == tl.lexeme_code)) {
      *tt = tl.type;
      t->cursor += tl.lexeme_len;
      *token_len = tl.lexeme_len;
      return true;
    }
  }

  return false;
}

int tokenizer_get_next(Tokenizer *t, Token* o_token) {
  enum TokenType tt;
  size_t token_len;
  if (find_token_type(t, &tt, &token_len)) {
    if (tt == TOKEN_TYPE_IGNORE) {
      t->cursor = advance_while(t->cursor, not_newline);
    }
    o_token->type = tt;
    o_token->line = t->line;
    o_token->lexeme.str = t->cursor - token_len; 
    o_token->lexeme.len = token_len;
    return 0;
  }

  switch(*(t->cursor)) {
    case '\n':
      t->line += 1;
    case ' ':
    case '\t':
      o_token->type = TOKEN_TYPE_IGNORE;
      t->cursor += 1;
      break;

    case '"': {
      const char* tokend = advance_while(t->cursor, not_doublequote_nor_newline);
      if (*tokend != '"') {
        t->cursor = tokend;
        o_token->type = TOKEN_TYPE_IGNORE;
        fprintf(stderr, "[line %d] Error: Unterminated string.\n", (int)t->line);
        return 65;
      } else {
        o_token->type = TOKEN_TYPE_STRING;
        o_token->lexeme.str = t->cursor; 
        o_token->lexeme.len = (size_t)(tokend - t->cursor + 1);
        o_token->line = t->line;

        size_t num_chars = (size_t)(tokend - t->cursor - 1);
        if (num_chars == 0) {
          o_token->content.str = t->cursor; 
          o_token->content.len = 0; 
        } else {
          o_token->content.str = t->cursor+1;
          o_token->content.len = num_chars;
        }

        // advance past the double quote
        t->cursor = tokend + 1;
      }
    }
    break;

    default:
      if (isdigit(*(t->cursor))) {
        const char* p = t->cursor + 1;

        while(isdigit(*p)) ++p;
        o_token->value.whole = strtol(t->cursor, NULL, 10);
        o_token->value.decimal = 0;
        if (*p == '.' && isdigit(p[1])) {
          const char* dec_end = p + 1;
          while(isdigit(*dec_end)) ++dec_end;
          long value = strtol(p+1, NULL, 10);
          if (value > 0) {
            while(value % 10 == 0) {
              value /= 10;
            }
          }
          o_token->value.decimal = value;
          p = dec_end;
        }

        o_token->type = TOKEN_TYPE_NUMBER;
        o_token->lexeme.str = t->cursor;
        o_token->lexeme.len = (size_t)(p - t->cursor);
        o_token->line = t->line;
        t->cursor = p;

      } else if (*(t->cursor) == '_' || isalpha(*(t->cursor))) {
        const char* tokend = advance_while(t->cursor, alpha_or_underscore_or_digit);
        size_t len = (size_t)(tokend - t->cursor);
        o_token->lexeme.str = t->cursor;
        o_token->lexeme.len = len;
        o_token->line = t->line;

        if (keyword_from_string(t->cursor, len, &(o_token->keyword))) {
          o_token->type = TOKEN_TYPE_KEYWORD;
        } else {
          o_token->type = TOKEN_TYPE_IDENTIFIER;
        }
        o_token->lexeme.str = t->cursor;
        o_token->lexeme.len = (size_t)(tokend - t->cursor);
        t->cursor = tokend;
      } else {
        o_token->type = TOKEN_TYPE_IGNORE;
        fprintf(stderr, "[line %d] Error: Unexpected character: %c\n", (int)t->line, *(t->cursor));
        t->cursor += 1;
        return 65;
      }
      break;
  }

  return 0;
}

int tokenizer_scan_file(Tokenizer* t, Token** tokens, size_t* num_tokens) {
  *num_tokens = 0;
  size_t tokens_cap = 10;
  *tokens = calloc(tokens_cap, sizeof(Token));

  if (*tokens == NULL) {
    fprintf(stderr, "Out of memory");
    return -1;
  }

  int tokenize_result = 0;
  while(true) {
    Token* token = *tokens + *num_tokens;
    int code = tokenizer_get_next(t, token);

    if (tokenize_result == 0 && code != 0) tokenize_result = code;
    if (token->type == TOKEN_TYPE_EOF) break;
    // reallocate next token at the same spot
    if (token->type == TOKEN_TYPE_IGNORE) continue;

    *num_tokens += 1;
    if (*num_tokens >= tokens_cap) {
      tokens_cap *= 2;
      *tokens = realloc((void*)(*tokens), sizeof(Token) * tokens_cap);
      if (*tokens == NULL) {
        fprintf(stderr, "Out of memory");
        return -1;
      }
    }
  }

  // so far we've been using this variable as a 0 based index
  *num_tokens += 1;
  return tokenize_result;
}
