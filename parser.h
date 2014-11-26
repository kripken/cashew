// Pure parsing. Calls methods on a Builder (template argument) to actually construct the AST
//
// XXX All parsing methods assume they take ownership of the input string. This lets them reuse
//     parts of it. You will segfault if the input string cannot be reused and written to.

#include <vector>
#include <iostream>

#include <stdio.h>

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
               EMPTY,
               SET;

extern StringSet keywords, operators;

extern const char* OPERATOR_INITS;
extern int MAX_OPERATOR_SIZE;

template<class NodeRef, class Builder>
class Parser {

  static bool isSpace(char x) { return x == 32 || x == 9 || x == 10 || x == 13; } /* space, tab, linefeed/newline, or return */
  static char* skipSpace(char* curr) { while (*curr && isSpace(*curr)) curr++; return curr; }

  static bool isIdentInit(char x) { return (x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') || x == '_' || x == '$'; }
  static bool isIdentPart(char x) { return isIdentInit(x) || (x >= '0' && x <= '9'); }

  static bool isDigit(char x) { return x >= '0' && x <= '9'; }

  static bool hasChar(const char* list, char x) { while (*list) if (*list++ == x) return true; return false; }

  // An atomic fragment of something. Stops at a natural boundary.
  enum FragType {
    KEYWORD = 0,
    OPERATOR = 1,
    IDENT = 2,
    STRING = 3, // without quotes
    NUMBER = 4
  };

  struct Frag {
    union {
      IString str;
      double num;
    };
    int size;
    FragType type;

    Frag(char* src) {
      assert(!isSpace(*src));
      char *start = src;
      if (isIdentInit(*src)) {
        // read an identifier or a keyword
        src++;
        while (isIdentPart(*src)) {
          src++;
        }
        if (*src == 0) {
          str.set(start);
        } else {
          char temp = *src;
          *src = 0;
          str.set(start, false);
          *src = temp;
        }
        type = keywords.has(str) ? KEYWORD : IDENT;
      } else if (*src == '"' || *src == '\'') {
        char *end = strchr(src+1, *src);
        *end = 0;
        str.set(src+1);
        src = end+1;
        type = STRING;
      } else if (isDigit(*src)) {
        num = strtod(start, &src);
        type = NUMBER;
      } else if (hasChar(OPERATOR_INITS, *src)) {
        IString op;
        for (int i = 0; i < MAX_OPERATOR_SIZE; i++) {
          if (!start[i]) break;
          char temp = start[i+1];
          start[i+1] = 0;
          if (operators.has(start)) {
            op.set(start, false);
            src = start + i + 1;
          }
          start[i+1] = temp;
        }
        type = OPERATOR;
        assert(!!op);
      } else {
        fprintf(stderr, "Frag parsing failed on %s\n", src);
        assert(0);
      }
      size = src - start;
    }
  };

  // Parses an element in a list of such elements, e.g. list of statements in a block, or list of parameters in a call
  NodeRef parseElement(char*& src, const char* seps=";") {
    Frag frag(src);
    src += frag.size;
    switch (frag.type) {
      case KEYWORD: {
        return parseAfterKeyword(frag, src, seps);
      }
      case IDENT:
      case STRING:
      case NUMBER: {
        src = skipSpace(src);
        if (*src == 0 || hasChar(seps, *src)) return parseFrag(frag); // all done
        if (frag.type == IDENT) return parseAfterIdent(frag, src, seps);
        assert(0);
      }
      default: assert(0);
    }
  }

  NodeRef parseFrag(Frag& frag) {
    switch (frag.type) {
      case IDENT:  return Builder::makeName(frag.str);
      case STRING: return Builder::makeString(frag.str);
      case NUMBER: return Builder::makeNumber(frag.num);
      default: assert(0);
    }
  }

  NodeRef parseAfterKeyword(Frag& frag, char*& src, const char* seps) {
    assert(0);
  }

  NodeRef parseAfterIdent(Frag& frag, char*& src, const char* seps) {
    assert(!isSpace(*src));
    assert(!hasChar(seps, *src));
    if (*src == '(') return parseExpression(parseCall(frag.str, src), src, seps);
    return parseExpression(parseFrag(frag), src, seps);
  }

  NodeRef parseCall(IString target, char*& src) {
    assert(*src == '(');
    src++;
    NodeRef ret = Builder::makeCall(target);
    while (1) {
      src = skipSpace(src);
      if (*src == ')') break;
      Builder::appendToCall(ret, parseElement(src, ",)"));
      src = skipSpace(src);
      if (*src && *src == ')') break;
      if (*src && *src == ',') {
        src++;
        continue;
      }
      assert(0);
    }
    src++;
    return ret;
  }


  NodeRef parseExpression(NodeRef initial, char*&src, const char* seps) {
    static std::vector<NodeRef> nodeStack; // XXX these are *static* lists of the current stack of node-operator-node-operator-etc. This is not threadsafe!!1
    static std::vector<IString> strStack;  //     this works by each parseExpression call appending to the vector; then recursing out, and the toplevel sorts it all
    src = skipSpace(src);
    if (*src == 0 || hasChar(seps, *src)) {
      if (nodeStack.size() > 0) {
        nodeStack.push_back(initial); // cherry on top of the cake
      }
      return initial;
    }
    Frag next(src);
    if (next.type == OPERATOR) {
      src += next.size;
      bool top = nodeStack.size() == 0;
      nodeStack.push_back(initial);
      strStack.push_back(next.str);
      NodeRef last = parseElement(src, seps);
      if (!top) return last; // XXX
      // we are the toplevel. sort it all out
      assert(nodeStack.size() == strStack.size()+1);
      for (int i = 0; i < strStack.size(); i++) {
        nodeStack[i]->stringify(std::cout);
        printf(" %s ", strStack[i].str);
      }
      nodeStack.back()->stringify(std::cout);
      nodeStack.clear();
      strStack.clear();
      assert(0);//return parseExpression(parseOperation(frag.str, next.str, src), src, seps);
    }
    assert(0);
  }

  // Debugging

  char *allSource;
  int allSize;

  void dump(const char *where, char* curr) {
    printf("%s\n=============\n", where);
    for (int i = 0; i < allSize; i++) printf("%c", allSource[i] ? allSource[i] : '?');
    printf("\n");
    for (int i = 0; i < (curr - allSource); i++) printf(" ");
    printf("^\n=============\n");
  }

public:

  Parser() : allSource(nullptr), allSize(0) {}
  
  // Highest-level parsing, as of a JavaScript script file.
  NodeRef parseToplevel(char* src) {
    allSource = src;
    allSize = strlen(src);
    return parseBlock(src, Builder::makeToplevel());
  }

  // Parses a block of code (e.g. a bunch of statements inside {,}, or the top level of o file)
  NodeRef parseBlock(char* src, NodeRef block=nullptr) {
    if (!block) block = Builder::makeBlock();
    while (*src) {
      src = skipSpace(src);
      if (!*src) break;
      NodeRef element = parseElement(src);
      src = skipSpace(src);
      if (*src && *src == ';') {
        element = Builder::makeStatement(element);
        src++;
      }
      Builder::appendToBlock(block, element);
    }
    return block;
  }
};

} // namespace cashew

