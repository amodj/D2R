/*
 * The Character class describes a character along with its attributes.
 *
 * Author: Amod Jaltade (amodjaltade@gmail.com)
 */

#ifndef __CHAR_H_
#define __CHAR_H_

#include <string>
#include <map>

namespace d2r {

class Character {
  public:
    Character(uint8_t *data);
    ~Character() = default;

    // Parse the character attributes from buffer 'data_' and
    // store it in-memory. Return the number of bytes processed.
    int Parse();

    // Print the character details on the console.
    void Print();

    // Write the character's attributes into the buffer pointed to by 'buf'.
    int WriteInto(uint8_t *buf);

  private:
    // Raw data of the character.
    uint8_t *data_;

    // Name of the character.
    std::string name_;

    // Class of the character.
    std::string class_;

    // Map from attribute to the value of the attribute of the character.
    std::map<uint16_t, unsigned int> attrib_map_;
};

} // namespace

#endif // __CHAR_H_
