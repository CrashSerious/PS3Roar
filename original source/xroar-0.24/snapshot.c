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

#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "fs.h"
#include "keyboard.h"
#include "logging.h"
#include "m6809.h"
#include "machine.h"
#include "mc6821.h"
#include "sam.h"
#include "snapshot.h"
#include "tape.h"
#include "vdg.h"
#include "vdisk.h"
#include "vdrive.h"
#include "xroar.h"

/* Write files in 'chunks', each with an identifying byte and a 16-bit
 * length.  This should mean no changes are required that break the
 * format.  */

/* Note: Setting up the correct ROM select for Dragon 64 depends on SAM
 * register update following PIA configuration. */

#define ID_REGISTER_DUMP (0)	/* deprecated */
#define ID_RAM_PAGE0     (1)
#define ID_PIA_REGISTERS (2)
#define ID_SAM_REGISTERS (3)
#define ID_M6809_STATE   (4)
#define ID_KEYBOARD_MAP  (5)
#define ID_ARCHITECTURE  (6)
#define ID_RAM_PAGE1     (7)
#define ID_MACHINECONFIG (8)
#define ID_SNAPVERSION   (9)
#define ID_VDISK_FILE    (10)

#define SNAPSHOT_VERSION_MAJOR 1
#define SNAPSHOT_VERSION_MINOR 6

int write_snapshot(const char *filename) {
	int fd;
	M6809State cpu_state;
	if ((fd = fs_open(filename, FS_WRITE)) == -1)
		return -1;
	fs_write(fd, "XRoar snapshot.\012\000", 17);
	/* Snapshot version */
	fs_write_uint8(fd, ID_SNAPVERSION); fs_write_uint16(fd, 3);
	fs_write_uint8(fd, SNAPSHOT_VERSION_MAJOR);
	fs_write_uint16(fd, SNAPSHOT_VERSION_MINOR);
	/* Machine running config */
	fs_write_uint8(fd, ID_MACHINECONFIG); fs_write_uint16(fd, 8);
	fs_write_uint8(fd, running_machine);
	fs_write_uint8(fd, running_config.architecture);
	fs_write_uint8(fd, running_config.romset);
	fs_write_uint8(fd, running_config.keymap);
	fs_write_uint8(fd, running_config.tv_standard);
	fs_write_uint8(fd, running_config.ram);
	fs_write_uint8(fd, running_config.dos_type);
	fs_write_uint8(fd, running_config.cross_colour_phase);
	/* RAM page 0 */
	fs_write_uint8(fd, ID_RAM_PAGE0);
	fs_write_uint16(fd, machine_ram_size > 0x8000 ? 0x8000 : machine_ram_size);
	fs_write(fd, ram0, machine_ram_size > 0x8000 ? 0x8000 : machine_ram_size);
	/* RAM page 1 */
	if (machine_ram_size > 0x8000) {
		fs_write_uint8(fd, ID_RAM_PAGE1);
		fs_write_uint16(fd, machine_ram_size - 0x8000);
		fs_write(fd, ram0 + 0x8000, machine_ram_size - 0x8000);
	}
	/* PIA state written before CPU state because PIA may have
	 * unacknowledged interrupts pending already cleared in the CPU
	 * state */
	fs_write_uint8(fd, ID_PIA_REGISTERS); fs_write_uint16(fd, 3*4);
	/* PIA0.a */
	fs_write_uint8(fd, PIA0.a.direction_register);
	fs_write_uint8(fd, PIA0.a.output_register);
	fs_write_uint8(fd, PIA0.a.control_register);
	/* PIA0.b */
	fs_write_uint8(fd, PIA0.b.direction_register);
	fs_write_uint8(fd, PIA0.b.output_register);
	fs_write_uint8(fd, PIA0.b.control_register);
	/* PIA1.a */
	fs_write_uint8(fd, PIA1.a.direction_register);
	fs_write_uint8(fd, PIA1.a.output_register);
	fs_write_uint8(fd, PIA1.a.control_register);
	/* PIA1.b */
	fs_write_uint8(fd, PIA1.b.direction_register);
	fs_write_uint8(fd, PIA1.b.output_register);
	fs_write_uint8(fd, PIA1.b.control_register);
	/* M6809 state */
	fs_write_uint8(fd, ID_M6809_STATE); fs_write_uint16(fd, 20);
	m6809_get_state(&cpu_state);
	fs_write_uint8(fd, cpu_state.reg_cc);
	fs_write_uint8(fd, cpu_state.reg_a);
	fs_write_uint8(fd, cpu_state.reg_b);
	fs_write_uint8(fd, cpu_state.reg_dp);
	fs_write_uint16(fd, cpu_state.reg_x);
	fs_write_uint16(fd, cpu_state.reg_y);
	fs_write_uint16(fd, cpu_state.reg_u);
	fs_write_uint16(fd, cpu_state.reg_s);
	fs_write_uint16(fd, cpu_state.reg_pc);
	fs_write_uint8(fd, cpu_state.halt);
	fs_write_uint8(fd, cpu_state.nmi);
	fs_write_uint8(fd, cpu_state.firq);
	fs_write_uint8(fd, cpu_state.irq);
	fs_write_uint8(fd, cpu_state.cpu_state);
	fs_write_uint8(fd, cpu_state.nmi_armed);
	/* SAM */
	fs_write_uint8(fd, ID_SAM_REGISTERS); fs_write_uint16(fd, 2);
	fs_write_uint16(fd, sam_get_register());
	/* Attached virtual disk filenames */
	{
		struct vdisk *disk;
		int drive;
		for (drive = 0; drive < VDRIVE_MAX_DRIVES; drive++) {
			disk = vdrive_disk_in_drive(drive);
			if (disk != NULL && disk->filename != NULL) {
				int length = strlen(disk->filename) + 1;
				fs_write_uint8(fd, ID_VDISK_FILE);
				fs_write_uint16(fd, 1 + length);
				fs_write_uint8(fd, drive);
				fs_write(fd, disk->filename, length);
			}
		}
	}
	/* Finish up */
	fs_close(fd);
	return 0;
}

