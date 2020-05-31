/**********************************************************/
//   ______                           _     _________
//  / _____)  _                      / \   (___   ___)
// ( (____  _| |_ _____  _____      / _ \      | |
//  \____ \(_   _) ___ |/ ___ \    / /_\ \     | |
//  _____) ) | |_| ____( (___) )  / _____ \    | |
// (______/   \__)_____)|  ___/  /_/     \_\   |_|
//                      | |
//                      |_|
/**********************************************************/
/* Bin - Hex - Ascii Toolkit ******************************/
/**********************************************************/
#pragma once

#include "defs.h"

int uti_char2int(char input);

bool is_hex_string(       const char *ps);
bool is_hex_string_of_len(const char *ps, uint32_t len);

void  hex2bin(  const char* src,  char*   target);
void  n_hex2bin(const char* src,  char*   target, int length);
void  n_bin2hex(const char* src,  char*   target, int length);
char *binToHex( char       *dest, const uint8_t *pu8Data, uint32_t size, bool nullTerminated);
