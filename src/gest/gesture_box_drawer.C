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
 * gesture_box_drawer.C
 *
 **********************************************************************/

#include "geom/gl_view.H"
#include "std/config.H"

#include "gesture_box_drawer.H"

using mlib::PIXEL;
using mlib::PIXEL_list;

int
GestureBoxDrawer::draw(const GESTURE* gest, CVIEWptr& v) {
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
  
  // draw the line strip
  const PIXEL_list& pts = gest->pts();
  int nb_pts = pts.num();
  if (nb_pts>=2){
    PIXEL pt_a = pts[0];
    PIXEL pt_b (pts[0][0], pts[nb_pts-1][1]);
    PIXEL pt_c = pts[nb_pts-1];
    PIXEL pt_d (pts[nb_pts-1][0], pts[0][1]);
    
    glBegin(GL_LINE_STRIP);
    double grey = 0.3f;
    glColor3d(grey, grey, grey);      // GL_CURRENT_BIT
    glVertex2dv(pt_a.data());
    glVertex2dv(pt_b.data());
    glVertex2dv(pt_c.data());
    glVertex2dv(pt_d.data());
    glVertex2dv(pt_a.data());
    glEnd();
  }
  
  // I tried to implement some code for corner getting highlighted
  // it is on the gesture.C version on Unnameable
  
  GL_VIEW::end_line_smooth();
  
  glPopAttrib();
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  
  return 0;
}


