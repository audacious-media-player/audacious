/*  Audacious - Cross-platform multimedia player
 *  Copyright (C) 2005-2007  Audacious development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef ACTIONS_EQUALIZER_H
#define ACTIONS_EQUALIZER_H

void action_equ_load_preset(void);
void action_equ_load_auto_preset(void);
void action_equ_load_default_preset(void);
void action_equ_zero_preset(void);
void action_equ_load_preset_file(void);
void action_equ_load_preset_eqf(void);
void action_equ_import_winamp_presets(void);
void action_equ_save_preset(void);
void action_equ_save_auto_preset(void);
void action_equ_save_default_preset(void);
void action_equ_save_preset_file(void);
void action_equ_save_preset_eqf(void);
void action_equ_delete_preset(void);
void action_equ_delete_auto_preset(void);

#endif
