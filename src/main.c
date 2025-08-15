#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "launch_context.h"
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "types/token.h"

const char* read_file_contents(const char* filename, size_t* byte_sz);

int main(int argc, char *argv[]) {
  // Disable output buffering
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  if (argc < 3) {
    fprintf(stderr, "Usage: ./your_program tokenize <filename>\n");
    return 1;
  }

  const char *command = argv[1];
  size_t file_sz;
  const char* file_contents = read_file_contents(argv[2], &file_sz);
  char** options = argv + 3;

  if (file_contents == NULL) {
    exit(1);
  }

  if (argc > 3) {
    launch_ctx_new(options, argc - 3);
  }

  int return_code = 0;
  if (strcmp(command, "tokenize") == 0) {
    Tokenizer t = tokenizer_new(file_contents, file_sz);

    int tokenize_result = 0;
    while(true) {
      Token token;
      int code = tokenizer_get_next(&t, &token);

      if (tokenize_result == 0 && code != 0) tokenize_result = code;
      else if (token.type != TOKEN_TYPE_IGNORE) token_pretty_print(token);

      if (token.type == TOKEN_TYPE_EOF) break;
    }

    if (tokenize_result > 0) {
      return_code = tokenize_result;
      goto cleanup;
    }

  } else if (strcmp(command, "parse") == 0) {
    Tokenizer t = tokenizer_new(file_contents, file_sz);

    Token* tokens;
    size_t num_tokens;
    int tokenize_result = tokenizer_scan_file(&t, &tokens, &num_tokens);

    if (tokenize_result > 0) {
      return_code = tokenize_result;
      free(tokens);
      goto cleanup;
    } 

    parser_init();

    Statements stmts;
    parse(tokens, num_tokens, &stmts);
    for (size_t i = 0; i < stmts.count; ++i) {
      statement_pretty_print(stmts.xs + i);
    }

    parser_free(&stmts);
    free(tokens);
  } else if (strcmp(command, "interpret") == 0) {
    Tokenizer t = tokenizer_new(file_contents, file_sz);

    Token* tokens;
    size_t num_tokens;
    int tokenize_result = tokenizer_scan_file(&t, &tokens, &num_tokens);

    if (tokenize_result > 0) {
      return_code = tokenize_result;
      free(tokens);
      goto cleanup;
    }

    parser_init();
    Statements stmts;
    if (!parse(tokens, num_tokens, &stmts)) {
      return_code = 66; // arbitrary fuck you
      parser_free(&stmts);
      goto cleanup;
    }

    interpret(stmts);

    parser_free(&stmts);
    free(tokens);
  } else {
    fprintf(stderr, "Unknown command: %s\n", command);
    return_code = 1;
  }

cleanup:
  free((void*)file_contents);
  return return_code;
}

const char* read_file_contents(const char* filename, size_t* byte_sz) {
  assert(byte_sz && "bytes_sz must be a valid pointer");

  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    fprintf(stderr, "Error reading file: %s\n", filename);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind(file);

  char *file_contents = malloc(file_size + 1);
  if (file_contents == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    fclose(file);
    return NULL;
  }

  *byte_sz = fread(file_contents, 1, file_size, file);
  if (*byte_sz < file_size) {
    fprintf(stderr, "Error reading file contents\n");
    free(file_contents);
    fclose(file);
    return NULL;
  }

  file_contents[file_size] = '\0';
  fclose(file);
  return file_contents;
}
