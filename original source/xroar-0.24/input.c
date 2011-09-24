/*  XRoar - a Dragon/Tandy Coco emulator
 *  Copyright (C) 2003-2010  Ciaran Anscomb
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "types.h"
#include "input.h"
#include "joystick.h"
#include "keyboard.h"
#include "machine.h"
#include "mc6821.h"

static int input_joysticks_swapped = 0;
static int pia_firebutton_swap = 0;

void input_control_press(int command, unsigned int arg) {
	switch (command) {
		case INPUT_JOY_RIGHT_X:
		case INPUT_JOY_RIGHT_Y:
		case INPUT_JOY_LEFT_X:
		case INPUT_JOY_LEFT_Y:
			command ^= input_joysticks_swapped;
			joystick_axis[command] = arg;
			break;
		case INPUT_JOY_RIGHT_FIRE:
			PIA0.a.tied_low &= (0xfe ^ pia_firebutton_swap);
			break;
		case INPUT_JOY_LEFT_FIRE:
			PIA0.a.tied_low &= (0xfd ^ pia_firebutton_swap);
			break;
		case INPUT_KEY:
			KEYBOARD_PRESS(arg);
			break;
		case INPUT_UNICODE_KEY:
			keyboard_unicode_press(arg);
			break;
		case INPUT_SWAP_JOYSTICKS:
			input_joysticks_swapped ^= 2;
			pia_firebutton_swap ^= 3;
			break;
	}
}

void input_control_release(int command, unsigned int arg) {
	switch (command) {
		case INPUT_JOY_RIGHT_X:
		case INPUT_JOY_RIGHT_Y:
		case INPUT_JOY_LEFT_X:
		case INPUT_JOY_LEFT_Y:
			command ^= input_joysticks_swapped;
			if (arg < 127 && joystick_axis[command] < 127)
				joystick_axis[command] = 127;
			if (arg > 128 && joystick_axis[command] > 128)
				joystick_axis[command] = 128;
			break;
		case INPUT_JOY_RIGHT_FIRE:
			PIA0.a.tied_low |= (0x01 ^ pia_firebutton_swap);
			break;
		case INPUT_JOY_LEFT_FIRE:
			PIA0.a.tied_low |= (0x02 ^ pia_firebutton_swap);
			break;
		case INPUT_KEY:
			KEYBOARD_RELEASE(arg);
			break;
		case INPUT_UNICODE_KEY:
			keyboard_unicode_release(arg);
			break;
		default:
			break;
	}
}
