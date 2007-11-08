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
#include "pattern/bbox_cell.H"

#include "geom/gl_view.H"
#include "std/config.H"

using namespace std;
using namespace mlib;

const double BBoxCell::FADE_WIDTH = 0.1;

////////////////////////////////
// constructor / destructor
////////////////////////////////

BBoxCell::BBoxCell(CPIXEL& p, CPIXEL& q){

  PIXEL l, h; 
  if (p[0]<q[0]){
    l[0] = p[0];
    h[0] = q[0];
  } else {
    l[0] = q[0];
    h[0] = p[0];     
  }
  if (p[1]<q[1]){
    l[1] = p[1];
    h[1] = q[1];
  } else {
    l[1] = q[1];
    h[1] = p[1];     
  }    
  _bbox = BBOXpix(l, h);
  if (_bbox.width()>_bbox.height()){
    _scale = _bbox.width();
    _offset = VEXEL(0.0, (_bbox.height()-_bbox.width())*0.5);
  } else {
    _scale = _bbox.height();
    _offset = VEXEL((_bbox.width()-_bbox.height())*0.5, 0.0);
  }
}

//////////////////
// accessors
//////////////////

void 
BBoxCell::add_stroke(const vector<UVpt>& pts,
		     const vector<double>& pressures){
  
  GestureStroke stroke (_bbox);
  
  unsigned int nb_pts = pts.size();
  for (unsigned int i=0 ; i<nb_pts ; i++){
    PIXEL src_pix  = PIXEL(pts[i][0], pts[i][1])*_scale+_offset;
    PIXEL dest_pix = _bbox.min() + src_pix;
    double dest_pressure = pressures[i] * alpha(pts[i]) * GestureCell::alpha(pts[i], dest_pix);
    stroke.add_stroke_pt(dest_pix, dest_pressure);
  }

  _strokes.push_back(stroke);
}



double 
BBoxCell::alpha (CUVpt& pt) const{
  double x,y;
  double inf, sup;
  if (_bbox.width()>_bbox.height()){
    x = pt[0];
    y = pt[1];
    inf = (1.0-(_bbox.height()/_bbox.width()))*0.5;
    sup = 1.0-inf;
  } else {
    x = pt[1];
    y = pt[0];    
    inf = (1.0-(_bbox.width()/_bbox.height()))*0.5;
    sup = 1.0-inf;
  }

  if (x>=0.0 && x<=1.0){
    if (y<inf || y>sup){
      return 0.0;
    } else if (y<inf || y>sup) {
      return min(y-inf, sup-y)/inf;
    } else {
      return 1.0;
    }
  } else {
    return 0.0;
  }
}

void 
BBoxCell::draw_cell_bound(){
	
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
		  
  // set up to draw in PIXEL coords:
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadMatrixd(VIEW::peek()->pix_proj().transpose().matrix());
		  
  // Set up line drawing attributes
  glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
  glDisable(GL_LIGHTING);              // GL_ENABLE_BIT
		  
  // turn on antialiasing for width-2 lines:
  GL_VIEW::init_line_smooth(GLfloat(VIEW::peek()->line_scale()*2));
		  
  // draw the line strip
		  
  PIXEL pt_a = _bbox.min();
  PIXEL pt_b (_bbox.max()[0], _bbox.min()[1]);
  PIXEL pt_c = _bbox.max();
  PIXEL pt_d (_bbox.min()[0], _bbox.max()[1]);
		    
  glBegin(GL_LINE_STRIP);
  double grey = 0.3f;
  glColor3d(grey, grey, grey);      // GL_CURRENT_BIT
  glVertex2dv(pt_a.data());
  glVertex2dv(pt_b.data());
  glVertex2dv(pt_c.data());
  glVertex2dv(pt_d.data());
  glVertex2dv(pt_a.data());
  glEnd();
		    
  // I tried to implement some code for corner getting highlighted
  // it is on the gesture.C version on Unnameable
		  
  GL_VIEW::end_line_smooth();
		  
  glPopAttrib();
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
	

}

