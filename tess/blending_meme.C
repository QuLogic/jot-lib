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
 * blending_meme.C:
 **********************************************************************/
#include "blending_meme.H"

/************************************************************
 * BlendingMeme
 ************************************************************/
bool
BlendingMeme::compute_delt()
{
   if (is_pinned())
      return false;
   compute_update();
   return true;
}

bool
BlendingMeme::apply_delt()
{
   // apply the update
   return apply_update();
}

CWpt& 
BlendingMeme::compute_update()
{
   // Take the average of the current loc and the centroid of
   // neighboring vertices.

   // XXX - open question what to use for "centroid"

   // Don't shrink when on the border of the mesh:
   //if (!vert() || vert()->is_border())
   //   return VertMeme::compute_update();

   _update = loc() + target_delt();

   return _update;
}

bool
BlendingMeme::is_boss_like()
{
   // XXX - trying this out

   return is_boss() || Bbase::find_boss_meme(vert());
}

VertMeme* 
BlendingMeme::_gen_child(Lvert* lv) const 
{
   Bsurface* c = surface()->child_surface();
   assert(c && !c->find_vert_meme(lv));

   // Produce a child just like this on the given vertex of
   // the subdiv mesh.
   return new BlendingMeme(c, lv);
}

/* end of file blending_meme.C */
