#include <iostream>
#include <fstream>
#include "bit_buffer.h"
#include "char.h"
#include "d2r.h"

using namespace std;
using namespace d2r;
using namespace d2r::util;

const int edits[] = { 1023, 123, 512, 313};

namespace {
  // Generate attribute-edit map from "edit.cfg" file.
  map<uint16_t, unsigned int> AttributeEditMap() {
    ifstream in_file("edit.cfg");
    string line;

    map<uint16_t, unsigned int> edit_map;

    while (getline(in_file, line)) {
      istringstream in_line(line);
      string key;
      if(getline(in_line, key, '=') ) {
        string value;
        if(getline(in_line, value)) {
          edit_map[kAttributeMap.at(key)] = atoi(value.c_str());
        }
      }
    }
    return edit_map;
  }
} // anonymous

namespace d2r {

Character::Character(uint8_t *data):
  data_(data) {
}

int Character::Parse() {
  d2s_t *pdata = (d2s_t *)(data_);

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
      // Reached the end of stats.
      break;
    }
    val = ReverseBits(b.ReadBits(kAttribLen[attrib]), kAttribLen[attrib]);
    attrib_map_[attrib] = val >> kAttribTransform[attrib];
  }
  cout << "Processed " << b.num_bytes_processed() << " bytes" << endl;
  return b.num_bytes_processed();
}

int Character::WriteInto(uint8_t *data) {
  map<uint16_t, unsigned int> edit_map = AttributeEditMap();

  BitBuffer b(data, true /* load_first */);

  // First write anything that's already present in the character stats,
  // overwriting it with values from edit.cfg.
  for (auto attrib: attrib_map_) {
    b.WriteBits(attrib.first, kAttribIdLen);
    int val = attrib.second;
    const int len = kAttribLen[attrib.first];

    if (edit_map.find(attrib.first) != edit_map.end()) {
      const unsigned int edit_val = edit_map.at(attrib.first);
      if (edit_val > 0) {
        val = edit_val;
        edit_map.erase(attrib.first);
      }
    }
    val <<= kAttribTransform[attrib.first];
    b.WriteBits(val, len);
  }

  // Any  remaining attributes which we want to add to the character go here.
  for (auto attrib : edit_map) {
    const int val = attrib.second << kAttribTransform[attrib.first];
    const int len = kAttribLen[attrib.first];
    if (val > 0) {
      b.WriteBits(attrib.first, kAttribIdLen);
      b.WriteBits(val, len);
    }
  }

  // Mark the end of attributes.
  b.WriteBits(kInvalidAttrib, kAttribIdLen);

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
