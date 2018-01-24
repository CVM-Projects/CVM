#ifndef _CVM_BASIC_H_
#define _CVM_BASIC_H_
// Includes
#include <string>
#include <cctype>
#include <memory>
#include <vector>
#include <stack>
#include <map>
#include <list>
#include <functional>
#include <algorithm>
#include <initializer_list>
#include <cassert>
#include <bitset>
#include <utility>
#include "../prilib/prilib.h"

// Declares
using int_t = std::conditional_t<std::is_same<size_t, uint64_t>::value, int64_t, int32_t>;
using uint_t = std::conditional_t<std::is_same<size_t, uint64_t>::value, uint64_t, uint32_t>;
using inth_t = std::conditional_t<std::is_same<size_t, uint64_t>::value, int32_t, int16_t>;
using uinth_t = std::conditional_t<std::is_same<size_t, uint64_t>::value, uint32_t, uint16_t>;

// Using
using std::string;
using std::vector;
using std::map;
using std::stack;
using std::shared_ptr;
using std::unique_ptr;
using std::list;
using std::bitset;
using std::to_string;

using PriLib::Output::print;
using PriLib::Output::println;
using PriLib::Output::putError;

#endif
