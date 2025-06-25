/*
 * Touch_Driver.h
 *
 *  Created on: Sep 22, 2021
 *      Author: MURAT OKUMUS
 */

#ifndef MODULE_KEYPAD_H
#define MODULE_KEYPAD_H
#include "globals.h"

#define KEYCODE_0 				0
#define KEYCODE_1 				1
#define KEYCODE_2 				2
#define KEYCODE_3 				3
#define KEYCODE_4 				4
#define KEYCODE_5 				5
#define KEYCODE_6 				6
#define KEYCODE_7 				7
#define KEYCODE_8 				8
#define KEYCODE_9 				9
#define KEYCODE_HOME                            10
#define KEYCODE_SETTINGS                        11


void keyp_initialize(void);
void keyp_update(void);
int keyp_is_multiple_key_down(void);
void keyp_set_settings(void);
void keyp_get_settings(void);
int keyp_is_home_or_setting_down(void);


extern void keyp_on_key_down(uint16_t keys,uint8_t keyCount);
extern void keyp_on_settings_down(void);
extern void keyp_on_lock_down(void);

#endif /* INC_MODULE_KEYPAD_H_ */
