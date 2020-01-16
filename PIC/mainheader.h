/* 
 * File:   
 * Author: Yuqian Zhou
 * Comments: To simplify main and run function
 * Revision history: 
 * v1.1 Feb.25 Stand along header files for main.c
 */

#ifndef mainheader_H
#define	mainheader_H
/********************************* Includes **********************************/
#include <xc.h> 
#include <stdio.h>
#include <stdbool.h>
#include "configBits.h"
#include "I2C.h"
#include "lcd.h"
#include "UI.h"
/********************************** Macros ***********************************/

/************************ Public Function Prototypes *************************/
void emergency_stop(void);
void restartRoutine(void);
#endif	/* mainheader_H */
