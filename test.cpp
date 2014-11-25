
#include "simple_ast.h"

int main() {
  cashew::Parser<Ref, ValueBuilder> builder;
  char input[] = "print('hello world')";
  Ref ast = builder.parseToplevel(input);
  std::cout << "output:\n";
  ast->stringify(std::cout, true);
  std::cout << "\n";
}

