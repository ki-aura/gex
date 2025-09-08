#include <stdio.h>


typedef struct {
    union {
        unsigned char byte;
        struct {
            unsigned char low  : 4;
            unsigned char high : 4;
        };
    };
} HexChar;


int main()
{

HexChar x;
x.high = '4' & 0xF;
x.low = '1' & 0xF;

printf("char %c\n", x.byte);
}
