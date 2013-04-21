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
#ifndef XF_MEME_IS_INCLUDED
#define XF_MEME_IS_INCLUDED

#include "tess/meme.H"

/*****************************************************************
 * XFMeme:
 *
 *   Meme for tracking a CoordFrame, updating and applying
 *   local coordinates via the xform of the CoordFrame
 *****************************************************************/
class XFMeme : public VertMeme, public CoordFrameObs {
 public:

   //******** MANAGERS ********

   XFMeme(Bbase*      bbase,  // owner Bsurface
          Lvert*      vert,   // vertex to control
          CoordFrame* frame); // skeleton frame
      
   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("XFMeme", XFMeme*, VertMeme, CSimplexData*);

   //******** ACCESSORS ********

   CoordFrame*   frame()        const   { return _frame;}

   //******** VertMeme METHODS ********

   virtual CWpt& compute_update();

   virtual bool move_to(CWpt&);

   // Called when the vertex location changed but this VertMeme
   // didn't cause it:
   virtual void vert_changed_externally();

   //******** CoordFrameObs VIRTUAL METHODS ********

   virtual void notify_frame_deleted(CoordFrame*) { assert(0); } // needs
   virtual void notify_frame_changed(CoordFrame*) {            } //   work

 protected:
   CoordFrame*  _frame;   // coordinate frame defining our position
   Wpt    _local;   // via these here local coordinates
};

#endif // XF_MEME_IS_INCLUDED

// end of file xf_meme.H
