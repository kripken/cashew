
#include "simple_ast.cpp"

// Arena methods

Ref Arena::alloc() {
  if (chunks.size() == 0 || index == CHUNK_SIZE) {
    chunks.push_back(new Value[CHUNK_SIZE]);
    index = 0;
  }
  return &chunks.back()[index++];
}

// dump

void dump(const char *str, Ref node, bool pretty) {
  std::cerr << str << ": ";
  if (!!node) node->stringify(std::cerr, pretty);
  else std::cerr << "(nullptr)";
  std::cerr << std::endl;
}

