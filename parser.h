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
               FUNCTION,
               OPEN_PAREN,
               OPEN_BRACE,
               COMMA,
               SET;

extern StringSet keywords, operators;

extern const char *OPERATOR_INITS, *SEPARATORS;

extern int MAX_OPERATOR_SIZE;

extern StringIntMap operatorPrec;

// parser

template<class NodeRef, class Builder>
class Parser {

  static bool isSpace(char x) { return x == 32 || x == 9 || x == 10 || x == 13; } /* space, tab, linefeed/newline, or return */
  static char* skipSpace(char* curr) {
    while (*curr) {
      if (isSpace(*curr)) {
        curr++;
        continue;
      }
      if (curr[0] == '/' && curr[1] == '/') {
        curr += 2;
        while (*curr && *curr != '\n') curr++;
        curr++;
        continue;
      }
      if (curr[0] == '/' && curr[1] == '*') {
        curr += 2;
        while (*curr && (curr[0] != '*' || curr[1] != '/')) curr++;
        curr += 2;
        continue;
      }
      break;
    }
    return curr;
  }

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
    NUMBER = 4,
    SEPARATOR = 5
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
        for (int i = 0; i < MAX_OPERATOR_SIZE; i++) {
          if (!start[i]) break;
          char temp = start[i+1];
          start[i+1] = 0;
          if (operators.has(start)) {
            str.set(start, false);
            src = start + i + 1;
          }
          start[i+1] = temp;
        }
        type = OPERATOR;
        assert(!str.isNull());
      } else if (hasChar(SEPARATORS, *src)) {
        type = SEPARATOR;
        char temp = src[1];
        src[1] = 0;
        str.set(src, false);
        src[1] = temp;
        src++;
      } else {
        fprintf(stderr, "Frag parsing failed on %c |%s|\n", *src, src);
        assert(0);
      }
      size = src - start;
    }
  };

  // Parses an element in a list of such elements, e.g. list of statements in a block, or list of parameters in a call
  NodeRef parseElement(char*& src, const char* seps=";") {
    //dump("parseElement", src);
    src = skipSpace(src);
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
        if (frag.type == IDENT) return parseAfterIdent(frag, src, seps);
        else return parseExpression(parseFrag(frag), src, seps);
      }
      case SEPARATOR: {
        if (frag.str == OPEN_PAREN) return parseExpression(parseAfterParen(src), src, seps);
        assert(0);
      }
      case OPERATOR: {
        return parseExpression(frag.str, src, seps);
      }
      default: dump("parseElement", src); printf("bad frag type: %d\n", frag.type); assert(0);
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
    src = skipSpace(src);
    if (frag.str == FUNCTION) return parseFunction(frag, src, seps);
    else if (frag.str == VAR) return parseVar(frag, src, seps);
    else if (frag.str == RETURN) return parseReturn(frag, src, seps);
    assert(0);
  }

  NodeRef parseFunction(Frag& frag, char*& src, const char* seps) {
    Frag name(src);
    assert(name.type == IDENT);
    src += name.size;
    NodeRef ret = Builder::makeFunction(name.str);
    src = skipSpace(src);
    assert(*src == '(');
    src++;
    while (1) {
      src = skipSpace(src);
      if (*src == ')') break;
      Frag arg(src);
      assert(arg.type == IDENT);
      src += arg.size;
      Builder::appendArgumentToFunction(ret, arg.str);
      src = skipSpace(src);
      if (*src && *src == ')') break;
      if (*src && *src == ',') {
        src++;
        continue;
      }
      assert(0);
    }
    assert(*src == ')');
    src++;
    src = skipSpace(src);
    assert(*src == '{');
    src++;
    parseBlock(src, ret, ";}"); // the two are not symmetrical, ; is just internally separating, } is the final one - parseBlock knows all this
    assert(*src == '}');
    src++;
    // TODO: parse expression?
    return ret;
  }

  NodeRef parseVar(Frag& frag, char*& src, const char* seps) {
    NodeRef ret = Builder::makeVar();
    while (1) {
      src = skipSpace(src);
      if (*src == ';') break;
      Frag name(src);
      assert(name.type == IDENT);
      NodeRef value;
      src += name.size;
      src = skipSpace(src);
      if (*src == '=') {
        src++;
        src = skipSpace(src);
        value = parseElement(src, ";,");
      }
      Builder::appendToVar(ret, name.str, value);
      src = skipSpace(src);
      if (*src && *src == ';') break;
      if (*src && *src == ',') {
        src++;
        continue;
      }
      assert(0);
    }
    assert(*src == ';');
    src++;
    return ret;
  }

  NodeRef parseReturn(Frag& frag, char*& src, const char* seps) {
    src = skipSpace(src);
    NodeRef value = *src != ';' ? parseElement(src, ";") : nullptr;
    src = skipSpace(src);
    assert(*src == ';');
    src++;
    return Builder::makeReturn(value);
  }

  NodeRef parseAfterIdent(Frag& frag, char*& src, const char* seps) {
    assert(!isSpace(*src));
    if (*src == '(') return parseExpression(parseCall(frag.str, src), src, seps);
    if (*src == '[') return parseExpression(parseIndexing(frag.str, src), src, seps);
    return parseExpression(parseFrag(frag), src, seps);
  }

  NodeRef parseCall(IString target, char*& src) {
    expressionPartsStack.resize(expressionPartsStack.size()+1);
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
    assert(expressionPartsStack.back().size() == 0);
    expressionPartsStack.pop_back();
    return ret;
  }

  NodeRef parseIndexing(IString target, char*& src) {
    expressionPartsStack.resize(expressionPartsStack.size()+1);
    assert(*src == '[');
    src++;
    NodeRef ret = Builder::makeIndexing(target, parseElement(src, "]"));
    src = skipSpace(src);
    assert(*src == ']');
    src++;
    assert(expressionPartsStack.back().size() == 0);
    expressionPartsStack.pop_back();
    return ret;
  }

  NodeRef parseAfterParen(char*& src) {
    expressionPartsStack.resize(expressionPartsStack.size()+1);
    src = skipSpace(src);
    NodeRef ret = parseElement(src, ")");
    src = skipSpace(src);
    assert(*src == ')');
    src++;
    assert(expressionPartsStack.back().size() == 0);
    expressionPartsStack.pop_back();
    return ret;
  }

  struct ExpressionElement {
    bool isNode;
    union {
      NodeRef node;
      IString op;
    };
    ExpressionElement(NodeRef n) : isNode(true), node(n) {}
    ExpressionElement(IString o) : isNode(false), op(o) {}
  };

  // This is a list of the current stack of node-operator-node-operator-etc.
  // this works by each parseExpression call appending to the vector; then recursing out, and the toplevel sorts it all
  typedef std::vector<ExpressionElement> ExpressionParts;
  std::vector<ExpressionParts> expressionPartsStack;

  void dumpParts(ExpressionParts& parts) {
    printf("expressionparts: %d,%d\n", parts.size(), parts.size());
    printf("|");
    for (int i = 0; i < parts.size(); i++) {
      if (parts[i].isNode) parts[i].node->stringify(std::cout);
      else printf(" _%s_ ", parts[i].op.str);
    }
    printf("|\n");
  }

  NodeRef parseExpression(ExpressionElement initial, char*&src, const char* seps) {
    //dump("parseExpression", src);
    ExpressionParts& parts = expressionPartsStack.back();
    src = skipSpace(src);
    if (*src == 0 || hasChar(seps, *src)) {
      if (parts.size() > 0) {
        parts.push_back(initial); // cherry on top of the cake
      }
      assert(initial.isNode);
      return initial.node;
    }
    Frag next(src);
    if (next.type == OPERATOR) {
      src += next.size;
      bool top = parts.size() == 0;
      parts.push_back(initial);
      parts.push_back(next.str);
      NodeRef last = parseElement(src, seps);
      if (!top) return last; // XXX
      {
        ExpressionParts& parts = expressionPartsStack.back(); // |parts| may have been invalidated by that call
        // we are the toplevel. sort it all out
        // collapse right to left, highest priority first
        int highest = 100, lowest = -1;
        for (int i = 0; i < parts.size(); i++) {
          if (parts[i].isNode) continue;
          int curr = operatorPrec[parts[i].op];
          highest = std::min(highest, curr);
          lowest = std::max(lowest, curr);
        }
        assert(highest <= lowest);
        auto merge = [](NodeRef left, IString op, NodeRef right) {
          return Builder::makeBinary(left, op, right);
        };
        for (int prec = highest; prec <= lowest; prec++) {
          for (int i = parts.size()-1; i >= 0; i--) {
            if (parts[i].isNode) continue;
            int curr = operatorPrec[parts[i].op];
            if (curr == prec) {
              assert(i > 0);
              parts[i] = merge(parts[i-1].node, parts[i].op, parts[i+1].node);
              parts.erase(parts.begin() + i + 1);
              parts.erase(parts.begin() + i - 1);
            }
          }
        }
        assert(parts.size() == 1 && parts[0].isNode);
        NodeRef ret = parts[0].node;
        parts.clear();
        return ret;
      }
    }
    assert(0);
  }

  // Parses a block of code (e.g. a bunch of statements inside {,}, or the top level of o file)
  NodeRef parseBlock(char*& src, NodeRef block=nullptr, const char* seps=";") {
    if (!block) block = Builder::makeBlock();
    while (*src) {
      src = skipSpace(src);
      if (*src == 0 || hasChar(seps, *src)) break; // XXX handle ;;
      NodeRef element = parseElement(src, seps);
      src = skipSpace(src);
      if (*src && *src == ';') {
        element = Builder::makeStatement(element);
        src++;
      }
      Builder::appendToBlock(block, element);
    }
    return block;
  }

  // Debugging

  char *allSource;
  int allSize;

  void dump(const char *where, char* curr) {
    /*
    printf("%s:\n=============\n", where);
    for (int i = 0; i < allSize; i++) printf("%c", allSource[i] ? allSource[i] : '?');
    printf("\n");
    for (int i = 0; i < (curr - allSource); i++) printf(" ");
    printf("^\n=============\n");
    */
    printf("%s:\n==========\n%s\n==========\n", where, curr);
  }

public:

  Parser() : allSource(nullptr), allSize(0) {
    expressionPartsStack.resize(1);
  }

  // Highest-level parsing, as of a JavaScript script file.
  NodeRef parseToplevel(char* src) {
    allSource = src;
    allSize = strlen(src);
    return parseBlock(src, Builder::makeToplevel());
  }
};

} // namespace cashew

