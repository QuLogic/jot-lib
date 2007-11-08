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
 * vert_strip.C:
 **********************************************************************/
#include "patch.H"
#include "stripcb.H"
#include "vert_strip.H"

/**********************************************************************
 * VertStrip:
 **********************************************************************/
VertStrip::~VertStrip()
{
   if (_patch)
      _patch->remove(this);
}

void
VertStrip::draw(StripCB* cb)
{
   // draw the strip -- e.g. by making calls to OpenGL or some other
   // graphics API, or by writing a description of the strip to a text
   // file. which of these is done depends on the implementation of
   // the StripCB.

   // stop now if nothing is going on
   if (empty())
      return;

   // iterate over the strip.
   cb->begin_verts(this);

   for (int i=0; i<_verts.num(); i++) {
      cb->vertCB(_verts[i]);
   }

   cb->end_verts(this);
}

/* end of file vert_strip.C */
