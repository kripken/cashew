// Interned String type, 100% interned on creation. Comparisons are always just a pointer comparison

#include <unordered_set>

#include <string.h>
#include <stdint.h>
#include <assert.h>

namespace cashew {

struct IString {
  const char *str;

  static size_t hash_c(const char *str) { // TODO: optimize?
    uint64_t ret = 0;
    while (*str) {
      ret = (ret*6364136223846793005ULL) + *str;
      str++;
    }
    return (size_t)ret;
  }

  class CStringHash : public std::hash<const char *> {
  public:
    size_t operator()(const char *str) const {
      return IString::hash_c(str);
    }
  };
  class CStringEqual : public std::equal_to<const char *> {
  public:
    bool operator()(const char *x, const char *y) const {
      return strcmp(x, y) == 0;
    }
  };
  typedef std::unordered_set<const char *, CStringHash, CStringEqual> StringSet;
  static StringSet strings;

  IString() : str(nullptr) {}
  IString(const char *s) { // input is assumed to remain alive; not copied
    set(s);
  }

  void set(const char *s) {
    auto result = strings.insert(s); // if already present, does nothing
    str = *(result.first);
  }

  void set(const IString &s) {
    str = s.str;
  }

  bool operator==(const IString& other) const {
    //assert((str == other.str) == !strcmp(str, other.str));
    return str == other.str; // fast!
  }
  bool operator!=(const IString& other) const {
    //assert((str == other.str) == !strcmp(str, other.str));
    return str != other.str; // fast!
  }

  char operator[](int x) {
    return str[x];
  }

  bool operator!() { // no string, or empty string
    return !str || str[0] == 0;
  }

  const char *c_str() const { return str; }

  bool isNull() { return str == nullptr; }
};

} // namespace cashew

