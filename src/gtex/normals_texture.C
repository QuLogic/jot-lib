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
 * normals_texture.C
 **********************************************************************/
#include "normals_texture.H"
#include "std/config.H"
#include "gtex/gl_sphir_tex_coord_gen.H"
#include "gtex/gl_extensions.H"
#include "gtex/util.H"
#include "mesh/uv_data.H"
#include "mesh/vert_attrib.H"

using mlib::CWpt;
using mlib::Wvec;
using mlib::CWvec;

/**********************************************************************
 * VertNormalsTexture:
 *
 *      Draws the distinct normals at each vertex.
 *      (A vertex with adjacent crease edges has multiple normals.)
 **********************************************************************/
// helper 
inline void
draw_seg(CWpt& p, CWvec& v)
{
   glVertex3dv(p.data());
   glVertex3dv((p + v).data());
}

int
VertNormalsTexture::draw(CVIEWptr& v)
{
   if (_ctrl)
      return _ctrl->draw(v);

   // set attributes for this mode
   glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
   glDisable(GL_LIGHTING);              // GL_ENABLE_BIT
   GL_COL(COLOR::red, alpha());        // GL_CURRENT_BIT

   if (BasicTexture::draw(v)) {
      glPopAttrib();
      return _patch->num_faces();
   }

   CBMESH* mesh = _patch->mesh();
   if (!mesh) {
      err_msg( "VertNormalsTexture::draw: mesh is nil");
      return 0;
   }

   int dl = 0;
   if ((dl = _dl.get_dl(v, 1, _patch->stamp()))) {
      glNewList(dl, GL_COMPILE);
   }
      
   // length to use for showing normals:
   double scale = ((BMESH*)&*mesh)->get_bb().dim().length() / 50;

   // Get the Patch vertices:
   //   (expensive, but OK in a display list):
   Bvert_list verts = _patch->cur_verts().filter(PrimaryVertFilter());

   ARRAY<Wvec> norms;
   glBegin(GL_LINES);
   for (int i=0; i<verts.num(); i++) {
      CBvert* bv = verts[i];
      bv->get_normals(norms);
      for (int k=0; k<norms.num(); k++)
         draw_seg(bv->loc(), norms[k]*scale);
   }
   glEnd();

   // end the display list here
   if (_dl.dl(v)) {
      _dl.close_dl(v);

      // the display list is built; now execute it
      BasicTexture::draw(v);
   }

   // restore gl state:
   glPopAttrib();

   return _patch->num_faces();
}

/*****************************************************
 * draws uv gradient vectors as blue and green lines 
 ******************************************************/
//this is a test of the automatic spherical texture generator
//which is being used by *_EX shaders
class UV_attrib  :  public VertAttrib<UVpt,UVvec>
{
 public:
        
   UV_attrib() {
      auto_UV = new GLSphirTexCoordGen; //automatic generator
      auto_UV->setup();
   }

   virtual ~UV_attrib() { delete auto_UV; }

   virtual  UVpt get_attrib(CBvert* v, CBface* f) {
      assert(v&&f);
      TexCoordGen* tg = f->patch()->tex_coord_gen();
      if (tg)
         return tg->uv_from_vert(v,f);
      else if (UVdata::lookup(f))
         return UVdata::get_uv(v,f);
      else
         return auto_UV->uv_from_vert(v,f); 
   }

 protected :
   TexCoordGen* auto_UV;
};

