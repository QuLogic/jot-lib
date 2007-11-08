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
    img_line_stroke.H
    
    ImageLineStroke
    -------------------
***************************************************************************/

#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1300)) /*VS 6.0*/
#pragma warning(disable: 4786)
#endif

#include "gtex/ref_image.H" 
#include "gtex/glsl_paper.H"
#include "mesh/uv_data.H" 
#include "img_line_stroke.H"

static int foo = DECODER_ADD(ImageLineStroke);

ImageLineStroke::ImageLineStroke() 
{   
   _vis_type = VIS_TYPE_SCREEN | VIS_TYPE_SUBCLASS;
}

/////////////////////////////////////
// draw_circles()
/////////////////////////////////////
int
ImageLineStroke::draw_circles()
{
  //cerr<<"draw_circles"<<endl;
   int i;

   glPointSize(2.0);

   GL_COL( _color, _alpha );

   GLUquadricObj *qobj;
   qobj = gluNewQuadric();
   gluQuadricDrawStyle(qobj, GLU_LINE);
   gluQuadricNormals(qobj, GLU_NONE);

   for (i=0; i < _verts.num(); i++) {
      BaseStrokeVertex *v = &(_verts[i]);

      glPushMatrix();
      glTranslatef(v->_base_loc[0], v->_base_loc[1], v->_base_loc[2]);
      gluDisk(qobj, 0, v->_width*VIEW::pix_to_ndc_scale()*0.5, 5, 1);
      glPopMatrix();
   }

   glBegin(GL_LINES);

   mlib::NDCZpt p2;
   for (i=0; i < _verts.num(); i++) {
      BaseStrokeVertex *v = &(_verts[i]);

      glVertex3dv(v->_base_loc.data());
      p2 = v->_base_loc + v->_dir*v->_width*VIEW::pix_to_ndc_scale()*1.5;
      glVertex3dv(p2.data());
   }
   glEnd();

   return 0;
}

/////////////////////////////////////
// draw_debug()
/////////////////////////////////////
int
ImageLineStroke::draw_debug(CVIEWptr &)
{
   int tris = 0;

   if (_verts.num() < 2) return 0;

   _strokes_drawn++;

   //update();

//   if (_draw_verts.num() < 2) return 0;

   tris += draw_circles();
   
   return tris;
}

int 
ImageLineStroke::draw(CVIEWptr& v)
{
//   cerr << "ImageLineStroke::draw " << endl;       
   if (BaseStroke::get_debug()) return draw_debug(v);
   
   int tris = 0;

   if (_verts.num() < 2) return 0;

   _strokes_drawn++;

   update();

   if (_draw_verts.num() < 2) return 0;

   tris = draw_body();

   if (_overdraw)
   {
      assert(_overdraw_stroke);
      draw_end();
      _overdraw_stroke->draw_start();
      tris += _overdraw_stroke->draw(v);
      _overdraw_stroke->draw_end();
      draw_start();
      
   }
   return tris;
}
/////////////////////////////////////
// draw_start()
/////////////////////////////////////
void
ImageLineStroke::draw_start()
{
  BaseStroke::draw_start();
}

/////////////////////////////////////
// draw_end()
/////////////////////////////////////
void
ImageLineStroke::draw_end()
{
  BaseStroke::draw_end();
}

ImageLineStroke* 
ImageLineStroke::copy() const
{
   ImageLineStroke *s = new ImageLineStroke;
   assert(s);
   s->copy(*this);
   return s;
}
void                    
ImageLineStroke::copy(CImageLineStroke& s)
{
   BaseStroke::copy(s);
}