static int old_arch_mapping[4] = {
	MACHINE_DRAGON32,
	MACHINE_DRAGON64,
	MACHINE_TANO,
	MACHINE_COCOUS
};

static void old_set_registers(uint8_t *regs) {
	M6809State state;
	state.reg_cc = regs[0];
	state.reg_a = regs[1];
	state.reg_b = regs[2];
	state.reg_dp = regs[3];
	state.reg_x = regs[4] << 8 | regs[5];
	state.reg_y = regs[6] << 8 | regs[7];
	state.reg_u = regs[8] << 8 | regs[9];
	state.reg_s = regs[10] << 8 | regs[11];
	state.reg_pc = regs[12] << 8 | regs[13];
	state.halt = 0;
	state.nmi = 0;
	state.firq = 0;
	state.irq = 0;
	state.cpu_state = M6809_COMPAT_STATE_NORMAL;
	state.nmi_armed = 0;
	m6809_set_state(&state);
}

int read_snapshot(const char *filename) {
	int fd;
	uint8_t buffer[17];
	int section, size, tmp;
	M6809State cpu_state;
	if (filename == NULL)
		return -1;
	if ((fd = fs_open(filename, FS_READ)) == -1)
		return -1;
	fs_read(fd, buffer, 17);
	if (strncmp((char *)buffer, "XRoar snapshot.\012\000", 17)) {
		/* Very old-style snapshot.  Register dump always came first.
		 * Also, it used to be written out as only taking 12 bytes. */
		if (buffer[0] != ID_REGISTER_DUMP || buffer[1] != 0
				|| (buffer[2] != 12 && buffer[2] != 14)) {
			LOG_WARN("Snapshot format not recognised.\n");
			fs_close(fd);
			return -1;
		}
	}
	/* Default to Dragon 64 for old snapshots */
	requested_machine = MACHINE_DRAGON64;
	machine_clear_requested_config();
	requested_config.dos_type = DOS_NONE;
	/* Need reset in case old snapshot doesn't trigger one */
	machine_reset(RESET_HARD);
	/* If old snapshot, buffer contains register dump */
	if (buffer[0] != 'X') {
		old_set_registers(buffer + 3);
	}
	while ((section = fs_read_uint8(fd)) >= 0) {
		size = fs_read_uint16(fd);
		if (size == 0) size = 0x10000;
		switch (section) {
			case ID_ARCHITECTURE:
				/* Deprecated: Machine architecture */
				if (size < 1) break;
				tmp = fs_read_uint8(fd);
				tmp %= 4;
				requested_machine = old_arch_mapping[tmp];
				machine_reset(RESET_HARD);
				size--;
				break;
			case ID_KEYBOARD_MAP:
				/* Deprecated: Keyboard map */
				if (size < 1) break;
				tmp = fs_read_uint8(fd);
				requested_config.keymap = tmp;
				keyboard_set_keymap(tmp);
				size--;
				break;
			case ID_REGISTER_DUMP:
				/* Deprecated */
				if (size < 14) break;
				size -= fs_read(fd, buffer, 14);
				old_set_registers(buffer);
				break;
			case ID_M6809_STATE:
				/* M6809 state */
				if (size < 20) break;
				cpu_state.reg_cc = fs_read_uint8(fd);
				cpu_state.reg_a = fs_read_uint8(fd);
				cpu_state.reg_b = fs_read_uint8(fd);
				cpu_state.reg_dp = fs_read_uint8(fd);
				cpu_state.reg_x = fs_read_uint16(fd);
				cpu_state.reg_y = fs_read_uint16(fd);
				cpu_state.reg_u = fs_read_uint16(fd);
				cpu_state.reg_s = fs_read_uint16(fd);
				cpu_state.reg_pc = fs_read_uint16(fd);
				cpu_state.halt = fs_read_uint8(fd);
				cpu_state.nmi = fs_read_uint8(fd);
				cpu_state.firq = fs_read_uint8(fd);
				cpu_state.irq = fs_read_uint8(fd);
				if (size == 21) {
					/* Old style */
					int wait_for_interrupt;
					int skip_register_push;
					wait_for_interrupt = fs_read_uint8(fd);
					skip_register_push = fs_read_uint8(fd);
					if (wait_for_interrupt && skip_register_push) {
						cpu_state.cpu_state = M6809_COMPAT_STATE_CWAI;
					} else if (wait_for_interrupt) {
						cpu_state.cpu_state = M6809_COMPAT_STATE_SYNC;
					} else {
						cpu_state.cpu_state = M6809_COMPAT_STATE_NORMAL;
					}
					size--;
				} else {
					cpu_state.cpu_state = fs_read_uint8(fd);
				}
				cpu_state.nmi_armed = fs_read_uint8(fd);
				cpu_state.firq &= 3;
				cpu_state.irq &= 3;
				m6809_set_state(&cpu_state);
				size -= 20;
				if (size > 0) {
					/* Skip 'halted' */
					tmp = fs_read_uint8(fd);
					size--;
				}
				break;
			case ID_MACHINECONFIG:
				/* Machine running config */
				if (size < 7) break;
				requested_machine = fs_read_uint8(fd);
				requested_config.architecture = fs_read_uint8(fd);
				requested_config.romset = fs_read_uint8(fd);
				requested_config.keymap = fs_read_uint8(fd);
				requested_config.tv_standard = fs_read_uint8(fd);
				requested_config.ram = fs_read_uint8(fd);
				requested_config.dos_type = fs_read_uint8(fd);
				size -= 7;
				if (size > 0) {
					requested_config.cross_colour_phase = fs_read_uint8(fd);
					size--;
				}
				machine_reset(RESET_HARD);
				break;
			case ID_PIA_REGISTERS:
				/* PIA0.a */
				if (size < 3) break;
				PIA0.a.direction_register = fs_read_uint8(fd);
				PIA0.a.output_register = fs_read_uint8(fd);
				PIA0.a.control_register = fs_read_uint8(fd);
				size -= 3;
				/* PIA0.b */
				if (size < 3) break;
				PIA0.b.direction_register = fs_read_uint8(fd);
				PIA0.b.output_register = fs_read_uint8(fd);
				PIA0.b.control_register = fs_read_uint8(fd);
				size -= 3;
				mc6821_update_state(&PIA0);
				/* PIA1.a */
				if (size < 3) break;
				PIA1.a.direction_register = fs_read_uint8(fd);
				PIA1.a.output_register = fs_read_uint8(fd);
				PIA1.a.control_register = fs_read_uint8(fd);
				size -= 3;
				/* PIA1.b */
				if (size < 3) break;
				PIA1.b.direction_register = fs_read_uint8(fd);
				PIA1.b.output_register = fs_read_uint8(fd);
				PIA1.b.control_register = fs_read_uint8(fd);
				size -= 3;
				mc6821_update_state(&PIA1);
				break;
			case ID_RAM_PAGE0:
				if (size <= (int)sizeof(ram0)) {
					size -= fs_read(fd, ram0, size);
				} else {
					size -= fs_read(fd, ram0, sizeof(ram0));
				}
				break;
			case ID_RAM_PAGE1:
				if (size <= (int)(sizeof(ram0) - 0x8000)) {
					size -= fs_read(fd, ram0 + 0x8000, size);
				} else {
					size -= fs_read(fd, ram0 + 0x8000, sizeof(ram0) - 0x8000);
				}
				break;
			case ID_SAM_REGISTERS:
				/* SAM */
				if (size < 2) break;
				tmp = fs_read_uint16(fd);
				size -= 2;
				sam_set_register(tmp);
				break;
			case ID_SNAPVERSION:
				{
				/* Snapshot version - abort if snapshot
				 * contains stuff we don't understand */
				int major, minor;
				if (size < 3) break;
				major = fs_read_uint8(fd);
				minor = fs_read_uint16(fd);
				size -= 3;
				if (major != SNAPSHOT_VERSION_MAJOR || minor > SNAPSHOT_VERSION_MINOR) {
					LOG_WARN("Snapshot version %d.%d not supported.\n", major, minor);
					fs_close(fd);
					return -1;
				}
				}
				break;
			case ID_VDISK_FILE:
				/* Attached virtual disk filenames */
				{
					int drive;
					size--;
					drive = fs_read_uint8(fd);
					vdrive_eject_disk(drive);
					if (size > 0) {
						char *name = malloc(size);
						if (name != NULL) {
							size -= fs_read(fd, name, size);
							vdrive_insert_disk(drive, vdisk_load(name));
						}
					}
				}
				break;
			default:
				/* Unknown chunk */
				LOG_WARN("Unknown chunk in snaphot.\n");
				break;
		}
		if (size > 0) {
			LOG_WARN("Skipping extra bytes in snapshot chunk id=%d.\n", (int)section);
			for (; size; size--)
				tmp = fs_read_uint8(fd);
		}
	}
	fs_close(fd);
	return 0;
}
