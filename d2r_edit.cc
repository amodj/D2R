#include <bitset>
#include <functional>
#include <fcntl.h>
#include <arpa/inet.h>
#include <iostream>
#include <unordered_map>
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
    //result = num << ((sizeof(unsigned int) << 3) - num_bits);
    int b = num_bits;
    for (int ii = 0; ii < b; ++ii) {
       result <<= 1;
       result |= (num & 0x01);
       num >>= 1;
    //   cout << dec << ii << " Inter: " << ToBinaryString(result) << endl;
    }
    //cout << "In: " << ToBinaryString(in_num) << " Rev: " << ToBinaryString(result) << endl; 
    return result;  
  }
} // anonymous namespace.

static const int kHeaderSize = 765;
static const int kCksumOffset = 12;
static const int kCksumSize = sizeof(int);

string kAttributes[] = {
  "Strength",
  "Engergy",
  "Dexterity",
  "Vitality",
  "Unused",
  "Unused",
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

int kAttribLen[] = {
  10, // Strength
  10, // Energy
  10, // Dex
  10, // Vit
  10, // Unused
  8, // Unused
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

typedef struct __attribute__((__packed__)) {
  char header[kHeaderSize];
  char stats[0];
} d2s_t;

class Character {
  public:
    Character(const char *data, const int size);
    ~Character() = default;

  private:
    void Parse(const char *data, const int size);

  private:
    // Name of the character.
    string name_;
    
    // Map from attribute to the value of the attribute.
    unordered_map<int, int> attrib_map_;
};

class D2SEditor {
 public:
   explicit D2SEditor(const string& filename);
   ~D2SEditor();

   // Write new checksum of the file to the file.
   void WriteNewChecksum();
   
   // Print the character stats.
   void PrintStats();

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
  cout << "File checksum: " << hex << cksum << endl;
 
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

typedef struct __attribute__((__packed__)) {
   unsigned int attrib_id_ : 9;
} attrib_id_t;


class BitBuffer {
  public:
    explicit BitBuffer(char *data) :
       data_(data),
       bits_remaining_(0) {       
      
    }
    ~BitBuffer() = default;

    unsigned int ReadBits(const int num_bits) {
      //cout << " -- Reading " << dec << num_bits << " bits " << endl;
      unsigned int r = 0; 
      if (num_bits == 0) {
        return r;
      }

      if (bits_remaining_ == 0) {
        // We need to read from the next byte.
        cur_ = ReverseBits(data_[0], sizeof(char) << 3);
        //cout << "Refreshed current: " << ToBinaryString(cur_) << endl;
        // Advance the pointer.
        ++data_;
        bits_remaining_ = 8;
      }
      
      int num_read_bits = num_bits;
      if (num_read_bits > bits_remaining_) {
        num_read_bits = bits_remaining_;
      }

      unsigned int mask = 0x0;
      for (int ii = 0; ii < num_read_bits; ++ii) {
        mask <<= 1;
        mask |= 0x01;
      }
     
      //cout << "Mask: " << hex << mask << endl;
      const int shift = bits_remaining_ - num_read_bits;
      //cout << "Shift: " << dec << shift << " B: " << bits_remaining_
      //     << " C: " << num_read_bits << endl;
      r = cur_ >> (bits_remaining_ - num_read_bits);   
      r &= mask;
     
      // We have now consumed num_bits from this byte.
      bits_remaining_ -= num_read_bits;
      
      //tie(r, num_read) = ReadBitsInternal(sizeof(char) << 3);
      num_read_bits = num_bits - num_read_bits;
      if (num_read_bits > 0) {
        //cout << "Reading " << num_bits << endl;
        r <<= num_read_bits;
        r |= ReadBits(num_read_bits);
      }
      //r = ReverseBits(r, num_bits);
      return r;
  }
  //private:

  private:
    const char *data_;
    int bits_remaining_;
    unsigned char cur_;
};

void Character::Parse(const char *const data, const int size) {
  d2s_t *pdata = (d2s_t *)(data);

  const int stats_size = size - kHeaderSize;

  unsigned short *attrib_bytes = (unsigned short *)pdata->stats;

  // Skip the first 16 bits - '0x6667'
  ++attrib_bytes;
  int ii = 0;
  BitBuffer b((char *)attrib_bytes);


  uint16_t d = b.ReadBits(9);
//  cout << "Val: " << ReverseBits(d, 9) << endl;
  d = b.ReadBits(10);
  cout << "Strength: " << ReverseBits(d, 10) << endl;

  b.ReadBits(9);
  d = b.ReadBits(10);
  cout << "Energy: " << ReverseBits(d, 10) << endl;

  b.ReadBits(9);
  d = b.ReadBits(10);
  cout << "Dex: " << ReverseBits(d, 10) << endl;

  b.ReadBits(9);
  d = b.ReadBits(10);
  cout << "Vitality: " << ReverseBits(d, 10) << endl;

  // Attributes end with the hex code 0x1ff.
  while (attrib_bytes[ii] != 0x1ff) {
   //cout << "Byte " << ii << ": " << hex << attrib_bytes[ii] << endl;
   ++ii;
  }
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

