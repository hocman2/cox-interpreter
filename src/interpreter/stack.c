#include "stack.h"
#include "../types/vector.h"

Stack stack_new() {
  Stack s = {0};
  vector_new(s, STACK_CAPACITY);
  return s;
}

void stack_push(Stack* s, const StackValue* value) {
  vector_push(*s, *value);
}

StackValue stack_pop(Stack* s) {
  StackValue value = s->xs[s->count - 1]; 
  vector_pop(*s);
  return value;
}

size_t stack_count(Stack* s) {
  return s->count;
}
