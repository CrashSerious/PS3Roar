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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>
#include <SDL/SDL_keycode.h>
#include <SDL/SDL_compat.h>
#include <sys/process.h>

#include "types.h"
#include "logging.h"
#include "cart.h"
#include "events.h"
#include "hexs19.h"
#include "input.h"
#include "keyboard.h"
#include "machine.h"
#include "mc6821.h"
#include "module.h"
#include "snapshot.h"
#include "tape.h"
#include "vdg.h"
#include "vdisk.h"
#include "vdrive.h"
#include "xroar.h"
#include "ps3gui.h"
#include "paths.h"

static int init(void);
static void shutdown(void);

//extern int DOS_ENABLED;

KeyboardModule keyboard_sdl_module = {
	{ "sdl", "SDL keyboard driver",
	  init, 0, shutdown }
};

static event_t *poll_event;
static void do_poll(void);

extern TTF_Font *text_fontSmall; 
extern TTF_Font *text_fontMedium; 
extern TTF_Font *text_fontLarge; 
extern TTF_Font *text_fontTiny; 

struct keymap {
	const char *name;
	unsigned int *raw;
};

#include "keyboard_sdl_mappings.h"

static unsigned int control = 0, shift = 0;
static unsigned int emulate_joystick = 0;
static int old_frameskip = 0;

static uint_least16_t sdl_to_keymap[768];

/* Track which unicode value was last generated by key presses (SDL only
 * guarantees to fill in the unicode field on key presses, not releases). */
static unsigned int unicode_last_keysym[SDLK_LAST];

static const char *keymap_option;
static unsigned int *selected_keymap;
static int translated_keymap;

static const char *disk_exts[] = { "DMK", "JVC", "VDK", "DSK", NULL };
static const char *tape_exts[] = { "CAS", NULL };
static const char *snap_exts[] = { "SNA", NULL };
static const char *cart_exts[] = { "ROM", NULL };

static void map_keyboard(unsigned int *map) {
	int i;
	for (i = 0; i < 256; i++)
		sdl_to_keymap[i] = i & 0x7f;
	for (i = 0; i < SDLK_LAST; i++)
		unicode_last_keysym[i] = 0;
	if (map == NULL)
		return;
	while (*map) {
		unsigned int sdlkey = *(map++);
		unsigned int dgnkey = *(map++);
		sdl_to_keymap[sdlkey & 0xff] = dgnkey & 0x7f;
	}
}

static int init(void) {
	int i;
	LOG_DEBUG(3,"\tAttempting to init Keyboard\n");
	keymap_option = xroar_opt_keymap;
	selected_keymap = NULL;
	for (i = 0; mappings[i].name; i++) {
		if (selected_keymap == NULL
				&& !strcmp("uk", mappings[i].name)) {
			selected_keymap = mappings[i].raw;
		}
		if (keymap_option && !strcmp(keymap_option, mappings[i].name)) {
			selected_keymap = mappings[i].raw;
			LOG_DEBUG(2,"\tSelecting '%s' keymap\n",keymap_option);
		}
	}
	map_keyboard(selected_keymap);
	translated_keymap = 0;
	SDL_EnableUNICODE(translated_keymap);
	poll_event = event_new();
	poll_event->dispatch = do_poll;
	poll_event->at_cycle = current_cycle + (OSCILLATOR_RATE / 100);
	event_queue(&UI_EVENT_LIST, poll_event);
	return 0;
}

static void shutdown(void) {
	event_free(poll_event);
}

