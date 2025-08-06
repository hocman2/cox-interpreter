#ifndef _STACK_H
#define _STACK_H

#include "../types/value.h"

#define STACK_CAPACITY 1024

typedef Value StackValue;

typedef struct {
  size_t count;
  size_t capacity;
  StackValue* xs;
} Stack;

Stack stack_new();
StackValue stack_pop(Stack* s);
void stack_push(Stack* s, const StackValue* value);
size_t stack_count(Stack* s);

#endif
