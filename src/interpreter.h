#ifndef _INTERPRETER_H
#define _INTERPRETER_H

#include "parser.h"
#include "types/evaluation.h"

void evaluation_pretty_print(Evaluation* e);
void interpret(Statements stmts);

#endif
