
#include "simple_ast.h"

int main(int argc, char **argv) {
  // Read input file
  FILE *f = fopen(argv[1], "r");
  assert(f);
  fseek(f, 0, SEEK_END);
  int size = ftell(f);
  char *src = new char[size+1];
  rewind(f);
  int num = fread(src, 1, size, f);
  assert(num == size);
  fclose(f);
  src[size] = 0;

  cashew::Parser<Ref, ValueBuilder> builder;
  Ref ast = builder.parseToplevel(src);
  std::cout << "output:\n";
  ast->stringify(std::cout, true);
  std::cout << "\n";
}

