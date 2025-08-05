#ifndef _INTERPRETER_H
#define _INTERPRETER_H

#include "parser.h"
#include "types/value.h"

void evaluation_pretty_print(Value* e);
void interpret(Statements stmts);

#endif
