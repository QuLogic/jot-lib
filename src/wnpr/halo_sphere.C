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
//halo sphere
//by Karol Szerszen

#include "geom/gl_view.H"
#include "gtex/wireframe.H" // for debugging

#include "halo_sphere.H"

static bool debug = Config::get_var_bool("DEBUG_HALO_SPHERE",false);

HaloSphere::HaloSphere()
{
   //setup the halo mesh
   sphere_mesh =  new BMESH;
   sphere_mesh->Sphere();
   sphere_mesh->fix_orientation(); // XXX - hack! fix this!!
   sphere_mesh->set_render_style("GLSL Toon Halo");
   //sphere_mesh->set_render_style("Flat Shading");
   
   if (_instance) 
      cerr << "Halo sphere already exists " << endl;
   else if (debug)
      cerr << "Halo sphere created with GLSL Toon Halo" << endl;
   _instance = this;
}

inline void
draw_wire(BMESHptr mesh)
{
   if (!mesh || mesh->npatches() < 1)
      return;
   WireframeTexture* wire = get_tex<WireframeTexture>(mesh->patch(0));
   assert(wire);
   wire->draw(VIEW::peek());
}

void
HaloSphere::draw_halo(CVIEWptr &v, CBBOX& box, double scale)
{
   GL_VIEW::print_gl_errors("halo_sphere.C : draw_halo #1 : OPEN_GL error : ");

   if (!box.valid())
      return;

   Wtransf tran =
      Wtransf::translation(box.center() - Wpt::Origin()) *
      Wtransf::scaling(box.dim()*scale);

   glPushAttrib(GL_ENABLE_BIT);
   glDisable(GL_DEPTH_TEST); // GL_ENABLE_BIT

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glMultMatrixd(tran.transpose().matrix());

   static bool do_wireframe = Config::get_var_bool("WIREFRAME_HALOS", false);
   if (do_wireframe)
      draw_wire(sphere_mesh);
   sphere_mesh->draw(v);
 
   GL_VIEW::print_gl_errors("halo_sphere.C : draw_halo #1 : OPEN_GL error : ");

   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();

   glPopAttrib();

   GL_VIEW::print_gl_errors("halo_sphere.C : draw_halo  #2 : OPEN_GL error : ");
}
