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
 * gesture_carrier_drawer.C
 *
 **********************************************************************/

#include "geom/gl_view.H"
#include "std/config.H"

#include "gesture_carrier_drawer.H"

using namespace mlib;

int
GestureCarrierDrawer::draw(const GESTURE* gest, CVIEWptr& v) {
  if (_carrier_gesture){
    draw_carriers(gest, v);
  } else {
    draw_first_carrier(gest, v);
  }
  return 0;
}


void
GestureCarrierDrawer::draw_first_carrier(const GESTURE* gest, CVIEWptr& v) {
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
GestureCarrierDrawer::draw_carriers(const GESTURE* gest, CVIEWptr& v) {
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

  // draw the first carrier
  const PIXEL_list& pts1 = _carrier_gesture->pts();
  glBegin(GL_LINE_STRIP);
  for (int k=0; k< pts1.num(); k++) {
    glVertex2dv(pts1[k].data());
  }
  glEnd();

  // draw the second carrier
  const PIXEL_list& pts2 = gest->pts();
  glBegin(GL_LINE_STRIP);
  for (int k=0; k< pts2.num(); k++) {
    glVertex2dv(pts2[k].data());
  }
  glEnd();

  // draw ends
  PIXEL start1 = _carrier_gesture->start();
  PIXEL end1 = _carrier_gesture->end();
  PIXEL start2 = gest->start();
  PIXEL end2 = gest->end();

  glBegin(GL_LINES);
  glVertex2dv(start1.data());
  glVertex2dv(start2.data());

  glVertex2dv(end1.data());
  glVertex2dv(end2.data());
  glEnd();

  GL_VIEW::end_line_smooth();
  
  glPopAttrib();
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  
}


