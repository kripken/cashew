
#include "simple_ast.h"

int main() {
  cashew::Parser<Ref, ValueBuilder> builder;
  char input[] = "print('hello world')";
  Ref ast = builder.parseToplevel(input);
  ast->stringify(std::cout);
}

