#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

const TokenLexeme token_lexemes[] = {
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

const Keyword keywords[] = {
  {RESERVED_KEYWORD_AND,    "and",    3},
  {RESERVED_KEYWORD_CLASS,  "class",  5},
  {RESERVED_KEYWORD_ELSE,   "else",   4},
  {RESERVED_KEYWORD_FALSE,  "false",  5},
  {RESERVED_KEYWORD_FOR,    "for",    3},
  {RESERVED_KEYWORD_FUN,    "fun",    3},
  {RESERVED_KEYWORD_IF,     "if",     2},
  {RESERVED_KEYWORD_NIL,    "nil",    3},
  {RESERVED_KEYWORD_OR,     "or",     2},
  {RESERVED_KEYWORD_PRINT,  "print",  5},
  {RESERVED_KEYWORD_RETURN, "return", 6},
  {RESERVED_KEYWORD_SUPER,  "super",  5},
  {RESERVED_KEYWORD_THIS,   "this",   4},
  {RESERVED_KEYWORD_TRUE,   "true",   4},
  {RESERVED_KEYWORD_VAR,    "var",    3},
  {RESERVED_KEYWORD_WHILE,  "while",  5},
};

const char* token_type_to_string(enum TokenType tt) {
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

bool keyword_from_string(const char* start, size_t len, enum ReservedKeywordType* kw) {
  bool ret = false;
  for (int k = 0; k < RESERVED_KEYWORD_NUM_RESERVED_KEYWORDS; ++k) {
    if (len != keywords[k].strlen) continue;

    bool found_match = true;
    for (size_t i = 0; i < len; ++i) {
      if (start[i] != keywords[k].str[i]) {
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

const char* keyword_to_string(enum ReservedKeywordType type) {

  for (int k = 0; k < RESERVED_KEYWORD_NUM_RESERVED_KEYWORDS; ++k) {
    if (keywords[k].type == type)
      return keywords[k].str;
  }

  return NULL;
}

void token_pretty_print(Token token) {
  switch(token.type) {
    case TOKEN_TYPE_KEYWORD: {
      char kwupper[128] = {0};
      const char* kw = keyword_to_string(token.keyword);
      size_t i = 0;
      for (; kw[i] != '\0'; ++i)
        kwupper[i] = toupper(kw[i]);

      kwupper[i] = '\0';
      printf("%s "SV_Fmt" null\n", kwupper, SV_Fmt_arg(token.lexeme));
    }
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
