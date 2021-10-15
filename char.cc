#include <iostream>

#include "bit_buffer.h"
#include "char.h"
#include "d2r.h"

using namespace std;
using namespace d2r;
using namespace d2r::util;

const int edits[] = { 1023, 123, 512, 313};

namespace d2r {

Character::Character(uint8_t *data, const int size):
  data_(data) {
  Parse(data_, size);
}

void Character::Parse(uint8_t *data, const int size) {
  d2s_t *pdata = (d2s_t *)(data);

  // Parse some of the header.
  uint8_t *header = pdata->header;

  // Parse the name and class of the character.
  name_ = string((char*)(pdata->header + kNameOffset));
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
      // Not the most correct way to do this, but it will work fine because
      // although attributes are stored in 9 bits, only 4 are ever really used.
      break;
    }
    val = ReverseBits(b.ReadBits(kAttribLen[attrib]), kAttribLen[attrib]);
    attrib_map_[attrib] = val >> kAttribTransform[attrib];
  }
  cout << "Processed " << b.num_bytes_processed() << " bytes" << endl;
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

  return b.num_bytes_processed();
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
} // namespace
