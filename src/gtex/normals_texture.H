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
 * normals_texture.H:
 **********************************************************************/
#ifndef NORMALS_TEXTURE_H_IS_INCLUDED
#define NORMALS_TEXTURE_H_IS_INCLUDED

#include "smooth_shade.H"
#include <map>

/**********************************************************************
 * VertNormalsTexture:
 *
 *      Shows the vertex normals of the patch as red GL lines
 *      sticking out. No base coat.
 **********************************************************************/
class VertNormalsTexture : public BasicTexture {
 public:
   //******** MANAGERS ********
   VertNormalsTexture(Patch* patch = 0) : BasicTexture(patch) {}

   virtual ~VertNormalsTexture() {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("VertNormals", BasicTexture, CDATA_ITEM *);

   //******** GTexture VIRTUAL METHODS ********
   virtual bool draws_filled() const  { return false; }
   virtual int  draw(CVIEWptr& v); 

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new VertNormalsTexture; }
};


/************************
Helper storage class 
for UV Verts Texture
************************/
class UV_grads 
{
public :
   Wvec U_grad;
   Wvec V_grad;
};


/**********************************************************************
 * VertUVTexture:
 *
 *      Shows the UV gradients of the patch as blue(U) and green(V) GL lines
 *      sticking out. No base coat.
 **********************************************************************/
class VertUVTexture : public BasicTexture {
 public:
   //******** MANAGERS ********
   VertUVTexture(Patch* patch = 0) : BasicTexture(patch) {}

   virtual ~VertUVTexture() {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("VertUV", BasicTexture, CDATA_ITEM *);

   //******** GTexture VIRTUAL METHODS ********
   virtual bool draws_filled() const  { return false; }
   virtual int  draw(CVIEWptr& v); 

   virtual void compute_UV_grads();

   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new VertUVTexture; }

protected:

   map<unsigned int,UV_grads> face_gradient_map;

};





/**********************************************************************
 * NormalsTexture:
 *
 *   Draws the plain vertex normals with a gouraud shaded base coat.
 *   Added capability to display UV gradients in similar manner.
 **********************************************************************/
class NormalsTexture : public OGLTexture {
 public:
   //******** MANAGERS ********
   NormalsTexture(Patch* patch = 0) :
      OGLTexture(patch),
      _smooth(new SmoothShadeTexture(patch)),
      _normals(new VertNormalsTexture(patch)),
      _uv_vecs(new VertUVTexture(patch))
      {
    
      }

   virtual ~NormalsTexture() { gtextures().delete_all(); }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("Normals", OGLTexture, CDATA_ITEM *);

   //******** GTexture VIRTUAL METHODS ********

   virtual GTexture_list gtextures() const {
      return GTexture_list(_smooth, _normals, _uv_vecs);
   }
   
   virtual int set_color(CCOLOR& col) {
      _smooth->set_color(col);
      return 1;
   }
   virtual void draw_filled_tris(double alpha) {
      _smooth->draw_with_alpha(alpha);
   }
   virtual void draw_non_filled_tris(double alpha) {
      _normals->draw_with_alpha(alpha);
   }

   virtual int draw(CVIEWptr& v); 




   //******** DATA_ITEM VIRTUAL METHODS ********
   virtual DATA_ITEM  *dup() const { return new NormalsTexture; }

   //extra texture coord debug stuff
   static bool uv_vectors() { return _uv_vectors;};
   static void set_uv_vectors(bool set_v) { _uv_vectors = set_v; };
   static void toggle_uv_vectors() { _uv_vectors = !_uv_vectors; };

 protected:
   SmoothShadeTexture* _smooth;
   VertNormalsTexture* _normals;
   VertUVTexture* _uv_vecs;

  
   static bool _uv_vectors;  // display uv vectors instead of normals

};

#endif // NORMALS_TEXTURE_H_IS_INCLUDED

/* end of file normals_texture.H */
