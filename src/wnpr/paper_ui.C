/*****************************************************************
 * This file is part of jot-lib (or "jot" for short):
 *   <http://code.google.com/p/jot-lib/>
 * 
 * jot-lib is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * jot-lib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with jot-lib.  If not, see <http://www.gnu.org/licenses/>.`
 *****************************************************************/

#include "disp/view.H"
#include "geom/winsys.H"
#include "gtex/paper_effect.H"
#include "glui/glui.h"

#include "paper_ui.H"

extern str_ptr JOT_ROOT;           // Accessed elsewhere

void
PaperUI::button_press_cb()
{
   PaperEffect::set_paper_tex(_files[_selected]);
}

void
PaperUI::init_listbox()
{
   // We disply just the file name ("paper.png"), but save the location
   // relative to JOT_ROOT ("/nprdata/paper_textures/paper.png") to be passed to
   // PaperEffect
   str_ptr sub_dir("/nprdata/paper_textures/");
   fill_listbox(_listbox, _files, JOT_ROOT + sub_dir, sub_dir,".png");
}
