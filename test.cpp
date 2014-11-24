
#include "simple_ast.h"

int main() {
  cashew::Parser<Ref, ValueBuilder> builder;
  Ref ast = builder.parseToplevel("print('hello world')");
  ast->stringify(std::cout);
}

