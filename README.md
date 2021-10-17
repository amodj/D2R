# D2R Save file reader/editor (OFFLINE only)

This is a D2S file (Diablo Save file) reader and editor.
Implemented completely in C++ - purely out of curiosity and as a hobby for
reading and editing stats of OFFLINE characters.

## TL;DR
How to use the tool: [Usage](#usage)
## What can this tool do:
1. Read a .d2S save file and print out character attributes.
2. Edit the .d2S file with any attribute values desired.

## What this tool cannot do:
1. Edit any online character.
2. Add/edit any specific skills.
3. Add/edit any specific items.

## Save file format
A D2S file consists of a 765 byte header followed by the character attributes, skills and then items.

| Byte | Length |Description |
|------|--------|------------|
| 0 | 4 | Magic (0xaa55aa55) |
| 4 | 4 | Version (0x61 for D2R) |
| 8 | 4 | File size |
| 12 | 4 | Checksum |
| ... | ... | ... |
| 765 | 2 | Stats begin (0x6667) |

Details of the save file format are courtesy of: https://github.com/krisives/d2s-format

### Stats encoding
Actual stats start from byte 767 in the file. The format of the attributes is:

| attribute_id (9 bits) | attribute_value (variable length) |
|-----------------------|-----------------------------------|

Attribute lengths are variable and are defined as:

``` c++
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
```

Attribute IDs are encoded in `9 bits` and the values are:

``` c++
         "Strength" = 0,
         "Energy" = 1,
         "Dexterity" = 2,
         "Vitality" = 3,
         "Stats left" = 4,
         "Skills left" = 5,
         "HP" = 6,
         "Max HP" = 7,
         "Mana" = 8,
         "Max Mana" = 9,
         "Stamina" = 10,
         "Max Stamina" = 11,
         "Level" = 12,
         "Experience" = 13,
         "Gold" = 14,
         "Stash Gold" = 15
```
The end of the attribute section is determined by the presence of an "invalid" attribute_id - 9 bits of all 1s (`0x1ff` or `511`)

### Reading the stats
One important thing to note is that each byte in the stats section is actually `bit-reversed` and stored in the file.
For example, if the bit-stream in the file is : `00000000 00011110 00001000 10010000`, it needs to be interpreted as:
1. Reverse the bits: `00000000 01111000 00010000 00001001`
2. Read 9 bits:
``` text
00000000 01111000 00010000 00001001
^        ^
```
Gives: `000000000`

3. Reverse the 9 bits: `000000000` gives the attribute_id: 0 - `Strength`
4. Strength value is 10 bits, so read the next `10 bits`:
``` text
00000000 01111000 00010000 00001001
          ^         ^
```
gives: `1111000000`

5. Reverse the 10 bits: `0000001111` : 15
6. Read the next 9 bits for the next attribute:
``` text
00000000 01111000 00010000 00001001
                     ^        ^
```
gives: `100000000`

7. Reverse the 9 bits read: `000000001` gives the attribute_id: 1 - `Energy`
8. Read the next 10 bits... and so on, until we find the attribute_id: `111111111` or `0x1ff`

### Modfying the stats.
Stats that are to be modified can be put in the `edit.cfg` file:

   ``` text
   Strength=512
   Energy=230
   Dexterity=413
   Vitality=872
   Stats left=14
   Skills left=14
   HP=500
   Max HP=500
   Mana=500
   Max Mana=500
   Stamina=199
   Max Stamina=199
   Level=1
   Experience=0
   Gold=8000
   Stash Gold=1000
   ```
   Values which are `0` remain unmodified, else the value present in the `edit.cfg` file is added to the character.

## Usage
1. Compile the binary: `g++ d2r_edit.cc util.cc char.cc -o d2r`
2. Invoke the tool: `./d2r <d2s_file_path>`

Example:

``` text
$ bin/d2r ../Diablo\ II\ Resurrected/N_one.d2s
Processed 36 bytes
----------------------------
Name: N_one
Class: Necromancer
----------------------------
Strength: 1023
Energy: 25
Dexterity: 25
Vitality: 15
HP: 45
Max HP: 45
Mana: 57
Max Mana: 57
Stamina: 79
Max Stamina: 79
Level: 1
----------------------------
Processed 49 bytes
----------------------------
Name: N_one
Class: Necromancer
----------------------------
Strength: 512
Energy: 230
Dexterity: 413
Vitality: 872
Stats left: 14
Skills left: 14
HP: 500
Max HP: 500
Mana: 500
Max Mana: 500
Stamina: 199
Max Stamina: 199
Level: 1
Gold: 8000
Stash Gold: 1000
----------------------------
```
