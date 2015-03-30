#ifndef BASE_SECURITY_NEUTRAL_H
#define BASE_SECURITY_NEUTRAL_H
#include "base64.h"
#include "random.h"
using namespace myrand;
extern void get_encoded_table_my_neutral(char * encoded_table, unsigned int in_len);
extern std::string encode_security_neutral(char const* bytes_to_encode, unsigned int in_len);
extern std::string decode_security_neutral(std::string & encoded_string);
#endif