static void emulator_command(SDLKey sym) {
	char *LOADfname;
	LOADfname = malloc(256);
	memset(LOADfname, 0x00, 256);

	char *SAVEfname;
	SAVEfname = malloc(256);
	memset(SAVEfname, 0x00, 256);

	/* Setup ROM Variable */
	char *ROM;
	ROM = malloc(256);
	memset(ROM, 0x00, 256);
	strcpy(ROM, ROMSdir);

	/* Setup CASSETTE Variable */
	char *CAS;
	CAS = malloc(256);
	memset(CAS, 0x00, 256);
	strcpy(CAS, CASdir);

	/* Setup DISK Variable */
	char *DSK;
	DSK = malloc(256);
	memset(DSK, 0x00, 256);
	strcpy(DSK, DSKdir);

	/* Setup DISK Variable */
	char *SNAP;
	SNAP = malloc(256);
	memset(SNAP, 0x00, 256);
	strcpy(SNAP, SNAPdir);
	
	char *temp12;
	temp12 = malloc(256);
	memset(temp12, 0x00, 256);
	strcpy(temp12, "");
	int tempint;

	switch (sym) {
	case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4:
		if (shift) {
			int drive = sym - SDLK_1;
			char *filename = malloc(256);
			memset(filename, 0x00, 256);
			CreateFile (DSK, filename, ".dsk");
			control = 0;

			if (filename == NULL)
				break;
			int filetype = xroar_filetype_by_ext(filename);
			vdrive_eject_disk(drive);
			/* Default to 40T 1H.  XXX: need interface to select */
			struct vdisk *new_disk = vdisk_blank_disk(1, 40, VDISK_LENGTH_5_25);
			if (new_disk == NULL)
				break;
			LOG_DEBUG(4, "Creating blank disk in drive %d\n (file: %s.\n", 1 + drive, filename);
			switch (filetype) {
				case FILETYPE_VDK:
				case FILETYPE_JVC:
				case FILETYPE_DMK:
					break;
				default:
					filetype = FILETYPE_DMK;
					break;
			}
			new_disk->filetype = filetype;
			new_disk->filename = strdup(filename);
			new_disk->file_write_protect = VDISK_WRITE_ENABLE;
			vdrive_insert_disk(drive, new_disk);
		} else {

			char *filename = malloc(256);
			memset(filename, 0x00, 256);
			filename=LoadRom(DSK, filename);// = filereq_module->save_filename(disk_exts);
			if (filename) {
				vdrive_eject_disk(sym - SDLK_1);
				vdrive_insert_disk(sym - SDLK_1, vdisk_load(filename));
			}
		}
		break;
	case SDLK_5: case SDLK_6: case SDLK_7: case SDLK_8:
		if (shift) {
			int drive = sym - SDLK_5;
			struct vdisk *disk = vdrive_disk_in_drive(drive);
			if (disk != NULL) {
				if (disk->file_write_protect == VDISK_WRITE_ENABLE) {
					disk->file_write_protect = VDISK_WRITE_PROTECT;
					LOG_DEBUG(2, "File for disk in drive %d write protected.\n", drive);
				} else {
					disk->file_write_protect = VDISK_WRITE_ENABLE;
					LOG_DEBUG(2, "File for disk in drive %d write enabled.\n", drive);
				}
			}
		} else {
			int drive = sym - SDLK_5;
			struct vdisk *disk = vdrive_disk_in_drive(drive);
			if (disk != NULL) {
				if (disk->write_protect == VDISK_WRITE_ENABLE) {
					disk->write_protect = VDISK_WRITE_PROTECT;
					LOG_DEBUG(2, "Disk in drive %d write protected.\n", drive);
				} else {
					disk->write_protect = VDISK_WRITE_ENABLE;
					LOG_DEBUG(2, "Disk in drive %d write enabled.\n", drive);
				}
			}
		}
		break;
	case SDLK_a:
		running_config.cross_colour_phase++;
		if (running_config.cross_colour_phase > 2)
			running_config.cross_colour_phase = 0;
		vdg_set_mode();
		break;
	case SDLK_c:
		exit(0);
		break;
	case SDLK_d: /* Enable/disableDOS */
		toggleDOS();
		break;
	
	case SDLK_e:
		requested_config.dos_type = DOS_ENABLED ? DOS_NONE : ANY_AUTO;
		break;
	case SDLK_f:
		if (video_module->set_fullscreen)
			video_module->set_fullscreen(!video_module->is_fullscreen);
		break;
	case SDLK_i:
		{
			/* insert a cartridge */
			ROM = LoadRom(ROM, LOADfname);
			xroar_load_file(ROM, !shift);
		}
		break;
	case SDLK_j:
		if (shift) {
			input_control_press(INPUT_SWAP_JOYSTICKS, 0);
		} else {
			if (emulate_joystick) {
				input_control_press(INPUT_SWAP_JOYSTICKS, 0);
			}
			emulate_joystick = (emulate_joystick + 1) % 3;
		}
		break;
	case SDLK_k:
		keyboard_set_keymap(running_config.keymap + 1);
		break;
	case SDLK_b:
	case SDLK_h:

	case SDLK_l:
		{
			/* insert a cassette */
			CAS = LoadRom(CAS, LOADfname);
			xroar_load_file(CAS, shift);
		}
		break;
	case SDLK_m:
		machine_clear_requested_config();
		requested_machine = running_machine + 1;
		machine_reset(RESET_HARD);
		break;
	case SDLK_p:
		//egg code
		displayStatic(1);
		displayStatic(0);
		break;

	case SDLK_r:
		machine_reset(shift ? RESET_HARD : RESET_SOFT);
		break;
	case SDLK_s:
		{
			if (shift)
			{
				/* Load snapshot */
				CAS = LoadRom(SNAP, LOADfname);
				xroar_load_file(SNAP, shift);
			}
			else
			{
				/* Save Snapshot */
				CreateFile(SNAP, SAVEfname, ".sna");//filereq_module->save_filename(snap_exts);
				if (SAVEfname)
				write_snapshot(SAVEfname);
				control = 0;
			}
		}
		break;
	case SDLK_t:
		{
			//test key
		}
		break;
	case SDLK_u:
		// Create/Check USB Drive for ROMS, BIOS, cassettes, disks, etc
		{
			tempint = CheckUSB();
		}
		break;
	case SDLK_w:
		{
		CreateFile(CAS, SAVEfname, ".cas");//filereq_module->save_filename(snap_exts);
		control = 0;

		if (SAVEfname) {
			tape_open_writing(SAVEfname);
		}
		break;
		}
#ifdef TRACE
	case SDLK_v:
		xroar_trace_enabled = !xroar_trace_enabled;
		break;
#endif
	case SDLK_z: // running out of letters...
		translated_keymap = !translated_keymap;
		/* UNICODE translation only used in
		 * translation mode */
		SDL_EnableUNICODE(translated_keymap);
		break;
	default:
		break;
	}
	return;
}

