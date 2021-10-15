/* The BitBuffer class allows one to read and write bits to a buffer. */

#ifndef __BIT_BUFFER_H_
#define __BIT_BUFFER_H_

#include "util.h"

namespace d2r { namespace util {

class BitBuffer {
  public:
    explicit BitBuffer(uint8_t *const data) :
      data_(data),
      bits_remaining_(0),
      num_bytes_processed_(0) {
    }

    // Same constructor as above, but load the first byte in-memory upfront.
    BitBuffer(uint8_t *data, const bool load_first):
      data_(data),
      cur_(*data),
      bits_remaining_(8),
      num_bytes_processed_(1) {

    }

    ~BitBuffer() = default;

    // Read the next "num_bits" bits from the buffer.
    unsigned int ReadBits(const int num_bits) {
      unsigned int r = 0;
      if (num_bits == 0) {
        return r;
      }

      if (bits_remaining_ == 0) {
        // We need to read from the next byte.
        cur_ = ReverseBits(data_[0], sizeof(uint8_t) << 3);
        // Advance the pointer to the next byte.
        ++data_;
        bits_remaining_ = 8;
        ++num_bytes_processed_;
      }

      int num_read_bits = num_bits;
      if (num_read_bits > bits_remaining_) {
        num_read_bits = bits_remaining_;
      }

      // Create a mask to remove unwanted bits from the byte.
      unsigned int mask = 0x0;
      for (int ii = 0; ii < num_read_bits; ++ii) {
        mask <<= 1;
        mask |= 0x01;
      }

      // Extract the required bits from the byte.
      const int shift = bits_remaining_ - num_read_bits;
      r = cur_ >> (bits_remaining_ - num_read_bits);
      r &= mask;

      // We have now consumed num_bits from this byte.
      bits_remaining_ -= num_read_bits;

      num_read_bits = num_bits - num_read_bits;
      // We still need to read more bits, but we will have to read
      // the next by first. Also left shift the bits that we read
      // to make space of the remaining bits that we will read.
      if (num_read_bits > 0) {
        r <<= num_read_bits;
        r |= ReadBits(num_read_bits);
      }
      return r;
  }

  // Write the value 'val' using 'num_bits' bits in the data buffer.
  void WriteBits(const unsigned int val, const int num_bits) {
    unsigned int r = val;

    if (num_bits == 0) {
      return;
    }

    // TODO: Realloc if we are running out of space in the buffer.

    if (bits_remaining_ == 0) {
      // We need to write to the next byte.
      ++data_;
      bits_remaining_ = 8;
      ++num_bytes_processed_;
    }

    int num_write_bits = num_bits;
    if (num_write_bits > bits_remaining_) {
      num_write_bits = bits_remaining_;
    }

    const int shift = (sizeof(uint8_t) << 3) - bits_remaining_;
    uint8_t c = *data_;

    c |= (r << shift);
    //cout << "C: " << ToBinaryString(c) << endl;
    *data_ = c;

    bits_remaining_ -= num_write_bits;

    r >>= num_write_bits;
    num_write_bits = num_bits - num_write_bits;
    if (num_write_bits > 0) {
      WriteBits(r, num_write_bits);
    }
  }

  int num_bytes_processed() const {
    return num_bytes_processed_;
  }

  private:
    // Pointer to the next byte to read.
    uint8_t *data_;

    // Number of bits remaining in the current byte.
    int bits_remaining_;

    // The current byte.
    uint8_t cur_;

    // Number of bytes processed.
    int num_bytes_processed_;
};
} } //namespace
#endif // __BIT_BUFFER_H_