int
VertUVTexture::draw(CVIEWptr& v)
{
   if (_ctrl)
      return _ctrl->draw(v);

   // set attributes for this mode
   glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
   glDisable(GL_LIGHTING);              // GL_ENABLE_BIT
   
   //change color so it doesn't get confused with normals
   GL_COL(COLOR::blue, alpha());        // GL_CURRENT_BIT

   if (BasicTexture::draw(v)) {
      glPopAttrib();
      return _patch->num_faces();
   }

   CBMESH* mesh = _patch->mesh();
   if (!mesh) {
      err_msg( "VertNormalsTexture::draw: mesh is nil");
      return 0;
   }

   int dl = 0;
   if ((dl = _dl.get_dl(v, 1, _patch->stamp()))) {
      glNewList(dl, GL_COMPILE);
   }

   cerr << "Please wait while compiling UV gradients display list" << endl;
      
   compute_UV_grads();

   // length to use for showing normals:
   double scale = ((BMESH*)&*mesh)->get_bb().dim().length() / 50;

   // Get the Patch vertices:
   //   (expensive, but OK in a display list):
   Bvert_list verts = _patch->cur_verts().filter(PrimaryVertFilter());

   ARRAY<Wvec> norms;
   UV_attrib attr;

   glBegin(GL_LINES);
   for (int i=0; i<verts.num(); i++) {
      CBvert* bv = verts[i];
      
      Bface_list faces = bv->get_faces();

      Wvec acc_U(0,0,0);
      Wvec acc_V(0,0,0);

      Wvec U_vec,V_vec;

      UVvec derivative;

      Wvec vertex_normal = bv->norm();
      Wvec base_2 = cross(vertex_normal,Wvec(1.0,0.0,0.0));
      Wvec base_3 = cross(vertex_normal,base_2);

      vertex_normal = vertex_normal.normalized();
      base_2 = base_2.normalized();
      base_3 = base_3.normalized();

      // Wtransf rotate( base_2, base_3, Wvec(0,0,0));
  
      for(int i=0; i<faces.num(); i++) {
         //getting the previously computed face gradients
         UV_grads grad = face_gradient_map[(unsigned int)(faces[i]->index())];
         
         U_vec = grad.U_grad;
         V_vec = grad.V_grad;
         
         //rotate to tangent plane

         double U_magnitude = U_vec.length();
         double V_magnitude = V_vec.length();

         U_vec =  Wvec((base_2*(base_2*U_vec)) + (base_3*(base_3*U_vec)));
         V_vec =  Wvec((base_2*(base_2*V_vec)) + (base_3*(base_3*V_vec)));

         U_vec = U_vec.normalized()*U_magnitude;
         V_vec = V_vec.normalized()*V_magnitude;

         acc_U += U_vec;
         acc_V += V_vec;
      }

      //just simple average for now
      acc_U = (acc_U / double(faces.num()) );
      acc_V = (acc_V / double(faces.num()) );

      GL_COL(COLOR::blue, alpha());  
      draw_seg(bv->loc(), acc_U*scale);
      GL_COL(COLOR::green, alpha());  
      draw_seg(bv->loc(), acc_V*scale);
   }
   glEnd();

   // end the display list here
   if (_dl.dl(v)) {
      _dl.close_dl(v);

      // the display list is built; now execute it
      BasicTexture::draw(v);
   }

   // restore gl state:
   glPopAttrib();

   return _patch->num_faces();
}

void 
VertUVTexture::compute_UV_grads()
{
   UV_attrib attr;
   Bface_list faces = patch()->faces();

   UV_grads gradient;
   UVvec derivative;

   for(int i=0; i<faces.num(); i++) {
      derivative = attr.dFdx(faces[i]);

      gradient.U_grad[0] = derivative[0]; 
      gradient.V_grad[0] = derivative[1]; 
         
      derivative = attr.dFdy(faces[i]);

      gradient.U_grad[1] = derivative[0]; 
      gradient.V_grad[1] = derivative[1]; 

      derivative = attr.dFdz(faces[i]);

      gradient.U_grad[2] = derivative[0]; 
      gradient.V_grad[2] = derivative[1]; 

      //uses the face pointer as a key
      face_gradient_map[(unsigned int)(faces[i]->index())]= gradient; 
   }
}

/**********************************************************************
 * NormalsTexture:
 **********************************************************************/
bool NormalsTexture::_uv_vectors = false;

int
NormalsTexture::draw(CVIEWptr& v)
{
   if (_ctrl)
      return _ctrl->draw(v);

   _smooth->draw(v);
  
   if (_uv_vectors) {
      _uv_vecs->draw(v);
   } else {
      _normals->draw(v);
   }
   return _patch->num_faces();
}
   
// end of file normals_texture.C
