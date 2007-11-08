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
#include "tess/disk_map.H"      // debugging
#include "tess/primitive.H"
#include "tess/xf_meme.H"


static bool debug = Config::get_var_bool("DEBUG_PRIMITIVE",false);

/************************************************************
 * XFMeme
 ************************************************************/
XFMeme::XFMeme(
   Bbase*      bbase,
   Lvert*      vert,  // vertex
   CoordFrame* frame  // skeleton frame
   ) :
   VertMeme(bbase, vert),
   _frame(frame),
   _local(frame->inv()*vert->loc())
{
   // get notification of frame changes
   if (_frame)
      _frame->add_obs(this);
}

CWpt& 
XFMeme::compute_update()
{
   if (_debug) {
      err_adv(_debug, "XFMeme::compute_update");
      cerr << "CoordFrame* " << _frame << endl;
      cerr << "o: " << _frame->o() << endl;
      cerr << "t: " << _frame->t() << endl;
      cerr << "b: " << _frame->b() << endl;
      cerr << "n: " << _frame->n() << endl;
      DiskMap* dm = dynamic_cast<DiskMap*>(_frame);
      if (dm) {
         cerr << dm->identifier() << endl;
         MeshGlobal::select(dm->faces());
      } else
         cerr << "dynamic cast to DiskMap* failed" << endl;
   }
   if (_frame)
      _update = _frame->xf() * _local;
   else {
      err_adv(debug, "XFMeme::compute_update: no frame");
      _update = loc();
   }
   return _update;
}

bool
XFMeme::move_to(CWpt& p)
{
   if (!_frame) {
      err_adv(debug, "XFMeme::move_to: no frame");
      return false;
   }
   _local = _frame->inv() * p;
   do_update();
   return true;
}

void 
XFMeme::vert_changed_externally() 
{
   // Called when the vertex location changed but this VertMeme
   // didn't cause it:

   err_adv(debug, "XFMeme::compute_update: changing local coords");
   //_local = _frame->inv()*loc();
}

// end of file xf_meme.C
