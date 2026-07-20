#ifndef BCF_H_
#define BCF_H_

#include "bcf_types.h"

extern bcf_bat_info bat_info;
extern bcf_bat_param bat_param;

void bcf_init(void);
void bcf_100ms(void);

#endif  // BCF_H_
