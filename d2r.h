#ifndef __D2R_H_
#define __D2R_H_

#include <string>

namespace d2r {

  static const int kHeaderSize = 765; // 0x2FD
  static const int kCksumOffset = 12;
  static const int kCksumSize = sizeof(int);
  static const int kNameOffset = 20; // Name is 20 bytes from the start.
  static const int kNameLen = 16; // Max length of the name is 16 bytes.
  static const int kClassOffset = 40; // Class is 40 bytes from the start.
  static const int kClassLen = 1;

  static const int kAttribIdLen = 9; // Attributes are stored in 9 bits.
  static const int kInvalidAttrib = 511; // 9 bits of all 1s.

  // Types of classes in D2R.
  static const std::string kClasses[] = {
      "Amazon",
      "Sorceress",
      "Necromancer",
      "Paladin",
      "Barbarian",
      "Druid",
      "Assasin"
  };

  // Attributes of a character.
  static const std::string kAttributes[] = {
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

} // namespace
#endif // __D2R_H_
