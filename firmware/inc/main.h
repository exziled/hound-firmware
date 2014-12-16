/*!
 * @file main.cpp
 * 
 * @brief HOUND Node Main
 * 
 * @author Benjamin Carlson
 * @author Blake Bourque
 * 
 * @date November 20, 2014
 * 
 * 
 * 
 */


#ifndef __HOUND_MAIN_H
#define __HOUND_MAIN_H

extern "C" {

#ifdef __cplusplus
extern "C" {
#endif
#include "hw_config.h"
#ifdef __cplusplus
}
#endif


void Timing_Decrement(void);

system_tick_t millis(void);

}

#endif // __HOUND_MAIN_H
