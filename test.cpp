
#include "simple_ast.h"

int main() {
  cashew::Parser<Value, ValueBuilder> builder;
  Ref ast = builder.parseToplevel("print('hello world')");
  ast.stringify(cout);
}

