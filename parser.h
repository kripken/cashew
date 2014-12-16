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
               ELSE,
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
               MUL,
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
               OPEN_CURLY,
               CLOSE_CURLY,
               COMMA,
               QUESTION,
               COLON,
               CASE,
               DEFAULT,
               DOT,
               PERIOD,
               NEW,
               ARRAY,
               OBJECT,
               THROW,
               SET;

extern IStringSet keywords, allOperators;

extern const char *OPERATOR_INITS, *SEPARATORS;

extern int MAX_OPERATOR_SIZE, LOWEST_PREC;

struct OperatorClass {
  enum Type {
    Binary = 0,
    Prefix = 1,
    Postfix = 2,
    Tertiary = 3
  };

  IStringSet ops;
  bool rtl;
  Type type;

  OperatorClass(const char* o, bool r, Type t) : ops(o), rtl(r), type(t) {}

  static int getPrecedence(Type type, IString op);
  static bool getRtl(int prec);
};

extern std::vector<OperatorClass> operatorClasses;

extern bool isIdentInit(char x);
extern bool isIdentPart(char x);

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
        if (*curr) curr++;
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
      } else if (isDigit(*src) || (src[0] == '.' && isDigit(src[1]))) {
        num = strtod(start, &src);
        assert(src > start);
        type = NUMBER;
      } else if (hasChar(OPERATOR_INITS, *src)) {
        switch (*src) {
          case '!': str = src[1] == '=' ? NE : L_NOT; break;
          case '%': str = MOD; break;
          case '&': str = AND; break;
          case '*': str = MUL; break;
          case '+': str = PLUS; break;
          case ',': str = COMMA; break;
          case '-': str = MINUS; break;
          case '.': str = PERIOD; break;
          case '/': str = DIV; break;
          case ':': str = COLON; break;
          case '<': str = src[1] == '<' ? LSHIFT : (src[1] == '=' ? LE : LT); break;
          case '=': str = src[1] == '=' ? EQ : SET; break;
          case '>': str = src[1] == '>' ? (src[2] == '>' ? TRSHIFT : RSHIFT) : (src[1] == '=' ? GE : GT); break;
          case '?': str = QUESTION; break;
          case '^': str = XOR; break;
          case '|': str = OR; break;
          case '~': str = B_NOT; break;
        }
        assert(!str.isNull());
        size = strlen(str.str);
#ifndef NDEBUG
        char temp = start[size];
        start[size] = 0;
        assert(strcmp(str.str, start) == 0);
        start[size] = temp;
#endif
        type = OPERATOR;
        return;
      } else if (hasChar(SEPARATORS, *src)) {
        type = SEPARATOR;
        char temp = src[1];
        src[1] = 0;
        str.set(src, false);
        src[1] = temp;
        src++;
      } else {
        dump("frag parsing", src);
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
        if (frag.str == OPEN_BRACE) return parseExpression(parseAfterBrace(src), src, seps);
        if (frag.str == OPEN_CURLY) return parseExpression(parseAfterCurly(src), src, seps);
        assert(0);
      }
      case OPERATOR: {
        return parseExpression(frag.str, src, seps);
      }
      default: /* dump("parseElement", src); printf("bad frag type: %d\n", frag.type); */ assert(0);
    }
    return nullptr;
  }

  NodeRef parseFrag(Frag& frag) {
    switch (frag.type) {
      case IDENT:  return builder.makeName(frag.str);
      case STRING: return builder.makeString(frag.str);
      case NUMBER: return builder.makeNumber(frag.num);
      default: assert(0);
    }
    return nullptr;
  }

  NodeRef parseAfterKeyword(Frag& frag, char*& src, const char* seps) {
    src = skipSpace(src);
    if (frag.str == FUNCTION) return parseFunction(frag, src, seps);
    else if (frag.str == VAR) return parseVar(frag, src, seps);
    else if (frag.str == RETURN) return parseReturn(frag, src, seps);
    else if (frag.str == IF) return parseIf(frag, src, seps);
    else if (frag.str == DO) return parseDo(frag, src, seps);
    else if (frag.str == WHILE) return parseWhile(frag, src, seps);
    else if (frag.str == BREAK) return parseBreak(frag, src, seps);
    else if (frag.str == CONTINUE) return parseContinue(frag, src, seps);
    else if (frag.str == SWITCH) return parseSwitch(frag, src, seps);
    else if (frag.str == NEW) return parseNew(frag, src, seps);
    dump(frag.str.str, src);
    assert(0);
    return nullptr;
  }

  NodeRef parseFunction(Frag& frag, char*& src, const char* seps) {
    Frag name(src);
    if (name.type == IDENT) {
      src += name.size;
    } else {
      assert(name.type == SEPARATOR && name.str[0] == '(');
      name.str = IString();
    }
    NodeRef ret = builder.makeFunction(name.str);
    src = skipSpace(src);
    assert(*src == '(');
    src++;
    while (1) {
      src = skipSpace(src);
      if (*src == ')') break;
      Frag arg(src);
      assert(arg.type == IDENT);
      src += arg.size;
      builder.appendArgumentToFunction(ret, arg.str);
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
    parseBracketedBlock(src, ret);
    // TODO: parse expression?
    return ret;
  }

  NodeRef parseVar(Frag& frag, char*& src, const char* seps) {
    NodeRef ret = builder.makeVar();
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
      builder.appendToVar(ret, name.str, value);
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
    NodeRef value = !hasChar(seps, *src) ? parseElement(src, seps) : nullptr;
    src = skipSpace(src);
    assert(hasChar(seps, *src));
    if (*src == ';') src++;
    return builder.makeReturn(value);
  }

  NodeRef parseIf(Frag& frag, char*& src, const char* seps) {
    NodeRef condition = parseParenned(src);
    NodeRef ifTrue = parseMaybeBracketed(src, seps);
    src = skipSpace(src);
    NodeRef ifFalse;
    if (*src && !hasChar(seps, *src)) {
      Frag next(src);
      if (next.type == KEYWORD && next.str == ELSE) {
        src += next.size;
        ifFalse = parseMaybeBracketed(src, seps);
      }
    }
    return builder.makeIf(condition, ifTrue, ifFalse);
  }

  NodeRef parseDo(Frag& frag, char*& src, const char* seps) {
    NodeRef body = parseMaybeBracketed(src, seps);
    src = skipSpace(src);
    Frag next(src);
    assert(next.type == KEYWORD && next.str == WHILE);
    src += next.size;
    NodeRef condition = parseParenned(src);
    return builder.makeDo(body, condition);
  }

  NodeRef parseWhile(Frag& frag, char*& src, const char* seps) {
    NodeRef condition = parseParenned(src);
    NodeRef body = parseMaybeBracketed(src, seps);
    return builder.makeWhile(condition, body);
  }

  NodeRef parseBreak(Frag& frag, char*& src, const char* seps) {
    src = skipSpace(src);
    Frag next(src);
    if (next.type == IDENT) src += next.size;
    return builder.makeBreak(next.type == IDENT ? next.str : IString());
  }

  NodeRef parseContinue(Frag& frag, char*& src, const char* seps) {
    src = skipSpace(src);
    Frag next(src);
    if (next.type == IDENT) src += next.size;
    return builder.makeContinue(next.type == IDENT ? next.str : IString());
  }

  NodeRef parseSwitch(Frag& frag, char*& src, const char* seps) {
    NodeRef ret = builder.makeSwitch(parseParenned(src));
    src = skipSpace(src);
    assert(*src == '{');
    src++;
    while (1) {
      // find all cases and possibly a default
      src = skipSpace(src);
      if (*src == '}') break;
      Frag next(src);
      if (next.type == KEYWORD) {
        if (next.str == CASE) {
          src += next.size;
          src = skipSpace(src);
          NodeRef arg;
          Frag value(src);
          if (value.type == NUMBER) {
            arg = parseFrag(value);
            src += value.size;
          } else {
            assert(value.type == OPERATOR);
            assert(value.str == MINUS);
            src += value.size;
            src = skipSpace(src);
            Frag value2(src);
            assert(value2.type == NUMBER);
            arg = builder.makePrefix(MINUS, parseFrag(value2));
            src += value2.size;
          }
          builder.appendCaseToSwitch(ret, arg);
          src = skipSpace(src);
          assert(*src == ':');
          src++;
          continue;
        } else if (next.str == DEFAULT) {
          src += next.size;
          builder.appendDefaultToSwitch(ret);
          src = skipSpace(src);
          assert(*src == ':');
          src++;
          continue;
        }
        // otherwise, may be some keyword that happens to start a block (e.g. case 1: _return_ 5)
      }
      // not case X: or default: or }, so must be some code
      src = skipSpace(src);
      bool explicitBlock = *src == '{';
      builder.appendCodeToSwitch(ret, parseMaybeBracketedBlock(src, ";}", CASE, DEFAULT), explicitBlock);
    }
    src = skipSpace(src);
    assert(*src == '}');
    src++;
    return ret;
  }

  NodeRef parseNew(Frag& frag, char*& src, const char* seps) {
    return builder.makeNew(parseElement(src, seps));
  }

  NodeRef parseAfterIdent(Frag& frag, char*& src, const char* seps) {
    assert(!isSpace(*src));
    if (*src == '(') return parseExpression(parseCall(parseFrag(frag), src), src, seps);
    if (*src == '[') return parseExpression(parseIndexing(parseFrag(frag), src), src, seps);
    if (*src == ':' && expressionPartsStack.back().size() == 0) {
      src++;
      return builder.makeLabel(frag.str, parseElement(src, seps));
    }
    if (*src == '.') return parseExpression(parseDotting(parseFrag(frag), src), src, seps);
    return parseExpression(parseFrag(frag), src, seps);
  }

  NodeRef parseCall(NodeRef target, char*& src) {
    expressionPartsStack.resize(expressionPartsStack.size()+1);
    assert(*src == '(');
    src++;
    NodeRef ret = builder.makeCall(target);
    while (1) {
      src = skipSpace(src);
      if (*src == ')') break;
      builder.appendToCall(ret, parseElement(src, ",)"));
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

  NodeRef parseIndexing(NodeRef target, char*& src) {
    expressionPartsStack.resize(expressionPartsStack.size()+1);
    assert(*src == '[');
    src++;
    NodeRef ret = builder.makeIndexing(target, parseElement(src, "]"));
    src = skipSpace(src);
    assert(*src == ']');
    src++;
    assert(expressionPartsStack.back().size() == 0);
    expressionPartsStack.pop_back();
    return ret;
  }

  NodeRef parseDotting(NodeRef target, char*& src) {
    assert(*src == '.');
    src++;
    Frag key(src);
    assert(key.type == IDENT);
    src += key.size;
    return builder.makeDot(target, key.str);
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

  NodeRef parseAfterBrace(char*& src) {
    expressionPartsStack.resize(expressionPartsStack.size()+1);
    NodeRef ret = builder.makeArray();
    while (1) {
      src = skipSpace(src);
      assert(*src);
      if (*src == ']') break;
      NodeRef element = parseElement(src, ",]");
      builder.appendToArray(ret, element);
      src = skipSpace(src);
      if (*src == ',') {
        src++;
        continue;
      } else assert(*src == ']');
    }
    assert(*src == ']');
    src++;
    return ret;
  }

  NodeRef parseAfterCurly(char*& src) {
    expressionPartsStack.resize(expressionPartsStack.size()+1);
    NodeRef ret = builder.makeObject();
    while (1) {
      src = skipSpace(src);
      assert(*src);
      if (*src == '}') break;
      Frag key(src);
      assert(key.type == IDENT || key.type == STRING);
      src += key.size;
      src = skipSpace(src);
      assert(*src == ':');
      src++;
      NodeRef value = parseElement(src, ",}");
      builder.appendToObject(ret, key.str, value);
      src = skipSpace(src);
      if (*src == ',') {
        src++;
        continue;
      } else assert(*src == '}');
    }
    assert(*src == '}');
    src++;
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

    NodeRef getNode() {
      assert(isNode);
      return node;
    }
    IString getOp() {
      assert(!isNode);
      return op;
    }
  };

  // This is a list of the current stack of node-operator-node-operator-etc.
  // this works by each parseExpression call appending to the vector; then recursing out, and the toplevel sorts it all
  typedef std::vector<ExpressionElement> ExpressionParts;
  std::vector<ExpressionParts> expressionPartsStack;

  void dumpParts(ExpressionParts& parts, int i) {
    printf("expressionparts: %d (at %d)\n", parts.size(), i);
    printf("| ");
    for (int i = 0; i < parts.size(); i++) {
      if (parts[i].isNode) {
        parts[i].getNode()->stringify(std::cout);
        printf("    ");
      } else {
        printf("    _%s_    ", parts[i].getOp().str);
      }
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
      return initial.getNode();
    }
    bool top = parts.size() == 0;
    if (initial.isNode) {
      Frag next(src);
      if (next.type == OPERATOR) {
        parts.push_back(initial);
        src += next.size;
        parts.push_back(next.str);
      } else {
        if (*src == '(') {
          initial = parseCall(initial.getNode(), src);
        } else if (*src == '[') {
          initial = parseIndexing(initial.getNode(), src);
        } else {
          dump("bad parseExpression state", src);
          assert(0);
        }
        return parseExpression(initial, src, seps);
      }
    } else {
      parts.push_back(initial);
    }
    NodeRef last = parseElement(src, seps);
    if (!top) return last;
    {
      ExpressionParts& parts = expressionPartsStack.back(); // |parts| may have been invalidated by that call
      // we are the toplevel. sort it all out
      // collapse right to left, highest priority first
      //dumpParts(parts, 0);
      for (auto ops : operatorClasses) {
        if (ops.rtl) {
          // right to left
          for (int i = parts.size()-1; i >= 0; i--) {
            if (parts[i].isNode) continue;
            IString op = parts[i].getOp();
            if (!ops.ops.has(op)) continue;
            if (ops.type == OperatorClass::Binary && i > 0 && i < parts.size()-1) {
              parts[i] = builder.makeBinary(parts[i-1].getNode(), op, parts[i+1].getNode());
              parts.erase(parts.begin() + i + 1);
              parts.erase(parts.begin() + i - 1);
            } else if (ops.type == OperatorClass::Prefix && i < parts.size()-1) {
              if (i > 0 && parts[i-1].isNode) continue; // cannot apply prefix operator if it would join two nodes
              parts[i] = builder.makePrefix(op, parts[i+1].getNode());
              parts.erase(parts.begin() + i + 1);
            } else if (ops.type == OperatorClass::Tertiary) {
              // we must be at  X ? Y : Z
              //                      ^
              //dumpParts(parts, i);
              if (op != COLON) continue;
              assert(i < parts.size()-1 && i >= 3);
              if (parts[i-2].getOp() != QUESTION) continue; // e.g. x ? y ? 1 : 0 : 2
              parts[i-3] = builder.makeConditional(parts[i-3].getNode(), parts[i-1].getNode(), parts[i+1].getNode());
              parts.erase(parts.begin() + i - 2, parts.begin() + i + 2);
              i = parts.size(); // basically a reset, due to things like x ? y ? 1 : 0 : 2
            } // TODO: postfix
          }
        } else {
          // left to right
          for (int i = 0; i < parts.size(); i++) {
            if (parts[i].isNode) continue;
            IString op = parts[i].getOp();
            if (!ops.ops.has(op)) continue;
            if (ops.type == OperatorClass::Binary && i > 0 && i < parts.size()-1) {
              parts[i] = builder.makeBinary(parts[i-1].getNode(), op, parts[i+1].getNode());
              parts.erase(parts.begin() + i + 1);
              parts.erase(parts.begin() + i - 1);
              i--;
            } else if (ops.type == OperatorClass::Prefix && i < parts.size()-1) {
              if (i > 0 && parts[i-1].isNode) continue; // cannot apply prefix operator if it would join two nodes
              parts[i] = builder.makePrefix(op, parts[i+1].getNode());
              parts.erase(parts.begin() + i + 1);
              i = std::max(i-2, 0); // allow a previous prefix operator to cascade
            } // TODO: tertiary, postfix
          }
        }
      }
      assert(parts.size() == 1);
      NodeRef ret = parts[0].getNode();
      parts.clear();
      return ret;
    }
  }

  // Parses a block of code (e.g. a bunch of statements inside {,}, or the top level of o file)
  NodeRef parseBlock(char*& src, NodeRef block=nullptr, const char* seps=";", IString keywordSep1=IString(), IString keywordSep2=IString()) {
    //dump("parseBlock", src);
    if (!block) block = builder.makeBlock();
    while (*src) {
      src = skipSpace(src);
      if (*src == 0) break;
      if (*src == ';') {
        src++; // skip a statement in this block
        continue;
      }
      if (hasChar(seps, *src)) break;
      if (!!keywordSep1) {
        Frag next(src);
        if (next.type == KEYWORD && next.str == keywordSep1) break;
      }
      if (!!keywordSep2) {
        Frag next(src);
        if (next.type == KEYWORD && next.str == keywordSep2) break;
      }
      NodeRef element = parseElementOrStatement(src, seps);
      builder.appendToBlock(block, element);
    }
    return block;
  }

  NodeRef parseBracketedBlock(char*& src, NodeRef block=nullptr) {
    if (!block) block = builder.makeBlock();
    src = skipSpace(src);
    assert(*src == '{');
    src++;
    parseBlock(src, block, ";}"); // the two are not symmetrical, ; is just internally separating, } is the final one - parseBlock knows all this
    assert(*src == '}');
    src++;
    return block;
  }

  NodeRef parseElementOrStatement(char*& src, const char *seps) {
    NodeRef ret = parseElement(src, seps);
    src = skipSpace(src);
    if (*src == ';') {
      ret = builder.makeStatement(ret);
      src++;
    }
    return ret;
  }

  NodeRef parseMaybeBracketed(char*& src, const char *seps) {
    src = skipSpace(src);
    return *src == '{' ? parseBracketedBlock(src) : parseElementOrStatement(src, seps);
  }

  NodeRef parseMaybeBracketedBlock(char*& src, const char *seps, IString keywordSep1=IString(), IString keywordSep2=IString()) {
    src = skipSpace(src);
    return *src == '{' ? parseBracketedBlock(src) : parseBlock(src, nullptr, seps, keywordSep1, keywordSep2);
  }

  NodeRef parseParenned(char*& src) {
    src = skipSpace(src);
    assert(*src == '(');
    src++;
    NodeRef ret = parseElement(src, ")");
    src = skipSpace(src);
    assert(*src == ')');
    src++;
    return ret;
  }

  // Debugging

  char *allSource;
  int allSize;

  static void dump(const char *where, char* curr) {
    /*
    printf("%s:\n=============\n", where);
    for (int i = 0; i < allSize; i++) printf("%c", allSource[i] ? allSource[i] : '?');
    printf("\n");
    for (int i = 0; i < (curr - allSource); i++) printf(" ");
    printf("^\n=============\n");
    */
    printf("%s:\n==========\n", where);
    int newlinesLeft = 2;
    int charsLeft = 200;
    while (*curr) {
      if (*curr == '\n') {
        newlinesLeft--;
        if (newlinesLeft == 0) break;
      }
      charsLeft--;
      if (charsLeft == 0) break;
      printf("%c", *curr++);
    }
    printf("\n\n");
  }

public:

  Builder builder;

  Parser(Builder&& builder = Builder()) : allSource(nullptr), allSize(0), builder(std::move(builder)) {
    expressionPartsStack.resize(1);
  }

  // Highest-level parsing, as of a JavaScript script file.
  NodeRef parseToplevel(char* src) {
    allSource = src;
    allSize = strlen(src);
    return parseBlock(src, builder.makeToplevel());
  }
};

} // namespace cashew

