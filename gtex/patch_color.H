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
/**********************************************************************
 * patch_color.H:
 **********************************************************************/
#ifndef PATCH_COLOR_H_IS_INCLUDED
#define PATCH_COLOR_H_IS_INCLUDED

#include "hidden_line.H"

/**********************************************************************
 * PatchColorTexture:
 *
 *   Draws each Patch in a separate color, in "hidden line" style.
 **********************************************************************/
class PatchColorTexture : public HiddenLineTexture {
 public:
   PatchColorTexture(Patch* patch = 0) : HiddenLineTexture(patch) {
      set_color(COLOR::any());
   }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("Show patches",
                        PatchColorTexture*,
                        HiddenLineTexture,
                        CDATA_ITEM*);

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new PatchColorTexture; }
};

#endif // PATCH_COLOR_H_IS_INCLUDED

// end of file patch_color.H
