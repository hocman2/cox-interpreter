#ifndef _TOKEN_H
#define _TOKEN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "string_view.h"

#define LEXEME_CODE(A, B) ((uint16_t)A) << 8 | B

enum TokenType {
  TOKEN_TYPE_EOF = 0,
  TOKEN_TYPE_LEFT_PAREN,
  TOKEN_TYPE_RIGHT_PAREN,
  TOKEN_TYPE_LEFT_BRACE,
  TOKEN_TYPE_RIGHT_BRACE,
  TOKEN_TYPE_DOT,
  TOKEN_TYPE_COMMA,
  TOKEN_TYPE_STAR,
  TOKEN_TYPE_PLUS,
  TOKEN_TYPE_MINUS,
  TOKEN_TYPE_SEMICOLON,
  TOKEN_TYPE_SLASH,
  TOKEN_TYPE_EQUAL_EQUAL,
  TOKEN_TYPE_EQUAL,
  TOKEN_TYPE_BANG_EQUAL,
  TOKEN_TYPE_BANG,
  TOKEN_TYPE_GREATER_EQUAL,
  TOKEN_TYPE_GREATER,
  TOKEN_TYPE_LESS_EQUAL,
  TOKEN_TYPE_LESS,
  TOKEN_TYPE_IGNORE,

  // Everything above are token types with known content of known length
  TOKEN_TYPE_NUM_FIXED_TOKENS,
  // Everything below are token types of unknown content and/or length

  TOKEN_TYPE_STRING,
  TOKEN_TYPE_NUMBER,
  TOKEN_TYPE_IDENTIFIER,
  TOKEN_TYPE_KEYWORD,
};

enum ReservedKeywordType {
  RESERVED_KEYWORD_AND = 0,
  RESERVED_KEYWORD_CLASS,
  RESERVED_KEYWORD_ELSE,
  RESERVED_KEYWORD_FALSE,
  RESERVED_KEYWORD_FOR,
  RESERVED_KEYWORD_FUN,
  RESERVED_KEYWORD_IF,
  RESERVED_KEYWORD_NIL,
  RESERVED_KEYWORD_OR,
  RESERVED_KEYWORD_PRINT,
  RESERVED_KEYWORD_RETURN,
  RESERVED_KEYWORD_SUPER,
  RESERVED_KEYWORD_THIS,
  RESERVED_KEYWORD_TRUE,
  RESERVED_KEYWORD_VAR,
  RESERVED_KEYWORD_WHILE,
  RESERVED_KEYWORD_NUM_RESERVED_KEYWORDS,
};

typedef struct {
  unsigned long whole;
  unsigned long decimal;
} Number;

typedef struct {
  enum TokenType type;
  StringView lexeme;
  size_t line;
  union {
    enum ReservedKeywordType keyword;
    StringView content;
    Number value;
  };
} Token;

typedef struct {
  enum ReservedKeywordType type;
  const char* str;
  size_t strlen;
} Keyword;

typedef struct {
  enum TokenType type;
  uint16_t lexeme_code;
  size_t lexeme_len;
} TokenLexeme;

static const TokenLexeme token_lexemes[] = {
  {TOKEN_TYPE_EQUAL_EQUAL,  LEXEME_CODE('=','='),  2},
  {TOKEN_TYPE_BANG_EQUAL,   LEXEME_CODE('!','='),  2},
  {TOKEN_TYPE_GREATER_EQUAL,LEXEME_CODE('>','='),  2},
  {TOKEN_TYPE_LESS_EQUAL,   LEXEME_CODE('<','='),  2},
  {TOKEN_TYPE_IGNORE,       LEXEME_CODE('/','/'),  2},

  {TOKEN_TYPE_EOF,          LEXEME_CODE('\0',0),   1},
  {TOKEN_TYPE_LEFT_PAREN,   LEXEME_CODE('(',0),    1},
  {TOKEN_TYPE_RIGHT_PAREN,  LEXEME_CODE(')',0),    1},
  {TOKEN_TYPE_LEFT_BRACE,   LEXEME_CODE('{',0),    1},
  {TOKEN_TYPE_RIGHT_BRACE,  LEXEME_CODE('}',0),    1},
  {TOKEN_TYPE_DOT,          LEXEME_CODE('.',0),    1},
  {TOKEN_TYPE_COMMA,        LEXEME_CODE(',',0),    1},
  {TOKEN_TYPE_STAR,         LEXEME_CODE('*',0),    1},
  {TOKEN_TYPE_PLUS,         LEXEME_CODE('+',0),    1},
  {TOKEN_TYPE_MINUS,        LEXEME_CODE('-',0),    1},
  {TOKEN_TYPE_SEMICOLON,    LEXEME_CODE(';',0),    1},
  {TOKEN_TYPE_SLASH,        LEXEME_CODE('/',0),    1},
  {TOKEN_TYPE_EQUAL,        LEXEME_CODE('=',0),    1},
  {TOKEN_TYPE_BANG,         LEXEME_CODE('!',0),    1},
  {TOKEN_TYPE_GREATER,      LEXEME_CODE('>',0),    1},
  {TOKEN_TYPE_LESS,         LEXEME_CODE('<',0),    1},
};

