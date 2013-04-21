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
 * gesture_rect_drawer.C
 *
 **********************************************************************/

#include "geom/gl_view.H"
#include "std/config.H"

#include "gesture_rect_drawer.H"

using namespace mlib;

int
GestureRectDrawer::draw(const GESTURE* gest, CVIEWptr& v) {
  if (_axis_gesture){
    draw_rect(gest, v);
  } else {
    draw_skeleton(gest, v);
  }
  return 0;
}


void
GestureRectDrawer::draw_skeleton(const GESTURE* gest, CVIEWptr& v) {
  // load identity for model matrix
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
  GL_VIEW::init_line_smooth(GLfloat(v->line_scale()*2));
  double grey = 0.3f;
  glColor3d(grey, grey, grey);      // GL_CURRENT_BIT
  
  // draw the line
  const PIXEL_list& pts = gest->pts();
  int nb_pts = pts.num();
  if (nb_pts<2) return;

  glBegin(GL_LINES);
  glVertex2dv(pts[0].data());
  glVertex2dv(pts[nb_pts-1].data());
  glEnd();
  
  // I tried to implement some code for corner getting highlighted
  // it is on the gesture.C version on Unnameable
  
  GL_VIEW::end_line_smooth();
  
  glPopAttrib();
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  
}


void
GestureRectDrawer::draw_rect(const GESTURE* gest, CVIEWptr& v) {
  // load identity for model matrix
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
  GL_VIEW::init_line_smooth(GLfloat(v->line_scale()*2));
  double grey = 0.3f;
  glColor3d(grey, grey, grey);      // GL_CURRENT_BIT

  // compute the thickness of the rect
  if (gest->pts().num()<2) return;
  double thickness = 0.5 * gest->endpoint_dist();
//  const PIXEL_list& pts = _axis_gesture->pts();

  // draw edges
  PIXEL start = _axis_gesture->start();
  PIXEL end = _axis_gesture->end();
  VEXEL normal = (end-start).perpend().normalized();
  PIXEL rect_pt_a = start + normal*thickness;
  PIXEL rect_pt_b = end + normal*thickness;
  PIXEL rect_pt_c = end - normal*thickness;
  PIXEL rect_pt_d = start - normal*thickness;

  glBegin(GL_LINE_STRIP);
  glVertex2dv(rect_pt_a.data());
  glVertex2dv(rect_pt_b.data());
  glVertex2dv(rect_pt_c.data());
  glVertex2dv(rect_pt_d.data());
  glVertex2dv(rect_pt_a.data());
  glEnd();
  
  // draw handle
  double light_gray = 0.7;
  glColor3d(light_gray, light_gray, light_gray);
  glBegin(GL_LINES);
  glVertex2dv(gest->start().data());  
  glVertex2dv(gest->end().data());  
  glEnd();
   
  GL_VIEW::end_line_smooth();
  
  glPopAttrib();
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  
}



void
GestureRectDrawer::compute_rect_pt(const PIXEL_list& pts, int k, double thickness, PIXEL& pt) {
  int nb_pts = pts.num();
  VEXEL normal;
  if (k==0){
    normal = (pts[1]-pts[0]).perpend().normalized();
  } else if (k==nb_pts-1){
    normal = (pts[nb_pts-1]-pts[nb_pts-2]).perpend().normalized();    
  } else {
    normal = (pts[k+1]-pts[k-1]).perpend().normalized();
  }
  
  pt = pts[k] + normal*thickness;
}
