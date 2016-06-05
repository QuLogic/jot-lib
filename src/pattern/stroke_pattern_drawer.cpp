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
#include "pattern/stroke_pattern_drawer.H"
#include "geom/gl_view.H"
#include "pattern/stroke_group.H"
#include "pattern/stroke_path.H"



int 
StrokePatternDrawer::draw(CVIEWptr& view){
  return 0;
}


int  
StrokePatternDrawer::draw_final(CVIEWptr & v){
  if (!_pattern) return 0;
  
  draw_start();
  
  // for each group
  const vector<StrokeGroup*>& groups = _pattern->groups();
  unsigned int nb_groups = groups.size();
  for (unsigned int i=0 ; i<nb_groups ; i++){
    const StrokeGroup* current_group = groups[i];
    const vector<StrokePaths*>& paths = current_group->paths();

    // first draw the element neighbors
    const vector<int>& elements = current_group->elements();
    const vector< vector<int> >& element_neighbors = current_group->element_neighbors();
    unsigned int nb_elements = elements.size();
    glColor3f(1.0, 1.0, 0.0);
    glBegin(GL_LINES);
    for (unsigned int j=0 ; j<nb_elements ; j++){
      int idx1 = elements[j];
      unsigned int nb_neighbors = element_neighbors[j].size();
      for (unsigned int k=0 ; k<nb_neighbors ; k++){
	int idx2 = element_neighbors[j][k];
	const StrokePaths* first_path = paths[idx1];
	const StrokePaths* second_path = paths[idx2];
	glVertex2dv(NDCZpt(first_path->center()).data());
	glVertex2dv(NDCZpt(second_path->center()).data());
      }
    }
    glEnd();


    // then draw the paths
    glColor3f(1.0, 0.0, 0.0);
    for(unsigned int j=0 ; j<nb_elements ; j++){
      const StrokePaths* current_path = paths[elements[j]];
      if (current_path->type()==StrokePaths::LINE){
	glBegin(GL_LINES);
	PIXEL start, end;      
	start = current_path->center() - current_path->axis_a()*0.5;
	end = current_path->center() + current_path->axis_a()*0.5;
	glVertex2dv(NDCZpt(start).data());
	glVertex2dv(NDCZpt(end).data());      
	glEnd();
      } else if (current_path->type()==StrokePaths::POINT) {
	glBegin(GL_POINTS);
	glVertex2dv(NDCZpt(current_path->center()).data());	
	glEnd();
      } else if (current_path->type()==StrokePaths::BBOX) {
	glBegin(GL_LINE_STRIP);
	PIXEL corner;
	corner = current_path->center() + current_path->axis_a()*0.5 + current_path->axis_b()*0.5;
	glVertex2dv(NDCZpt(corner).data());
	corner = current_path->center() - current_path->axis_a()*0.5 + current_path->axis_b()*0.5;
	glVertex2dv(NDCZpt(corner).data());
	corner = current_path->center() - current_path->axis_a()*0.5 - current_path->axis_b()*0.5;
	glVertex2dv(NDCZpt(corner).data());
	corner = current_path->center() + current_path->axis_a()*0.5 - current_path->axis_b()*0.5;
	glVertex2dv(NDCZpt(corner).data());
	corner = current_path->center() + current_path->axis_a()*0.5 + current_path->axis_b()*0.5;
	glVertex2dv(NDCZpt(corner).data());
	glEnd();
      }
    }


  }
  
  draw_end();

  return 0;
}


void 
StrokePatternDrawer::draw_start(){
  // Push affected state
  glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );
  glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

  // Set state for drawing locators:
  glDisable(GL_LIGHTING);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D); 
  glPointSize(4.0f);
  glLineWidth(2.0f);

  // Set projection and modelview matrices for drawing in NDC:
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadMatrixd(VIEW::peek()->ndc_proj().transpose().matrix());

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

}


void  
StrokePatternDrawer::draw_end(){
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glPopClientAttrib();
  glPopAttrib();
}
