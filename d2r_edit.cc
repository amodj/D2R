#include <bitset>
#include <cstring>
#include <functional>
#include <fcntl.h>
#include <arpa/inet.h>
#include <iostream>
#include <map>
#include <tuple>
#include <sys/stat.h>
#include <sstream>
#include <unistd.h>

#include "d2r.h"
#include "util.h"

using namespace std;
using namespace std::placeholders;

using namespace d2r;
using namespace d2r::util;

const int edits[] = { 1023, 123, 512, 313};

class Character {
  public:
    Character(uint8_t *data, const int size);
    ~Character() = default;

    void Print();

    int WriteInto(uint8_t *buf);

  private:

    void Parse(uint8_t *data, const int size);

  private:
    // Raw data of the character.
    uint8_t *data_;

    // Name of the character.
    string name_;

    // Class of the character.
    string class_;

    // Map from attribute to the value of the attribute.
    map<uint16_t, unsigned int> attrib_map_;
};

class D2SEditor {
 public:
   explicit D2SEditor(const string& filename);
   ~D2SEditor();

   // Write new checksum of the file to the file.
   void WriteNewChecksum();

   // Print the character stats.
   void PrintChar();

   void WriteChar();

 private:
   // Get the checksum of 'data_' using the Diablo-2 algo.
   int GetD2Checksum();

  // Read the contents of the file into data_.
  void ReadData();

  // Parse the character stats.
  void ParseChar();

  void WriteNewStats(const uint8_t *buf, int size);

 private:
   // File data cached in memory. These files are small, so this should be okay.
   uint8_t *data_;

   // File descriptor.
   int fd_;

   // File size.
   int fsz_;

   // The character.
   Character *c_;
};

D2SEditor::D2SEditor(const string& filename):
  data_(nullptr) {

  fd_ = Open(filename.c_str());
  fsz_ = GetFileSize(filename.c_str());

  ReadData();
  ParseChar();
  PrintChar();
  WriteChar();

  ReadData();
  ParseChar();
  cout << "Edited char: " << endl;
  PrintChar();
}

D2SEditor::~D2SEditor() {
  delete []data_;
  delete c_;
  close(fd_);
}

void D2SEditor::ReadData() {
  if (data_) {
    delete []data_;
  }
  data_ = new uint8_t[fsz_];
  if (!data_) {
    cout << "Unable to allocate memory" << endl;
    exit(1);
  }

  lseek(fd_, 0, SEEK_SET);
  if (read(fd_, data_, fsz_) != fsz_) {
    cout << "Unable to read file" << endl;
    exit(1);
  }
}

void D2SEditor::ParseChar() {
  c_ = new Character(data_, fsz_);
}

void D2SEditor::PrintChar() {
  if (c_) {
    c_->Print();
  }
}

void D2SEditor::WriteChar() {
  uint8_t buf[100];
  memset(buf, 0, 100);
  const int bytes_written = c_->WriteInto(buf);

  WriteNewStats(buf, bytes_written);
}

int D2SEditor::GetD2Checksum() {
   int cksum = 0;
   int total_bytes_read = 0;

   for (int ii = 0; ii < fsz_; ++ii) {
     unsigned char data_byte = data_[ii];
     if (ii >= kCksumOffset && ii < (kCksumOffset + kCksumSize)) {
       data_byte = 0;
     }
     cksum = (cksum << 1) + data_byte + (cksum < 0);
   }
   return cksum;
}

void D2SEditor::WriteNewStats(const uint8_t *const buf, const int sz) {
  if (lseek(fd_, 767, SEEK_SET)!= 767) {
    cout << "Unable to write checksum to file" << endl;
    exit(1);
  }

  if (write(fd_, (void*)buf, sz) != sz) {
    cout << "Unable to write to file" << endl;
    exit(1);
  }
}

void D2SEditor::WriteNewChecksum() {
  const int cksum = GetD2Checksum();
  //cout << "File checksum: " << hex << cksum << endl;

  if (lseek(fd_, kCksumOffset, SEEK_SET) != kCksumOffset) {
    cout << "Unable to write checksum to file" << endl;
    exit(1);
  }

  if (write(fd_, (void*)&cksum, sizeof(cksum)) != sizeof(cksum)) {
    cout << "Unable to write to file" << endl;
    exit(1);
  }
}

