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
 * control_frame.C
 **********************************************************************/
#include "gtex/gl_extensions.H"
#include "geom/gl_view.H"
#include "mesh/lpatch.H"
#include "mesh/mi.H"
#include "gtex/paper_effect.H"
#include "std/config.H"

#include "ffs_control_frame.H"

int
FFSControlFrameTexture::draw(CVIEWptr& v)
{
   
   if (_ctrl)
      return _ctrl->draw(v);

   // Create a strip for the given mesh type (BMESH or LMESH):
   assert(_patch && _patch->mesh());
   if (_strip)
      _strip->reset();
   else
      _strip = _patch->mesh()->new_edge_strip();

    
   // Init line smoothing and push attributes:
   GL_VIEW::init_line_smooth(1.0f, GL_CURRENT_BIT);

   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT

   if (!BasicTexture::draw(v)) {

     int dl = 0;
     if ((dl = _dl.get_dl(v, 1, _patch->stamp())))
       glNewList(dl, GL_COMPILE);

     // Draw control curves down to the current "edit level"
     int el = _patch->rel_edit_level();

     // Draw each level:
     for (int k = 0; k <= el; k++)
       draw_level(v, k);

      // end the display list here
      if (_dl.dl(v)) {
         _dl.close_dl(v);

         // the display list is built; now execute it
         BasicTexture::draw(v);
      }
   }

   // Restore gl state:
   GL_VIEW::end_line_smooth();

   draw_selected_faces();
   draw_selected_edges();
   draw_selected_verts();

   return _patch->num_faces();
}

void
FFSControlFrameTexture::draw_level(CVIEWptr& v, int k)
{
   // Draw the control curves for the Patch at level k,
   // which is relative to the control Patch.

   // Get the level-k edge strip:
   if (!build_strip(k))
      return;

   assert(_strip != NULL);

   // Set line thickness and color

   // Get a scale factor s used to control line width and color.
   // s drops toward 0 exponentially with increasing level:
   double r = Config::get_var_dbl("CONTROL_FRAME_RATIO", 0.6,true);
   assert(r > 0 && r < 1);
   double s = pow(r, _strip->mesh()->subdiv_level());

   // Default top-level line thickness is 1.0:
   double top_w = 1.0;
   double w = top_w*s;
   glLineWidth(GLfloat(v->line_scale()*w));       // GL_LINE_BIT

   // choose a lightest color:
   //COLOR l = interp(_color, COLOR::white, 0.75);

   
   // With white lines get too light in FFSTexture
   //l = interp(_color, COLOR(.659,.576,.467), 0.75);
   
   // line color drifts toward the light color as level k increases:
   //COLOR c = interp(l, _color, s);
   //GL_COL(c, alpha());                  // GL_CURRENT_BIT
   GL_COL(_color, s/3); 

    //paper
   //str_ptr tf = Config::JOT_ROOT() + "nprdata/paper_textures/simon.png";
   //str_ptr ret_tf;
   //TEXTUREptr paper;

   //paper = PaperEffect::get_texture(tf, ret_tf);

   //PaperEffect::begin_paper_effect(paper);
   //PaperEffect::begin_paper_effect(VIEW::peek()->get_use_paper());
   //for(int i=0; i < _strip->num(); i++){
   //   PaperEffect::paper_coord(NDCZpt(_strip->vert(i)->loc()).data());
   //   glTexCoord2dv(NDCZpt(_strip->vert(i)->loc()).data());
   //}

   //paper

   _strip->draw(_cb);

   //PaperEffect::end_paper_effect(paper);
}  

// end of file control_frame.C