static const Keyword keywords[] = {
  {RESERVED_KEYWORD_AND,    "AND",    3},
  {RESERVED_KEYWORD_CLASS,  "CLASS",  5},
  {RESERVED_KEYWORD_ELSE,   "ELSE",   4},
  {RESERVED_KEYWORD_FALSE,  "FALSE",  5},
  {RESERVED_KEYWORD_FOR,    "FOR",    3},
  {RESERVED_KEYWORD_FUN,    "FUN",    3},
  {RESERVED_KEYWORD_IF,     "IF",     2},
  {RESERVED_KEYWORD_NIL,    "NIL",    3},
  {RESERVED_KEYWORD_OR,     "OR",     2},
  {RESERVED_KEYWORD_PRINT,  "PRINT",  5},
  {RESERVED_KEYWORD_RETURN, "RETURN", 6},
  {RESERVED_KEYWORD_SUPER,  "SUPER",  5},
  {RESERVED_KEYWORD_THIS,   "THIS",   4},
  {RESERVED_KEYWORD_TRUE,   "TRUE",   4},
  {RESERVED_KEYWORD_VAR,    "VAR",    3},
  {RESERVED_KEYWORD_WHILE,  "WHILE",  5},
};

static const char* token_type_to_string(enum TokenType tt) {
  switch(tt) {
    case TOKEN_TYPE_EOF:
      return "EOF";
    case TOKEN_TYPE_LEFT_PAREN:
      return "LEFT_PAREN";
    case TOKEN_TYPE_RIGHT_PAREN:
      return "RIGHT_PAREN";
    case TOKEN_TYPE_LEFT_BRACE:
      return "LEFT_BRACE";
    case TOKEN_TYPE_RIGHT_BRACE:
      return "RIGHT_BRACE";
    case TOKEN_TYPE_DOT:
      return "DOT";
    case TOKEN_TYPE_COMMA:
      return "COMMA";
    case TOKEN_TYPE_STAR:
      return "STAR";
    case TOKEN_TYPE_PLUS:
      return "PLUS";
    case TOKEN_TYPE_MINUS:
      return "MINUS";
    case TOKEN_TYPE_SEMICOLON:
      return "SEMICOLON";
    case TOKEN_TYPE_SLASH:
      return "SLASH";
    case TOKEN_TYPE_EQUAL_EQUAL:
      return "EQUAL_EQUAL";
    case TOKEN_TYPE_EQUAL:
      return "EQUAL";
    case TOKEN_TYPE_BANG_EQUAL:
      return "BANG_EQUAL";
    case TOKEN_TYPE_BANG:
      return "BANG";
    case TOKEN_TYPE_GREATER_EQUAL:
      return "GREATER_EQUAL";
    case TOKEN_TYPE_GREATER:
      return "GREATER";
    case TOKEN_TYPE_LESS_EQUAL:
      return "LESS_EQUAL";
    case TOKEN_TYPE_LESS:
      return "LESS";
    case TOKEN_TYPE_STRING:
      return "STRING";
    case TOKEN_TYPE_NUMBER:
      return "NUMBER";
    case TOKEN_TYPE_IDENTIFIER:
      return "IDENTIFIER";
    case TOKEN_TYPE_KEYWORD:
      return "KEYWORD";
    default:
      fprintf(stderr, "Unhandled token_type_to_string: %d", tt);
      exit(2);
  }
}

static bool keyword_from_string(const char* start, size_t len, enum ReservedKeywordType* kw) {
  bool ret = false;
  for (int k = 0; k < RESERVED_KEYWORD_NUM_RESERVED_KEYWORDS; ++k) {
    if (len != keywords[k].strlen) continue;

    bool found_match = true;
    for (size_t i = 0; i < len; ++i) {
      if (start[i] != tolower(keywords[k].str[i])) {
        found_match = false;
        break;
      }
    }

    if (found_match) {
      ret = true;
      *kw = keywords[k].type;
    }
  }

  return ret;
}

static const char* keyword_to_string(enum ReservedKeywordType type) {
  for (int k = 0; k < RESERVED_KEYWORD_NUM_RESERVED_KEYWORDS; ++k) {
    if (keywords[k].type == type)
      return keywords[k].str;
  }

  return NULL;
}


static void token_pretty_print(Token token) {
  switch(token.type) {
    case TOKEN_TYPE_KEYWORD:
      printf("%s %.*s null\n", keyword_to_string(token.keyword), (int)token.lexeme.len, token.lexeme.str);
    break;
    case TOKEN_TYPE_STRING:
      printf("%s %.*s %.*s\n", 
             token_type_to_string(token.type),
             (int)token.lexeme.len,
             token.lexeme.str,
             (int)token.content.len,
             token.content.str
             );
    break;
    case TOKEN_TYPE_NUMBER:
      printf("%s %.*s %lu.%lu\n",
          token_type_to_string(token.type),
          (int)token.lexeme.len,
          token.lexeme.str, 
          token.value.whole,
          token.value.decimal
          );
    break;
    default:
      printf("%s %.*s null\n", token_type_to_string(token.type), (int)token.lexeme.len, token.lexeme.str);
    break;
  }
}

#endif
