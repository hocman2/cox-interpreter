#ifndef _INTERPRETER_H
#define _INTERPRETER_H

#include "parser.h"
#include "types/value.h"

#define NUM_CLASSES 255
#define NUM_INSTANCES 255

struct PendingReturn {
  Value value;
  bool await_return;
  bool should_return;
};

struct InstanceSubject {
  Value subject;
  bool has_subject;
};

typedef struct {
  struct PendingReturn pending_return;
  struct InstanceSubject instance_subject;
  StringView this_kw;
} Interpreter;

void evaluation_pretty_print(Value* e);
void interpret(Statements stmts);

#endif
