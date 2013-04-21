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
/*****************************************************************
 * impostor.C
 *****************************************************************/
#include "impostor.H"
#include "geom/gl_view.H"

/*****************************************************************
 * Impostor
 *****************************************************************/

Impostor::Impostor(BMESH* m)
{
   _mesh = m;
}

Impostor::~Impostor() {
	
}


int
Impostor::draw(CVIEWptr&)
{   
   return 0;
}

int
Impostor::draw_final(CVIEWptr&)
{
   if(_mesh){
     draw_start();
    
     // b-c
     // | | 
     // a-d
     mlib::NDCZpt a, c;
     _mesh->get_bb().ndcz_bounding_box(_mesh->obj_to_ndc(),a,c);
     mlib::NDCZpt b(a[0], c[1], 0);
     mlib::NDCZpt d(c[0], a[1], 0);
 
     
  //   cerr << "final draw impostor " << min_pt << " and " << max_pt << endl;

  //glBegin(GL_QUADS);
  glBegin(GL_LINE_STRIP);   
	      glVertex2dv(a.data());
         glVertex2dv(b.data());
	      glVertex2dv(c.data());
         glVertex2dv(d.data());
         glVertex2dv(a.data());
	glEnd();
    
	 
  draw_end();

   }  
   return 0;

}

void 
Impostor::draw_start(){
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
  glColor3f(0.0f,0.0f,0.0f);    

  // Set projection and modelview matrices for drawing in NDC:
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadMatrixd(VIEW::peek()->ndc_proj().transpose().matrix());

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

}


void  
Impostor::draw_end(){
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glPopClientAttrib();
  glPopAttrib();
}
// end of file impostor.C
