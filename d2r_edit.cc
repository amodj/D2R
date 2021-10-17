/*
 * A tool to read/edit Diablo 2 save files.
 *
 * Author: Amod Jaltade (amodjaltade@gmail.com)
 */

#include <assert.h>
#include <fcntl.h>
#include <iostream>
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

   // Parse the character stats.
   void ParseChar();

   // Print the character stats.
   void PrintChar();

   // Void write new stats to the character reading from edit.cfg.
   void WriteNewStats();

 private:
   // Get the checksum of [data, data + size) using the Diablo-2 algo.
   uint GetD2Checksum(const uint8_t *data, int size);

  // Read the contents of the file into data_.
  void ReadData();

 private:
   // File descriptor.
   int fd_;

   // File data cached in memory. These files are small, so this should be okay.
   uint8_t *data_;

   // File size.
   int fsz_;

   // The character.
   Character *c_;

   // Original size of the stats area in the file.
   int orig_stats_size_;
};

D2SEditor::D2SEditor(const string& filename):
  fd_(0),
  data_(nullptr),
  c_(nullptr),
  fsz_(0),
  orig_stats_size_(0) {

  fd_ = Open(filename.c_str());
}

D2SEditor::~D2SEditor() {
  delete []data_;
  delete c_;
  close(fd_);
}

void D2SEditor::ParseChar() {
  // Read the character from the file.
  ReadData();

  // Assert that the file size is correct in the file.
  assert(fsz_ == *(uint *)(data_ + kFileSizeOffset));

  // Parse the character now that we have read it from the file.
  c_ = new Character(data_);
  orig_stats_size_ = c_->Parse();
  PrintChar();
}

void D2SEditor::ReadData() {
  if (data_) {
    delete []data_;
  }

  // Seek to the end to get the file size.
  fsz_= lseek(fd_, 0, SEEK_END);

  data_ = new uint8_t[fsz_];
  if (!data_) {
    cout << "Unable to allocate memory" << endl;
    exit(1);
  }

  // Seek back to the start to read.
  lseek(fd_, 0, SEEK_SET);
  if (read(fd_, data_, fsz_) != fsz_) {
    cout << "Unable to read file" << endl;
    exit(1);
  }
}

void D2SEditor::PrintChar() {
  if (c_) {
    c_->Print();
  }
}

void D2SEditor::WriteNewStats() {
  // Allocate at least 'fsz_' sized buffer - to save the number of file-writes
  // (and some cushion), we will write the whole buffer just once instead of
  // breaking it into parts. These files are small - < 2KB.
  uint8_t *new_data = new uint8_t[fsz_ + 512];
  memset(new_data, 0, fsz_ + 512);
  int bytes_written = 0;

  // Copy the header and the magic number at the start of stats (0x6667) as is -
  // haven't changed that.
  memcpy(new_data, data_, kHeaderSize + 2);
  bytes_written += (kHeaderSize + 2);

  // Write the character stats now.
  bytes_written += c_->WriteInto(new_data + kHeaderSize + 2);

  // Remaining bytes to be written.
  const int remaining_size = fsz_ - (kHeaderSize + 2 + orig_stats_size_);
  const int remaining_offset = kHeaderSize + 2 + orig_stats_size_;

  memcpy(new_data + bytes_written, data_ + remaining_offset, remaining_size);
  bytes_written += remaining_size;

  // Update the file size.
  *(uint *)(new_data + kFileSizeOffset) = bytes_written;
  // Generate the new checksum.
  *(uint *)(new_data + kCksumOffset) = GetD2Checksum(new_data, bytes_written);

  lseek(fd_, 0, SEEK_SET);
  if (write(fd_, (void*)new_data, bytes_written) != bytes_written) {
    cout << "Unable to write new stats to file" << endl;
    exit(1);
  }

  delete []new_data;
}

uint D2SEditor::GetD2Checksum(const uint8_t *const buf, const int size) {
   int cksum = 0;

   for (int ii = 0; ii < size; ++ii) {
     uint8_t data_byte = buf[ii];
     if (ii >= kCksumOffset && ii < (kCksumOffset + kCksumSize)) {
       data_byte = 0;
     }
     cksum = (cksum << 1) + data_byte + (cksum < 0);
   }
   return cksum;
}

int main(const int argc, char *const argv[]) {
  if (argc < 2) {
    cout << "Enter the file name for calculating checksum as the first "
         << "argument" << endl;
    return 1;
  }

  D2SEditor editor(argv[1]);
  editor.ParseChar();
  editor.WriteNewStats();

  editor.ParseChar();
  return 0;
}
