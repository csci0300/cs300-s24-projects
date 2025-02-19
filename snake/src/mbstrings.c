#include "mbstrings.h"

#include <stddef.h>

/* Helper function that gets the amount of bytes in the unicode encoding
of a character*/
int num_unicode_elts(char* ptr) {
    int num_elts = 0;
    int zero_comp = 0b1000000; 
    int two_leading_comp = 0b11100000; 
    int three_leading_comp = 0b11110000;
    int four_leading_comp = 0b11111000;
    if ((*ptr & zero_comp) == 0) { //0xxxxxxx & 10000000 = 0
        //means that we have a leading zero
        num_elts = 1;
    } else if ((*ptr & two_leading_comp) == 0b11000000) { //110xxxxx & 11100000 = 11000000 = 192
        num_elts = 2;
    } else if ((*ptr & three_leading_comp) == 0b11100000) { //1110xxxx & 11110000 = 11100000 = 
        num_elts = 3;
    } else if ((*ptr & four_leading_comp) == 0b11110000) { //11111000 & 11110000 = 11110000
        num_elts = 4;
    } else {
        num_elts = -1;
    }
    return num_elts;
}

/* mbslen - multi-byte string length
 * - Description: returns the number of UTF-8 code points ("characters")
 * in a multibyte string. If the argument is NULL or an invalid UTF-8
 * string is passed, returns -1.
 *
 * - Arguments: A pointer to a character array (`bytes`), consisting of UTF-8
 * variable-length encoded multibyte code points.
 *
 * - Return: returns the actual number of UTF-8 code points in `src`. If an
 * invalid sequence of bytes is encountered, return -1.
 *
 * - Hints:
 * UTF-8 characters are encoded in 1 to 4 bytes. The number of leading 1s in the
 * highest order byte indicates the length (in bytes) of the character. For
 * example, a character with the encoding 1111.... is 4 bytes long, a character
 * with the encoding 1110.... is 3 bytes long, and a character with the encoding
 * 1100.... is 2 bytes long. Single-byte UTF-8 characters were designed to be
 * compatible with ASCII. As such, the first bit of a 1-byte UTF-8 character is
 * 0.......
 *
 * You will need bitwise operations for this part of the assignment!
 */
size_t mbslen(const char* bytes) {
    // TODO: implement!

    if (!bytes) {
        return -1;
    }

    size_t num_chars = 0;
    char* ptr = (char*)bytes;


    while ((*ptr)== '\0') {
        int num_bytes_in_char = num_unicode_elts(ptr);
        if (num_bytes_in_char == -1) {
            return -1;
        } else if (num_bytes_in_char == 1) {
            //if it's a one byte char
            num_chars++;
            ptr = ptr + 1;
        } else {
            //if it's a more than one byte char
            for (int i = 0; i < num_bytes_in_char; i++) {
                //checking the next bytes to make sure they have leading ones
                if ((*(ptr + i) & 0b10000000) != 0b10000000) {
                    return -1;
                }
            }
            num_chars++;
            //skip over the next bytes to get to the next character
            ptr = ptr + num_bytes_in_char;
        }
    }
    return num_chars;
}
