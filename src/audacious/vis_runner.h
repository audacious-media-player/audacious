// vis_runner.c
// Copyright 2009 John Lindgren

// This file is part of Audacious.

// Audacious is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, version 3 of the License.

// Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along with
// Audacious. If not, see <http://www.gnu.org/licenses/>.

// The Audacious team does not consider modular code linking to Audacious or
// using our public API to be a derived work.

void vis_runner_init (void);
void vis_runner_pass_audio (int time, float * data, int samples, int channels);
void vis_runner_add_hook (HookFunction func, void * user_data);
void vis_runner_remove_hook (HookFunction func);
