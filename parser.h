// Pure parsing. Calls methods on a Builder (template argument) to actually construct the AST

namespace cashew {

template<class ASTNode, class Builder>
struct Parser {
  // Highest-level parsing, as of a JavaScript script file.
  ASTNode* parseToplevel(char* src) {
    return parseBlock(src, Builder::makeToplevel());
  }

  // Parses a block of code (e.g. a bunch of statements inside {,}, or the top level of o file)
  ASTNode* parseBlock(char* src, ASTNode *block) {
    if (block) block = Builder::makeBlock();
  }
};

} // namespace cashew

