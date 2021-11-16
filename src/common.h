#ifndef _COMMON_H
#define _COMMON_H

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


#define BYTES_PER_READING (2)
#define READINGS_PER_TRIPLET (3)
#define BYTES_PER_XYZ_READINGS_TRIPLET ( BYTES_PER_READING * READINGS_PER_TRIPLET )

struct acc_reading_triplet
{
    uint16_t x;
    uint16_t y;
    uint16_t z;
};


// 2021-11-05 - Ted looking for best language, we'll clean these 'set' and 'clear' symbols up:    - TMH
#define VALUE_OF_FLAG_SET     1
#define VALUE_OF_FLAG_CLEARED 0

#define SET_FLAG VALUE_OF_FLAG_SET
#define CLEAR_FLAG VALUE_OF_FLAG_CLEARED


// 2021-11-15 - output line and data formatting:
#define ONE_NEWLINE "\n\r"
#define TWO_NEWLINES "\n\r\n\r"


#endif // _COMMON_H
