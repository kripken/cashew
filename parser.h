// Pure parsing. Calls methods on a Builder (template argument) to actually construct the AST
//
// All parsing methods assume they take ownership of the input string. This lets them reuse
// parts of it.

#include "istring.h"

namespace cashew {

// common strings

extern IString TOPLEVEL,
               DEFUN,
               BLOCK,
               STAT,
               ASSIGN,
               NAME,
               VAR,
               CONDITIONAL,
               BINARY,
               RETURN,
               IF,
               WHILE,
               DO,
               FOR,
               SEQ,
               SUB,
               CALL,
               NUM,
               LABEL,
               BREAK,
               CONTINUE,
               SWITCH,
               STRING,
               INF,
               NaN,
               TEMP_RET0,
               UNARY_PREFIX,
               UNARY_POSTFIX,
               MATH_FROUND,
               SIMD_FLOAT32X4,
               SIMD_INT32X4,
               PLUS,
               MINUS,
               OR,
               AND,
               XOR,
               L_NOT,
               B_NOT,
               LT,
               GE,
               LE,
               GT,
               EQ,
               NE,
               DIV,
               MOD,
               RSHIFT,
               LSHIFT,
               TRSHIFT,
               TEMP_DOUBLE_PTR,
               HEAP8,
               HEAP16,
               HEAP32,
               HEAPF32,
               HEAPU8,
               HEAPU16,
               HEAPU32,
               HEAPF64,
               F0,
               EMPTY;

#define is_space(x) (x == 32 || x == 9 || x == 10 || x == 13) /* space, tab, linefeed/newline, or return */
#define skip_space(curr) { while (*curr && is_space(*curr)) curr++; }

template<class NodeRef, class Builder>
struct Parser {
  // Highest-level parsing, as of a JavaScript script file.
  NodeRef parseToplevel(char* src) {
    return parseBlock(src, Builder::makeToplevel());
  }

  // Parses a block of code (e.g. a bunch of statements inside {,}, or the top level of o file)
  NodeRef parseBlock(char* src, NodeRef block=nullptr) {
    if (!block) block = Builder::makeBlock();
    skip_space(src);
    return nullptr;
  }
};

} // namespace cashew

