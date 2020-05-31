#include "binhex.hpp"


static const char *bin2hexTable = "0123456789ABCDEF";


int uti_char2int(char input)
{
  if(input >= '0' && input <= '9') return input - '0';
  if(input >= 'A' && input <= 'F') return input - 'A' + 10;
  if(input >= 'a' && input <= 'f') return input - 'a' + 10;
  return -1;
}

/**
 * Indicate if the string only contains hexadecimal characters or not.
 *
 * @param[in] ps the string.
 *
 * @return true  if the string only contains hexadecimal characters.
 * @return false is ps is NULL or empty.
 * @return false if it contains a character that is not hexadecimal.
 */
bool is_hex_string(const char *ps)
{
  bool res = true;

  if(ps && *ps)
  {
    while(*ps) { if(uti_char2int(*ps++) < 0) { res = false; break; } }
  }
  else { res = false; }

  return res;
}

/**
 * Indicate if the string only contains hexadecimal characters or not.
 *
 * @param[in] ps  the string.
 * @param[in] len the string's expected length.
 *
 * @return true  if the string only contains hexadecimal characters.
 * @return false is ps is NULL or empty.
 * @return false if it contains a character that is not hexadecimal.
 * @return false if the string length is not the expected one.
 */
bool is_hex_string_of_len(const char *ps, uint32_t len)
{
  const char *ps_end = ps + len;
  bool        res    = true;

  if(ps && *ps)
  {
    while(ps <  ps_end && *ps) { if(uti_char2int(*ps++) < 0) { res = false; break; } }
    if(   ps != ps_end)        { res = false; }
  }
  else { res = false; }

  return res;
}

// This function assumes src to be a zero terminated sanitized string with
// an even number of [0-9a-f] characters, and target to be sufficiently large
void hex2bin(const char* src, char* target)
{
  while(*src && src[1]){
    *(target++) = uti_char2int(*src)*16 + uti_char2int(src[1]);
    src += 2;
  }
}

// Same as hex2bin but with a length
void n_hex2bin(const char* src, char* target, int length){
  for(int i = 0 ; i < 2*length ; i+=2){
    *(target++) = uti_char2int(*src)*16 + uti_char2int(src[1]);
    src += 2;
  }
}

void n_bin2hex(const char *src, char* target, int length)
{
  int i, j;
  int b = 0;

  for (i = j = 0; i < length; i++) {
    b = src[i] >> 4;
    target[j++] = (char) (87 + b + (((b - 10) >> 31) & -39));
    b = src[i] & 0xf;
    target[j++] = (char) (87 + b + (((b - 10) >> 31) & -39));
  }
  target[j] = '\0';

}

char *binToHex(char *dest, const uint8_t *pu8Data, uint32_t size, bool nullTerminated)
{
  char          *pc;
  const uint8_t *pu8End;

  for(pc = dest, pu8End = pu8Data + size ; pu8Data < pu8End; pu8Data++)
  {
    *pc++ = bin2hexTable[(*pu8Data >> 4) & 0x0F];
    *pc++ = bin2hexTable[ *pu8Data       & 0x0F];
  }
  if(nullTerminated) { *pc = '\0'; }

  return dest;
}
