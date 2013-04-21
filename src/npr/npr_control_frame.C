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
 * npr_control_frame.C
 **********************************************************************/


#include "geom/gl_view.H"
#include "geom/world.H"
#include "mesh/ledge_strip.H"
#include "mesh/bfilters.H"
#include "stroke/base_stroke.H"
#include "npr_control_frame.H"

using mlib::Wpt_list;
using mlib::NDCpt;
using mlib::NDCZpt;

/**********************************************************************
 * NPRControlFrameTexture:
 **********************************************************************/

bool NPRControlFrameTexture::_show_box = true;
bool NPRControlFrameTexture::_show_wire = false;

#define SELECTION_STROKE_TEXTURE "nprdata/system_textures/select_box.png"

/////////////////////////////////////
// Constructor
/////////////////////////////////////

NPRControlFrameTexture::NPRControlFrameTexture(
   Patch* patch, 
   CCOLOR& col) :
      BasicTexture(patch, new GLStripCB),
         _color(col),
         _alpha(1.0),
         _strip(0) 
{
   _strokes.add(new BaseStroke);
   _strokes[0]->set_texture(str_ptr(Config::JOT_ROOT() + SELECTION_STROKE_TEXTURE));
   _strokes[0]->set_use_depth(true);
   _strokes[0]->set_width(4);
   _strokes[0]->set_flare(0.2f);
   _strokes[0]->set_halo(0);
   _strokes[0]->set_fade(0);
   _strokes[0]->set_taper(15);

   for (int i=1; i<12; i++)
      _strokes.add(_strokes[0]->copy());
     
}

/////////////////////////////////////
// Destructor
/////////////////////////////////////

NPRControlFrameTexture::~NPRControlFrameTexture()
{
   for (int i=0; i<12; i++)
   {
      assert(_strokes[i]);
      delete _strokes[i];
   }

}

/////////////////////////////////////
// next_select_mode()
/////////////////////////////////////
void
NPRControlFrameTexture::next_select_mode()
{
   if ((!_show_box)&&(!_show_wire))
   {
      _show_box = false; _show_wire = true;
      WORLD::message("Changing NPR Selection: Wireframe Only");
   }
   else if ((!_show_box)&&(_show_wire))
   {
      _show_box = true; _show_wire = false;
      WORLD::message("Changing NPR Selection: BBOX Only");
   }
   else if ((_show_box)&&(!_show_wire))
   {
      _show_box = true; _show_wire = true;
      WORLD::message("Changing NPR Selection: BBOX and WireFrame");
   }
   else if ((_show_box)&&(_show_wire))
   {
      _show_box = false; _show_wire = false;
      WORLD::message("Changing NPR Selection: Invisible");
   }
}

/////////////////////////////////////
// draw()
/////////////////////////////////////

int
NPRControlFrameTexture::draw(CVIEWptr& v)
{
   int n=0;

   if (_ctrl)
      return _ctrl->draw(v);

   if (!_show_wire) return n;

   // Create a strip for the given mesh type (BMESH or LMESH):
   assert(_patch && _patch->mesh());
   if (!_strip)
      _strip = _patch->mesh()->new_edge_strip();

   // set line smoothing, set line width, and push attributes:
   GL_VIEW::init_line_smooth(GLfloat(v->line_scale()*0.75), GL_CURRENT_BIT);

   glDisable(GL_LIGHTING);             // GL_ENABLE_BIT
   GL_COL(_color, _alpha*alpha());     // GL_CURRENT_BIT

   if (!BasicTexture::draw(v)) {
      int dl = 0;
      if ((dl = _dl.get_dl(v, 1, _patch->stamp())))
         glNewList(dl, GL_COMPILE);
      
      Bedge_list edges = _patch->edges();
      edges.clear_flags();

      UnreachedSimplexFilter unreached;
      StrongEdgeFilter    strong;
      PatchEdgeFilter     mine(_patch);

      _strip->build(edges, unreached + strong + mine);
      _strip->draw(_cb);

      if (_dl.dl(v)) {
         _dl.close_dl(v);
         BasicTexture::draw(v);
      }
   }

   GL_VIEW::end_line_smooth();

   n += _patch->num_faces();

   return n;
}

/////////////////////////////////////
// draw_final()
/////////////////////////////////////
int 
NPRControlFrameTexture::draw_final(CVIEWptr& v) 
{
   int i,n = 0;

   const int box[12][2] = {{0,1},{0,2},{0,3},{7,6},{7,5},{7,4},
                              {2,4},{2,6},{6,1},{5,1},{5,3},{4,3}};

   //Draw the bounding box...
   if (_show_box)
   {
      double a = alpha();

      Wpt_list bb_pts;
      BBOX bb = _patch->xform() * _patch->mesh()->get_bb();
      bb.points(bb_pts);

      for (i=0; i<12; i++)
      {
         _strokes[i]->clear();
         _strokes[i]->set_color(_color); 
         _strokes[i]->set_alpha((float)(_alpha*a)); 

         if (NDCpt(bb_pts[box[i][0]]) != NDCpt(bb_pts[box[i][1]]))
         {
            _strokes[i]->add(NDCZpt(bb_pts[box[i][0]]));   
            _strokes[i]->add(NDCZpt(bb_pts[box[i][1]]));   
         }
      }

      _strokes[0]->draw_start();
      for (i=0; i<12; i++)
         n += _strokes[i]->draw(v);
      _strokes[0]->draw_end();
   }
   return n;
}
/* end of file npr_control_frame.C */
