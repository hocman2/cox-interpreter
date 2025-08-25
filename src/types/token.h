#ifndef _TOKEN_H
#define _TOKEN_H

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

extern const TokenLexeme token_lexemes[];
extern const Keyword keywords[];

const char* token_type_to_string(enum TokenType tt); 
bool keyword_from_string(const char* start, size_t len, enum ReservedKeywordType* kw);
const char* keyword_to_string(enum ReservedKeywordType type);
void token_pretty_print(Token token); 

#endif
