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
#include "image_plate.H"
#include "mesh/bmesh.H"
#include "geom/texturegl.H"
#include "gtex/tone_shader.H"
#include "npr/img_line_shader.H"

ImagePlate::ImagePlate() : GEOM()
{
   _do_halo = false;
}

ImagePlate::ImagePlate(Cstr_ptr& filename) : GEOM()
{
   _do_halo = false;

   BMESHptr m = new BMESH;

   TEXTUREglptr texture = new TEXTUREgl(filename, GL_TEXTURE_2D, TexUnit::PERLIN + GL_TEXTURE0);

//   if(!texture->load_texture()){
 //     return;
  // }

   //texture->set_tex_fn(GL_REPLACE);

   double tex_dim[2];
   XYpt topleft, topright, bottomleft, bottomright;
/*      Point2i ts = texture->get_size();
      tex_dim[0] = ts[0]; tex_dim[1] = ts[1];
      if(tex_dim[1] >= tex_dim[0]){
         tex_dim[0] = (1.0*tex_dim[0])/tex_dim[1];
         tex_dim[1] = 1.0;
      } else {
         tex_dim[1] = (1.0*tex_dim[1])/tex_dim[0];
         tex_dim[0] = 1.0;
      }*/

      tex_dim[0] = tex_dim[1] = 1.0;
          
      topleft     = XYpt(-tex_dim[0],  tex_dim[1]);
      topright    = XYpt( tex_dim[0],  tex_dim[1]);
      bottomleft  = XYpt(-tex_dim[0], -tex_dim[1]);
      bottomright = XYpt( tex_dim[0], -tex_dim[1]);
 //  m->Sphere();
      m->set_render_style("ImageLineShader");

      m->add_vertex(Wpt(bottomleft));
      m->add_vertex(Wpt(bottomright));
      m->add_vertex(Wpt(topright));
      m->add_vertex(Wpt(topleft));

      Patch *p = m->new_patch();

      UVpt u00(0,0), u10(1,0), u11(1,1), u01(0,1);

      m->add_quad(0, 1, 2, 3, u00, u10, u11, u01, p);
      m->changed(BMESH::TOPOLOGY_CHANGED);

      GTexture* tex = p->cur_tex(VIEW::peek());

      assert(ImageLineShader::isa(tex));
      ((ImageLineShader*)tex)->get_tone_shader()->set_tex_2d(texture);
      ((ImageLineShader*)tex)->get_basecoat_shader()->set_tex_2d(texture);

      cerr<<tex->type()<<endl;

   if(!m->has_name()){
      m->set_unique_name("image");
   }

   BMESH::set_focus(m);

   _body = m;

   _body->set_geom(this);
}

ImagePlate::~ImagePlate()
{
}

int ImagePlate::draw(CVIEWptr &v)
{
	return GEOM::draw(v);
}
