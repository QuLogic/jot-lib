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
 * solid_color.C
 **********************************************************************/

#include "mesh/patch_blend_weight.H" // for debugging
#include "solid_color.H"

static bool debug_patch_blend =
   Config::get_var_bool("DEBUG_PATCH_BLEND_WEIGHTS",false);

// debug class: used to show the patch blend weights
class PatchBlendStripCB : public GLStripCB {
 public:
   PatchBlendStripCB(Patch* p) : _patch(p) {}

   void set_patch(Patch* p) { _patch = p; }
   void set_color(CCOLOR& c, double a) { _c = c; _a = a; }

   virtual void faceCB(CBvert* v, CBface* f) {
      if (_patch) {
         double w = PatchBlendWeight::get_weight(_patch, v);
         glColor4fv(float4(_c, _a*w));
      }
      glVertex3dv(v->loc().data());
   }
 protected:
   Patch* _patch;
   COLOR  _c;           // color
   double _a;           // alpha
};

/**********************************************************************
 * SolidColorTexture:
 **********************************************************************/
TAGlist*  SolidColorTexture::_solid_color_texture_tags   = NULL;

CTAGlist &
SolidColorTexture::tags() const
{
   if (!_solid_color_texture_tags) {
      _solid_color_texture_tags  = new TAGlist;

      *_solid_color_texture_tags += OGLTexture::tags();

      *_solid_color_texture_tags += new TAG_val<SolidColorTexture,COLOR>(
         "color",
         &SolidColorTexture::color_);
   }
   return *_solid_color_texture_tags;
}
int
SolidColorTexture::draw(CVIEWptr& v)
{
   if (_ctrl)
      return _ctrl->draw(v);

   bool show_weights = debug_patch_blend;
   if (show_weights) {
      if (!dynamic_cast<PatchBlendStripCB*>(cb())) {
         set_cb(new PatchBlendStripCB(patch()));
         assert(dynamic_cast<PatchBlendStripCB*>(cb()));
      }
   } else if (dynamic_cast<PatchBlendStripCB*>(cb())) {
      dynamic_cast<PatchBlendStripCB*>(cb())->set_patch(0);
   }

   // push GL state before changing things
   glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);

   // Set the color
   COLOR  c = _color;
   double a = _alpha*alpha();
   if (_track_view_color)
      c = v->color();
   glColor4fv(float4(c,a));

   // try it with the display list:
   if (BasicTexture::draw(v))
      return _patch->num_faces();

   // ensure this never happens again! get a display list:
   int dl = _dl.get_dl(v, 1, _patch->stamp());
   if (dl)
      glNewList(dl, GL_COMPILE);

   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT

   // set up face culling for closed surfaces
   set_face_culling();          // GL_ENABLE_BIT

   // draw the triangle strips
   PatchBlendStripCB* pbcb = dynamic_cast<PatchBlendStripCB*>(cb());
   if (show_weights) {
      glEnable(GL_BLEND);                                // GL_ENABLE_BIT
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_COLOR_BUFFER_BIT
      if (Config::get_var_bool("DEBUG_PATCH_BLEND_WEIGHTS",false)) {
         cerr << "SolidColorTexture::draw: patch "
              << mesh()->patches().get_index(patch())
              << ": drawing patch blend weights"
              << endl;
      }
      mesh()->update_patch_blend_weights();
      assert(pbcb);
      pbcb->set_patch(patch()->cur_patch());
      pbcb->set_color(c,a);
      _patch->draw_n_ring_triangles(
         mesh()->patch_blend_smooth_passes() + 1, pbcb, false
         );
      // draw the boundary w/ given line width and color:
      EdgeStrip bdry = patch()->cur_faces().get_boundary();
      GtexUtil::draw_strip(bdry, 2, Color::yellow);
   } else {
      if (pbcb)
         pbcb->set_patch(0);
      _patch->draw_tri_strips(_cb);
   }

   // restore gl state:
   glPopAttrib();

   // end the display list here
   if (_dl.dl(v)) {
      _dl.close_dl(v);

      // the display list is built; now execute it
      BasicTexture::draw(v);
   }

   return _patch->num_faces();
}

int
SolidColorTexture::write_stream(ostream& os) const
{
   os << _begin_tag << class_name() << endl << _color << endl << _end_tag;
   return 1;
}

int
SolidColorTexture::read_stream(istream& is, str_list &leftover)
{
   leftover.clear();
   COLOR col;
   is >> col;
   set_color(col);

   return 1;
}

/* end of file solid_color.C */
