#include <bitset>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sstream>
#include <unistd.h>

#include "char.h"
#include "d2r.h"
#include "util.h"

using namespace std;

using namespace d2r;
using namespace d2r::util;

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

  // Read the character from the file.
  ReadData();
  ParseChar();
  PrintChar();

  // Write new attributes.
  WriteChar();

  // Read back the new attributes.
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


int main(const int argc, char *const argv[]) {
  if (argc < 2) {
    cout << "Enter the file name for calculating checksum as the first "
         << "argument" << endl;
    return 1;
  }

  D2SEditor editor(argv[1]);
  editor.WriteNewChecksum();

  return 0;
}
