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

using namespace std;
using namespace std::placeholders;

namespace {
  // Return litte-endian of a given integer.
  template <typename T>
  T LittleEndian(const T num) {
    T le = 0;
    for (int ii = 0; ii < sizeof(T); ++ii) {
      unsigned char b = num >> ((sizeof(T) - ii - 1) << 3) & 0xff;
      le |= b << (ii << 3);
    }
    return le;
  }

unsigned short LittleEndian(unsigned short num);
unsigned int LittleEndian(unsigned int num);

  int Open(const char *file) {
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

  template<typename T>
  static string ToBinaryString(const T& x) {
    stringstream ss;
    ss << bitset<sizeof(T) * 8>(x);
    return ss.str();
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
} // anonymous namespace.

static const int kHeaderSize = 765;
static const int kCksumOffset = 12;
static const int kCksumSize = sizeof(int);
static const int kNameOffset = 20; // Name is 20 bytes from the start.
static const int kNameLen = 16; // Max length of the name is 16 bytes.
static const int kClassOffset = 40; // Class is 40 bytes from the start.
static const int kClassLen = 1;

static const int kAttribIdLen = 9; // Attributes are stored in 9 bits.
static const int kInvalidAttrib = 511; // 9 bits of all 1s.

// Types of classes in D2R.
string kClasses[] = {
  "Amazon",
  "Sorceress",
  "Necromancer",
  "Paladin",
  "Barbarian",
  "Druid",
  "Assasin"
};

// Attributes of a character.
static const string kAttributes[] = {
  "Strength",
  "Engergy",
  "Dexterity",
  "Vitality",
  "Stats left",
  "Skills left",
  "HP",
  "Max HP",
  "Mana",
  "Max Mana",
  "Stamina",
  "Max Stamina",
  "Level",
  "Experience",
  "Gold",
  "Stash Gold"
};

// Lengths of the attributes stored in the file.
static const int kAttribLen[] = {
  10, // Strength
  10, // Energy
  10, // Dex
  10, // Vit
  10, // Stats left
  8, // Skills left
  21, // HP
  21, // Max HP
  21, // Mana
  21, // Max Mana
  21, // Stamina
  21, // Max stamina
  7, // Level
  32, // Experience,
  25, // Gold
  25 // Stashed Gold
};

// Attribute values obtained should be right-shifted (reading) or
// left-shifted (writing) by the respective number to get the
// real value.
static const uint8_t kAttribTransform[] = {
  0, // Strength
  0, // Energy
  0, // Dex
  0, // Vit
  0, // Stats left
  0, // Skills left
  8, // HP
  8, // Max HP
  8, // Mana
  8, // Max Mana
  8, // Stamina
  8, // Max stamina
  0, // Level
  0, // Experience,
  0, // Gold
  0 // Stashed Gold
};

typedef struct __attribute__((__packed__)) {
  char header[kHeaderSize];
  char stats[0];
} d2s_t;

class Character {
  public:
    Character(const char *data, const int size);
    ~Character() = default;

    void Print();

  private:
    void Parse(const char *data, const int size);

  private:
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

 private:
   // Get the checksum of 'data_' using the Diablo-2 algo.
   int GetD2Checksum();

  // Read the contents of the file into data_.
  void ReadData();

  // Parse the character stats.
  void ParseChar();

 private:
   // File data cached in memory. These files are small, so this should be okay.
   char *data_;

   // File descriptor.
   int fd_;

   // File size.
   int fsz_;

   // The character.
   Character *c_;
};

D2SEditor::D2SEditor(const string& filename) {
  fd_ = Open(filename.c_str());
  fsz_ = GetFileSize(filename.c_str());

  ReadData();
  ParseChar();
  PrintChar();
}

D2SEditor::~D2SEditor() {
  delete data_;
  delete c_;
  close(fd_);
}

void D2SEditor::ReadData() {
  data_ = new char[fsz_];
  if (!data_) {
    cout << "Unable to allocate memory" << endl;
    exit(1);
  }

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

Character::Character(const char *const data, const int size) {
  Parse(data, size);
}

class BitBuffer {
  public:
    explicit BitBuffer(char *data) :
       data_(data),
       bits_remaining_(0) {

    }
    ~BitBuffer() = default;

    unsigned int ReadBits(const int num_bits) {
      unsigned int r = 0;
      if (num_bits == 0) {
        return r;
      }

      if (bits_remaining_ == 0) {
        // We need to read from the next byte.
        cur_ = ReverseBits(data_[0], sizeof(char) << 3);
        // Advance the pointer to the next byte.
        ++data_;
        bits_remaining_ = 8;
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

  private:
    // Pointer to the next byte to read.
    const char *data_;

    // Number of bits remaining in the current byte.
    int bits_remaining_;

    // The current byte.
    unsigned char cur_;
};

void Character::Parse(const char *const data, const int size) {
  d2s_t *pdata = (d2s_t *)(data);

  // Parse some of the header.
  char *header = pdata->header;

  // Parse the name and class of the character.
  name_ = string(pdata->header + kNameOffset);
  class_ = kClasses[pdata->header[kClassOffset]];

  unsigned short *attrib_bytes = (unsigned short *)pdata->stats;
  // Skip the first 16 bits - '0x6667'
  ++attrib_bytes;

  BitBuffer b((char *)attrib_bytes);

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

