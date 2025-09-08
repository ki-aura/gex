#ifndef GEX_HELPER_FUNCS_H
#define GEX_HELPER_FUNCS_H

unsigned long  popup_question(const char *qline1, const char *qline2, popup_types pt);
char byte_to_ascii(unsigned char b);

// t[3] = nibs_to_byte('4', '6');
unsigned char nibs_to_byte(const char hi, const char lo);

// apply_hinib_to_byte(t+2, '4');
void apply_hinib_to_byte(unsigned char *byte, const char hi);
void apply_lonib_to_byte(unsigned char *byte, const char lo);

//byte_to_nibs(t[8], &nibhi, &niblo);
void byte_to_nibs(const unsigned char byte, char *hi, char *lo);


#endif 
