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
/*****************************************************************
 * blending_meme.H
 *****************************************************************/
#ifndef BLENDING_MEME_H_IS_INCLUDED
#define BLENDING_MEME_H_IS_INCLUDED

#include "bsurface.H"

/*****************************************************************
 * BlendingMeme:
 *
 *   Vertex meme that moves toward centroid of neighboring
 *   vertices.
 *****************************************************************/
class BlendingMeme : public VertMeme {
 public:
   //******** MANAGERS ********
   BlendingMeme(Bsurface* b, Lvert* v) : VertMeme(b,v) {}
      
   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("BlendingMeme", BlendingMeme*, VertMeme, CSimplexData*);

   //******** ACCESSORS ********
   Bsurface* surface() const { return (Bsurface*)bbase(); }

   //******** VertMeme VIRTUAL METHODS ********

   // Compute where to move to a more "relaxed" position:
   virtual bool compute_delt();

   // Move to the previously computed relaxed position:
   virtual bool apply_delt();

   virtual CWpt&  compute_update();

   virtual bool is_boss_like();

   // Methods for generating vert memes in the child Bbase.
   // (See meme.H for more info):
   virtual VertMeme* _gen_child(Lvert*) const;
   virtual VertMeme* _gen_child(Lvert* lv, VertMeme*) const {
      // we don't need no steenkin averaging
      return _gen_child(lv);
   }
   virtual VertMeme*
   _gen_child(Lvert* lv , VertMeme*, VertMeme*, VertMeme*) const {
      return _gen_child(lv);
   }
};

#endif // BLENDING_MEME_H_IS_INCLUDED

/* end of file blending_meme.H */
