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
#ifndef PAPER_UI_H_IS_INCLUDED
#define PAPER_UI_H_IS_INCLUDED

#include "geom/file_listbox.H"

/*****************************************************************
 * PaperUI
 *****************************************************************/
class PaperUI : public FileListbox {
   public:
      //******** MANAGERS ********
      PaperUI() :
         FileListbox("Paper Texture Controls", "Paper Texture", "Set Texture")
         { init_listbox(); }
   protected:
       virtual void button_press_cb();
               void init_listbox();   // Fill up listbox
};

#endif // PAPER_UI_H_IS_INCLUDED

/* end of file paper_ui.H */
