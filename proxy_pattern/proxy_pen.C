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
/***************************************************************************
    proxy_pen.C
 ***************************************************************************/

#include "gtex/buffer_ref_image.H"
#include "gtex/ref_image.H"
#include "geom/command.H"
#include "mesh/mesh_global.H"


#include "stroke/gesture_stroke_drawer.H"

#include "proxy_pen.H"
#include "proxy_pen_ui.H"
#include <list>
#include <stack>

using namespace mlib;

ProxyPen::ProxyPen(
   CGEST_INTptr &gest_int,
   CEvent& d, CEvent& m, CEvent& u,
   CEvent& shift_down, CEvent& shift_up,
   CEvent& ctrl_down,  CEvent& ctrl_up) :
   Pen(str_ptr("Proxy Mode"),
       gest_int, d, m, u,
       shift_down, shift_up,
       ctrl_down, ctrl_up)
{
   // gestures we recognize:
   _draw_start += DrawArc(new TapGuard,      drawCB(&ProxyPen::tap_cb));
   _draw_start += DrawArc(new StrokeGuard,   drawCB(&ProxyPen::stroke_cb));

   _blank_gesture_drawer = new GestureDrawer();
  
   _ui = new ProxyPenUI();   
   assert(_ui);     
}

ProxyPen::~ProxyPen()
{
   if(_ui)
      delete _ui;
   if(_blank_gesture_drawer)
      delete _blank_gesture_drawer;
}

void
ProxyPen::activate(State *s)
{    
   if(_ui)
      _ui->show();
   Pen::activate(s);

   _gest_int->set_drawer(_blank_gesture_drawer);   
}

bool
ProxyPen::deactivate(State* s)
{
   if(_ui)
      _ui->hide();

   bool ret = Pen::deactivate(s);
   assert(ret);

   return true;
}

int
ProxyPen::tap_cb(CGESTUREptr& gest, DrawState*&)
{
   Bface *f = VisRefImage::Intersect(gest->center());
   Patch* p = get_ctrl_patch(f);
   
   cerr << "ProxyPen::tap_cb " << p << " " << f << endl;
   if (!f || !p)
       return 0;   

   BMESH::set_focus(p->mesh(), p); 
  
   _ui->update(); 

   return 0;
}

int
ProxyPen::stroke_cb(CGESTUREptr& gest, DrawState*&)
{
   err_msg("ProxyPen::stroke_cb()");   
   if (gest->pts().num() < 2) {
       WORLD::message("Failed to generate hatch stroke (too few samples)...");
       return 0;
   }
   // Find the patch to add the line to
   Bface* f = VisRefImage::Intersect(gest->center());
   if(!f){
       WORLD::message("Center of stroke not on a mesh");
       return 0;
   }
   Patch* p = get_ctrl_patch(f);       assert(p);
 
   // Project the stroke onto the surface and create Wpt_list from that
   NDCpt_list ndcpts = gest->pts();
   
   add_direcction_stroke(p, ndcpts);
   
   return 0;
}

bool
ProxyPen::add_direcction_stroke(Patch* p, NDCpt_list& pts)
{
   // It happens:
   if (pts.empty()) 
   {
      err_msg("ProxyPen:add_direcction_stroke() - Error: point list is empty!");
      return false;
   }
   Wpt_list wp_list;   
   for(int i=0; i < pts.num(); ++i){
      Wpt wp;
      VisRefImage::Intersect(pts[i],wp);
      wp_list += wp;
   }
   
   p->set_direction_stroke(wp_list);
   cerr << "and the stroke is " << p->get_direction_stroke();
   return true;
}


// end of file proxy_pen.C
