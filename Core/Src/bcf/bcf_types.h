#ifndef BCF_TYPES_H_
#define BCF_TYPES_H_

#include "rte.h"

#define NUM_OF_CELLS_SER (AFEDRV_CELL_CNT)
#define NUM_OF_CELLS_PAR (1U)

typedef struct {
    float cell_V[NUM_OF_CELLS_SER * NUM_OF_CELLS_PAR];
    float current_A;
    float stack_V;
    float bat_temp_degC;
    float soc;
    float est_cap_Ah;
    float consumed_Ah;
} bcf_bat_info;

typedef struct  {
    float nominal_cap_Ah;
    float min_cell_volt_V;
} bcf_bat_param;


#endif    // BCF_TYPES_H_
