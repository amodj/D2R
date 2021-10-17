/*
 * Author: Amod Jaltade (amodjaltade@gmail.com)
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>

using namespace std;

namespace d2r { namespace util {

  int Open(const char *const file) {
    const int fd = open(file, O_RDWR);
    if (fd < 0) {
      cout << "Unable to open file" << endl;
      exit(1);
    }
    return fd;
  }

  int GetFileSize(const char *const filename) {
    struct stat st;
    stat(filename, &st);
    return st.st_size;
  }

  unsigned int ReverseBits(const unsigned int in_num, const int num_bits) {
    unsigned int result = 0;
    unsigned int num = in_num;
    for (int ii = 0; ii < num_bits; ++ii) {
       result <<= 1;
       result |= (num & 0x01);
       num >>= 1;
    }
    return result;
  }
} } // namespace
