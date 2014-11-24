// Pure parsing. Calls methods on a Builder (template argument) to actually construct the AST

#include "istring.h"

namespace cashew {

#define is_space(x) (x == 32 || x == 9 || x == 10 || x == 13) /* space, tab, linefeed/newline, or return */
#define skip_space() { while (*curr && is_space(*curr)) curr++; }

template<class ASTNode, class Builder>
struct Parser {
  // Highest-level parsing, as of a JavaScript script file.
  ASTNode* parseToplevel(char* src) {
    return parseBlock(src, Builder::makeToplevel());
  }

  // Parses a block of code (e.g. a bunch of statements inside {,}, or the top level of o file)
  ASTNode* parseBlock(char* src, ASTNode *block) {
    if (block) block = Builder::makeBlock();
    skip_space(src);
  }
};

} // namespace cashew

