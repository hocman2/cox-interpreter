#ifndef _INTERPRETER_H
#define _INTERPRETER_H

#include "parser.h"
#include "types/value.h"

#define NUM_CLASSES 255

struct PendingReturn {
  Value value;
  bool await_return;
  bool should_return;
};

typedef struct {
  struct PendingReturn pending_return;
  StringView this_kw;
} Interpreter;

void evaluation_pretty_print(Value* e);
void interpret(Statements stmts);

#endif
