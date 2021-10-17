/*
 * Some utility functions for the tool.
 *
 * Author: Amod Jaltade (amodjaltade@gmail.com)
 */

#ifndef __UTIL_H_
#define __UTIL_H_

#include <bitset>
#include <cstring>
#include <string>
#include <sstream>

namespace d2r { namespace util {

  // Open a file and check for error.
  int Open(const char *file);

  // Get the file size of 'filename' in bytes.
  int GetFileSize(const char *filename);

  // Print a binary string for any given number. Useful for debugging.
  template<typename T>
  static std::string ToBinaryString(const T& x) {
    std::stringstream ss;
    ss << std::bitset<sizeof(T) * 8>(x);
    return ss.str();
  }

  // From a give number 'in_num', reverse the first 'num_bits' bits, starting
  // froim the LSB.
  unsigned int ReverseBits(unsigned int in_num, int num_bits);
}} // namespace

#endif // __UTIL_H_
