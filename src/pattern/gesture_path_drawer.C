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
 * gesture_path_drawer.C
 *
 **********************************************************************/

#include "geom/gl_view.H"
#include "std/config.H"

#include "gesture_path_drawer.H"

using namespace mlib;

int
GesturePathDrawer::draw(const GESTURE* gest, CVIEWptr& v) {
  if (_path_gesture){
    draw_path(gest, v);
  } else {
    draw_skeleton(gest, v);
  }
  return 0;
}


void
GesturePathDrawer::draw_skeleton(const GESTURE* gest, CVIEWptr& v) {
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
  
  // draw the line strip
  const PIXEL_list&    pts   = gest->pts();
  glBegin(GL_LINE_STRIP);
  for (int k=0; k< pts.num(); k++) {
      glVertex2dv(pts[k].data());
  }
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
GesturePathDrawer::draw_path(const GESTURE* gest, CVIEWptr& v) {
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

  // compute the thickness of the path
  if (gest->pts().num()<2) return;
  double thickness = 0.5 * gest->endpoint_dist();
  const PIXEL_list& pts = _path_gesture->pts();
  int nb_pts = pts.num();

  // draw the upper line strip
  glBegin(GL_LINE_STRIP);
  for (int k=0; k< pts.num(); k++) {
    PIXEL rect_pt;
    compute_path_pt(pts, k, thickness, rect_pt);
    glVertex2dv(rect_pt.data());
  }
  glEnd();

  // draw the lower line strip
  glBegin(GL_LINE_STRIP);
  for (int k=0; k< pts.num(); k++) {
    PIXEL rect_pt;
    compute_path_pt(pts, k, -thickness, rect_pt);
    glVertex2dv(rect_pt.data());
  }
  glEnd();

  // draw ends
  PIXEL path_pt;
  glBegin(GL_LINES);
  compute_path_pt(pts, 0, thickness, path_pt);
  glVertex2dv(path_pt.data());
   compute_path_pt(pts, 0, -thickness, path_pt);
  glVertex2dv(path_pt.data());

  compute_path_pt(pts, nb_pts-1, thickness, path_pt);
  glVertex2dv(path_pt.data());
   compute_path_pt(pts, nb_pts-1, -thickness, path_pt);
  glVertex2dv(path_pt.data());
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
GesturePathDrawer::compute_path_pt(const PIXEL_list& pts, int k, double thickness, PIXEL& pt) {
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