Character::Character(uint8_t *data, const int size):
  data_(data) {
  Parse(data_, size);
}

class BitBuffer {
  public:
    explicit BitBuffer(uint8_t *data) :
      data_(data),
      bits_remaining_(0),
      num_bytes_proc_(0) {
    }

    BitBuffer(uint8_t *data, const bool load_first):
      data_(data),
      cur_(*data),
      bits_remaining_(8),
      num_bytes_proc_(1) {

    }

    ~BitBuffer() = default;

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
        ++num_bytes_proc_;
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

  void WriteBits(const unsigned int val, const int num_bits) {
    //cout << "Writing -- " << val << ", " << num_bits << endl;
    unsigned int r = val;

    if (num_bits == 0) {
      return;
    }

    // TODO: Realloc if we are running out of space in the buffer.

    if (bits_remaining_ == 0) {
      // We need to write to the next byte.
      ++data_;
      bits_remaining_ = 8;
      ++num_bytes_proc_;
    }

    int num_write_bits = num_bits;
    if (num_write_bits > bits_remaining_) {
      num_write_bits = bits_remaining_;
    }

    const int shift = (sizeof(uint8_t) << 3) - bits_remaining_;
    uint8_t c = *data_;

    //cout << "Prev C: " << ToBinaryString(c) << " Shift " << shift << " val: " << r << endl;
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

  int num_bytes_proc() const {
    return num_bytes_proc_;
  }

  private:
    // Pointer to the next byte to read.
    uint8_t *data_;

    // Number of bits remaining in the current byte.
    int bits_remaining_;

    // The current byte.
    uint8_t cur_;

    // Number of bytes processed.
    int num_bytes_proc_;
};

void Character::Parse(uint8_t *data, const int size) {
  d2s_t *pdata = (d2s_t *)(data);

  // Parse some of the header.
  char *header = pdata->header;

  // Parse the name and class of the character.
  name_ = string(pdata->header + kNameOffset);
  class_ = kClasses[pdata->header[kClassOffset]];

  uint8_t *attrib_bytes = (uint8_t *)pdata->stats;
  // Skip the first 16 bits - '0x6667'
  attrib_bytes +=2;

  BitBuffer b((uint8_t *)attrib_bytes);

  for (;;) {
    uint16_t attrib;
    unsigned int val;

    attrib = ReverseBits(b.ReadBits(kAttribIdLen), kAttribIdLen);
    if (attrib == kInvalidAttrib) {
      // Not the most correct way to do this, but it will work fine because although attributes
      // are stored in 9 bits, only 4 are ever really used.
      break;
    }
    val = ReverseBits(b.ReadBits(kAttribLen[attrib]), kAttribLen[attrib]);
    attrib_map_[attrib] = val >> kAttribTransform[attrib];
  }
  cout << "Processed " << b.num_bytes_proc() << " bytes" << endl;
}

int Character::WriteInto(uint8_t *data) {
  BitBuffer b(data, true /* load_first */);

  for (auto attrib: attrib_map_) {
    b.WriteBits(attrib.first, kAttribIdLen);
    int val = attrib.second << kAttribTransform[attrib.first];
    const int bit_len = kAttribLen[attrib.first];
    if (attrib.first < 4) {
      val = edits[attrib.first];
    }
    b.WriteBits(val, bit_len);
  }
  b.WriteBits(0xff, 8);
  b.WriteBits(0x1, 1);

/*  cout << "Wrote " << b.num_bytes_proc() << " bytes" << endl;
  for (int ii = 1; ii <= 100; ++ii) {
    cout << ToBinaryString(data[ii - 1]) << " ";
    if (!(ii % 5)) {
      cout << endl;
    }
  }
  cout << endl;
*/
  return b.num_bytes_proc();
}

void Character::Print() {
  cout << "---------------------------- " << endl;
  cout << "Name: " << name_ << endl;
  cout << "Class: " << class_ << endl;

  cout << "---------------------------- " << endl;
  for (auto attrib : attrib_map_) {
    cout << kAttributes[attrib.first] << ": " << attrib.second << endl;
  }

  cout << "---------------------------- " << endl;
}

int main(const int argc, char *const argv[]) {
  if (argc < 2) {
    cout << "Enter the file name for calculating checksum as the first argument" << endl;
    return 1;
  }

  D2SEditor editor(argv[1]);
  editor.WriteNewChecksum();

  return 0;
}