static void keypress(SDL_keysym *keysym) {
	SDLKey sym = keysym->sym;
	if (emulate_joystick) {
		if (sym == SDLK_UP) { input_control_press(INPUT_JOY_RIGHT_Y, 0); return; }
		if (sym == SDLK_DOWN) { input_control_press(INPUT_JOY_RIGHT_Y, 255); return; }
		if (sym == SDLK_LEFT) { input_control_press(INPUT_JOY_RIGHT_X, 0); return; }
		if (sym == SDLK_RIGHT) { input_control_press(INPUT_JOY_RIGHT_X, 255); return; }
		if (sym == SDLK_LALT) { input_control_press(INPUT_JOY_RIGHT_FIRE, 0); return; }
	}
	if (sym == SDLK_LSHIFT || sym == SDLK_RSHIFT) {
		shift = 1;
		KEYBOARD_PRESS(0);
		return;
	}
	if (sym == SDLK_LCTRL || sym == SDLK_RCTRL) { control = 1; return; }
	if (sym == SDLK_F12) {
		xroar_noratelimit = 1;
		old_frameskip = xroar_frameskip;
		xroar_frameskip = 10;
	}
	if (control) {
		emulator_command(sym);
		return;
	}
	if (sym == SDLK_UP) { KEYBOARD_PRESS(94); return; }
	if (sym == SDLK_DOWN) { KEYBOARD_PRESS(10); return; }
	if (sym == SDLK_LEFT) { KEYBOARD_PRESS(8); return; }
	if (sym == SDLK_RIGHT) { KEYBOARD_PRESS(9); return; }
	if (sym == SDLK_HOME) { KEYBOARD_PRESS(12); return; }
	if (translated_keymap) {
		unsigned int unicode;
		if (sym >= SDLK_LAST)
			return;
		unicode = keysym->unicode;
		/* Map shift + backspace/delete to ^U */
		if (shift && (unicode == 8 || unicode == 127)) {
			unicode = 0x15;
		}
		unicode_last_keysym[sym] = unicode;
		keyboard_unicode_press(unicode);
		return;
	}
	if (sym < 256) {
		unsigned int mapped = sdl_to_keymap[sym];
		KEYBOARD_PRESS(mapped);
	}

}

