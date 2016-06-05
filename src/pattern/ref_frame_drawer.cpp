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
#include "pattern/ref_frame_drawer.H"
#include "geom/gl_view.H"
#include "pattern/stroke_group.H"



int 
RefFrameDrawer::draw(CVIEWptr& view){
  return 0;
}


int  
RefFrameDrawer::draw_final(CVIEWptr & v){
  draw_start();

  // first draw the reference frame
  glColor3f(0.5, 0.5, 0.5);
  glBegin(GL_LINES);
  if (_ref_frame==StrokeGroup::AXIS){
    glVertex2d(-1.0, 0.0);
    glVertex2d(1.0, 0.0);
  } else {
    glVertex2d(-0.05, 0.0);     
    glVertex2d(0.05, 0.0);     
    glVertex2d(0.0, -0.05);     
    glVertex2d(0.0, 0.05);     
  }
  glEnd();
  
  draw_end();

  return 0;
}


void 
RefFrameDrawer::draw_start(){
  // Push affected state
  glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );
  glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

  // Set state for drawing locators:
  glDisable(GL_LIGHTING);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D); 

  // Set projection and modelview matrices for drawing in NDC:
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadMatrixd(VIEW::peek()->ndc_proj().transpose().matrix());

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

}


void  
RefFrameDrawer::draw_end(){
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glPopClientAttrib();
  glPopAttrib();
}
