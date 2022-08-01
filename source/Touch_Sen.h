/******************************************************************/
//Touch_Sen.h	Rev 1.0	5/11/2016																		//
//Author: Ben Roloff																							//
//Purpose: Allow for reading of Touch Sensor											//
/******************************************************************/

#ifndef __TOUCH_SEN_H_
#define __TOUCH_SEN_H_

#include "MKL46Z4.h"

// Touch Sensor function prototypes
void Touch_Init(void);
int Touch_Scan_LH(void);
int Touch_Scan_HL(void);

// Macros
#define SCAN_OFFSET 544  // Offset for scan range
#define SCAN_DATA TSI0->DATA & 0xFFFF // Accessing the bits held in TSI0_DATA_TSICNT

#endif