#define JOY_UNLOW(j) if (j < 127) j = 127;
#define JOY_UNHIGH(j) if (j > 128) j = 128;

static void keyrelease(SDL_keysym *keysym) {
	SDLKey sym = keysym->sym;
	if (emulate_joystick) {
		if (sym == SDLK_UP) { input_control_release(INPUT_JOY_RIGHT_Y, 0); return; }
		if (sym == SDLK_DOWN) { input_control_release(INPUT_JOY_RIGHT_Y, 255); return; }
		if (sym == SDLK_LEFT) { input_control_release(INPUT_JOY_RIGHT_X, 0); return; }
		if (sym == SDLK_RIGHT) { input_control_release(INPUT_JOY_RIGHT_X, 255); return; }
		if (sym == SDLK_LALT) { input_control_release(INPUT_JOY_RIGHT_FIRE, 0); return; }
	}
	if (sym == SDLK_LSHIFT || sym == SDLK_RSHIFT) {
		shift = 0;
		KEYBOARD_RELEASE(0);
		return;
	}
	if (sym == SDLK_LCTRL || sym == SDLK_RCTRL) { control = 0; return; }
	if (sym == SDLK_F12) {
		xroar_noratelimit = 0;
		xroar_frameskip = old_frameskip;
	}
	if (sym == SDLK_UP) { KEYBOARD_RELEASE(94); return; }
	if (sym == SDLK_DOWN) { KEYBOARD_RELEASE(10); return; }
	if (sym == SDLK_LEFT) { KEYBOARD_RELEASE(8); return; }
	if (sym == SDLK_RIGHT) { KEYBOARD_RELEASE(9); return; }
	if (sym == SDLK_HOME) { KEYBOARD_RELEASE(12); return; }
	if (translated_keymap) {
		unsigned int unicode;
		if (sym >= SDLK_LAST)
			return;
		unicode = unicode_last_keysym[sym];
		/* Map shift + backspace/delete to ^U */
		if (shift && (unicode == 8 || unicode == 127)) {
			unicode = 0x15;
		}
		keyboard_unicode_release(unicode);
		/* Might have unpressed shift prematurely */
		if (shift)
			KEYBOARD_PRESS(0);
		return;
	}
	if (sym < 256) {
		unsigned int mapped = sdl_to_keymap[sym];
		KEYBOARD_RELEASE(mapped);
	}
}

static void do_poll(void) {
	/* poll the keyboard */
	SDL_Event event;
	while (SDL_PollEvent(&event) == 1) {
		switch(event.type) {
			case SDL_VIDEORESIZE:
				if (video_module->resize) {
					video_module->resize(event.resize.w, event.resize.h);
				}
				break;
			case SDL_QUIT:
				exit(0); break;
			case SDL_KEYDOWN:
				keypress(&event.key.keysym);
				keyboard_column_update();
				keyboard_row_update();
				break;
			case SDL_KEYUP:
				keyrelease(&event.key.keysym);
				keyboard_column_update();
				keyboard_row_update();
				break;
			default:
				break;
		}
	}
	poll_event->at_cycle += OSCILLATOR_RATE / 100;
	event_queue(&UI_EVENT_LIST, poll_event);
}