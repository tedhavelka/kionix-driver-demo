#ifndef _CONVERSIONS_H
#define _CONVERSIONS_H


#define APPR_ACCELERATION_OF_GRAVITY (9.80665)

#define BINARY_REPRESENTATION_EIGHT_BITS_AS_STRING (8)
#define BINARY_REPRESENTATION_SIXTEEN_BITS_AS_STRING (16)
#define BINARY_REPRESENTATION_THIRTY_TWO_BITS_AS_STRING (32)



float reading_in_g(const uint32_t reading_in_twos_comp, const uint32_t full_scale, const uint32_t resolution_in_bits);

void integer_to_binary_string(const uint32_t integer, char* string, const uint32_t str_length);



#endif // _CONVERSIONS_H
