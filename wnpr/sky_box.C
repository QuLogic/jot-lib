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
#include "disp/colors.H"
#include "geom/gl_view.H"
#include "geom/world.H"
#include "gtex/glsl_perlin_test.H"
#include "mesh/bmesh.H"
#include "npr/skybox_texture.H"

#include "sky_box.H"


/*************************************************
 * Implementation of SKY BOX   (under construction)
 *
 * SLILL UNDER CONSTRUCTION
 * Pardon all the garbage 
 **************************************************/
static bool debug          = Config::get_var_bool("DEBUG_SKYBOX",false);
static bool do_perlin_test = Config::get_var_bool("SKYBOX_TEXT_PERLIN",false);
static TEXTUREglptr perlin_tex;

SKY_BOXptr SKY_BOX::_sky_instance;

SKY_BOX::SKY_BOX()
{
   set_name("Skybox"); 

   // for debugging
//   set_color(Color::blue);

   // build the skybox mesh:
   LMESHptr sky_mesh = new LMESH;
   Patch* p = 0;
   //sky_mesh->Sphere();

   sky_mesh->UV_BOX(p);

   p->set_sps_regularity(8.0);
   p-> set_do_lod(false);

    //sky_mesh->Cube();
   //sky_mesh->Icosahedron();


  //sky_mesh->read_file(**(Config::JOT_ROOT()+"/../models/simple/sphere.sm"));
   //sky_mesh->read_file(**(Config::JOT_ROOT()+"/../models/simple/sphere-no-uvs.sm"));
   //sky_mesh->read_file(**(Config::JOT_ROOT()+"/../models/simple/icosahedron.sm"));
  // sky_mesh->update_subdivision(3);

 

   //flip normals
   const double RADIUS =1000;

   Wvec d = Wpt::Origin()-sky_mesh->get_bb().center();
   sky_mesh->transform(Wtransf::scaling(RADIUS)*
                       Wtransf::translation(d),MOD());
   //sky_mesh->reverse();
 

   // TODO : make it use skybox texture for the reference image
   //        and proxy surface for the main draw
   
   //Skybox renders in default style now
   //skybox texture only draws the gradient
   //Translation bug fixed for the 10th time he he

   //the skybox GEOM contains a bode with the sphere geometry
   set_body(sky_mesh); 

   //world creation 
   if (_sky_instance)
      err_msg("SKY_BOX::SKY_BOX: warning: instance exists");
   _sky_instance = this;
   
   atexit(SKY_BOX::clean_on_exit);

   WORLD::create(_sky_instance, false);
   WORLD::undisplay(_sky_instance, false);//otherwise it starts up visible
   if (Config::get_var_bool("SHOW_SKYBOX",false))
      show();
}

SKY_BOX::~SKY_BOX()
{
}

Patch*
SKY_BOX::get_patch()
{
   BMESHptr mesh = BMESH::upcast(_body);
   return (mesh && mesh->npatches() > 0) ? mesh->patch(0) : 0;
}

/************************************
The Draw function draws the sky 
using the current shader that can be 
assigned using jot's UI
*************************************/

int
SKY_BOX::draw(CVIEWptr &v)
{
   update_position();
   return GEOM::draw(v);
}

void 
SKY_BOX::test_perlin(CVIEWptr &v)
{
   //test
   if (!perlin_tex) {
      perlin_tex = new TEXTUREgl(
         Config::JOT_ROOT() +
         "nprdata/other_textures/" +
         "perlin_tex_RGB.png"
         );
      perlin_tex->load_texture();
   }
   assert(perlin_tex);
   if (perlin_tex->load_attempt_failed()) {
      cerr << "SKY_BOX::test_perlin: could not load file: "
           << perlin_tex->file()
           << endl;
      return;
   }
   
   // load identity for model matrix
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   // set up to draw in XY coords:
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadMatrixd(VIEW::peek()->xypt_proj().transpose().matrix());

   // set opengl state:
   glPushAttrib(GL_ENABLE_BIT);
   glDisable(GL_LIGHTING);
   glEnable(GL_TEXTURE_2D);
   glDisable(GL_CULL_FACE);
   glDisable(GL_DEPTH_TEST); // prevents depth testing AND writing to depth buffer
   glDisable(GL_ALPHA_TEST);
   //glDisable(GL_BLEND);

   perlin_tex->apply_texture(); // GL_ENABLE_BIT

   GLfloat a = 0.5f;
   glColor4f(0.5, 0.5, 0.5, 1);
   glBegin(GL_QUADS);
   // draw vertices in CCW order starting at bottom left:
   glTexCoord2f( 0,  0);
   glVertex2f  (-a, -a);
   glTexCoord2f( 1,  0);
   glVertex2f  ( a, -a);
   glTexCoord2f( 1,  1);
   glVertex2f  ( a,  a);
   glTexCoord2f( 0,  1);
   glVertex2f  (-a,  a);
   glEnd();

   // restore state:
   glPopAttrib();

   // restore projection matrix
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();

   // restore modelview matrix
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();

}
//**********************************************
// Draw the Gradient for the purpose of
// Modulating the main shader
//**********************************************

int  
SKY_BOX::DrawGradient(CVIEWptr &v)
{
   //DebugBreak();
   update_position(); //center at the eye position
   
      
   Patch* p = get_patch();
   assert(p);
   Skybox_Texture* tex = get_tex<Skybox_Texture>(p);
   assert(tex);

   glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT );
   glEnable(GL_DEPTH_TEST);

   // set xform:
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glMultMatrixd(xform().transpose().matrix());
 
   // draw the mesh normally:
   int ret= tex->draw(v);
          
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();

   glPopAttrib();

   GL_VIEW::print_gl_errors("sky_box.C : draw_gradient (regular) : ");

   return ret;
}

// sky box visibility functions
void
SKY_BOX::show()
{
   assert(_sky_instance);
   WORLD::display(_sky_instance, false);
   WORLD::message("Skybox is enabled");
   //err_adv(debug, "Skybox is enabled");
}

void
SKY_BOX::hide()
{
   assert(_sky_instance);
   WORLD::undisplay(_sky_instance, false);
   err_adv(debug, "Skybox is disabled");
}

void
SKY_BOX::toggle()
{
   assert(_sky_instance);
   WORLD::toggle_display(_sky_instance, false);
}


//-----------------ref image rendering

int 
SKY_BOX::draw_color_ref(int i)
{  
   if (i != 0)
      return 0;

   update_position(); 

   // XXX - probably wrong...
   return DrawGradient(VIEW::peek());
}

int 
SKY_BOX::draw_img(const RefImgDrawer& r, bool enable_shading)
{
   update_position();
   return GEOM::draw_img(r, enable_shading); 
}

void 
SKY_BOX::update_position() //centers the sky box around the camera
{
   Wpt eye = VIEW::eye();
   if (eye.dist_sqrd(xform().origin()) > 0) { //only update when really needed
      set_xform(Wtransf(eye));
      err_adv(debug, "SKY_BOX::update_position: updated skybox");
   }
}

// sky_box.C
