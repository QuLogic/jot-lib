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
 * basic_texture.C
 **********************************************************************/
#include "geom/texturegl.H"
#include "mesh/ioblock.H"
#include "mesh/lmesh.H"
#include "std/config.H"

#include "basic_texture.H"
#include "color_id_texture.H"
#include "control_line.H"
#include "creases_texture.H"
#include "curvature_texture.H"
#include "line_drawing.H"
#include "fader_texture.H"
#include "flat_shade.H"
#include "hidden_line.H"
#include "patch_color.H"
#include "key_line.H"
#include "normals_texture.H"
#include "ref_image.H"
#include "sil_frame.H"
#include "smooth_shade.H"
#include "solid_color.H"
#include "tri_strips_texture.H"
#include "wireframe.H"
#include "zxsil_frame.H"
#include "toon_texture_1D.H"
#include "glsl_shader.H"
#include "halftone_shader.H"
#include "halftone_shader_ex.H"
#include "hatching_tx.H"
#include "glsl_xtoon.H"
#include "glsl_toon.H"
#include "tone_shader.H"
#include "glsl_halo.H"
#include "glsl_toon_halo.H"
#include "dots.H"
#include "dots_ex.H"
#include "glsl_hatching.H"
#include "glsl_marble.H"
#include "glsl_normal.H"
#include "haftone_tx.H"
#include "painterly.H"
#include "blur_shader.H"
#include "halo_blur_shader.H" 
#include "halo_ref_image.H"
#include "patch_id_texture.H"
#include "multi_lights_tone.H"


// make sure textures can be looked up via DECODER mechanism
class DecoderAdds {
 public:
   DecoderAdds() {
      DECODER_ADD(ColorIDTexture);
      DECODER_ADD(ControlFrameTexture);
      DECODER_ADD(ControlLineTexture);
      DECODER_ADD(CreasesTexture);
      DECODER_ADD(CurvatureTexture);
      DECODER_ADD(FlatShadeTexture);
      DECODER_ADD(HiddenLineTexture);
      DECODER_ADD(PatchColorTexture);
      DECODER_ADD(KeyLineTexture);
      DECODER_ADD(LineDrawingTexture);
      DECODER_ADD(NormalsTexture);
      DECODER_ADD(SilFrameTexture);
      DECODER_ADD(SmoothShadeTexture);
      DECODER_ADD(SolidColorTexture);
      DECODER_ADD(TriStripsTexture);
      DECODER_ADD(VertColorTexture);
      DECODER_ADD(WireframeTexture);
      DECODER_ADD(ZcrossFrameTexture);    
      DECODER_ADD(ToonTexture_1D);
      DECODER_ADD(GLSLShader);
      DECODER_ADD(GLSLLightingShader);
      DECODER_ADD(HalftoneShader);
      DECODER_ADD(HalftoneShaderEx);
      DECODER_ADD(GLSLXToonShader);
      DECODER_ADD(GLSLToonShader);
      DECODER_ADD(ToneShader);
      DECODER_ADD(GLSLHaloShader);
      DECODER_ADD(GLSLToonShaderHalo);
      DECODER_ADD(DotsShader);
      DECODER_ADD(DotsShader_EX);
      DECODER_ADD(GLSLHatching);
      DECODER_ADD(GLSLMarbleShader);
      DECODER_ADD(Halftone_TX);
      DECODER_ADD(HatchingTX);
      DECODER_ADD(Painterly);
      DECODER_ADD(BlurShader);
      DECODER_ADD(GLSLNormalShader);
      DECODER_ADD(HaloBlurShader); 
      DECODER_ADD(PatchIDTexture);
      DECODER_ADD(MLToneShader);
   }
} DecoderAdds_static;

class SetupGL {
 public:
   SetupGL() {

      // Use VisRefImage's for visibility reference images
      VisRefImage::init();
   }
} SetupGL_static;

// XXX casey 3/18
// XXX enable stencilling; disable all else; just draw to stencil buffer

int OGLTexture::draw_stencil() {
        
   if (_patch == NULL) return -1;
   // push/pop attribs instead
   glColorMask(0, 0, 0, 0);
   glDisable(GL_DEPTH_TEST);
   //glStencilFunc(GL_ALWAYS, _patch->stencil_id(), 0xFF); 
   glStencilFunc(GL_ALWAYS, 1, 0xFF);
   glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
   //glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
   // just need the simplest kind- just uses GlVertex()
   GLStripCB cb; 
   _patch->draw_tri_strips(&cb);
   
   glColorMask(1, 1, 1, 1);
   glEnable(GL_DEPTH_TEST);
   
   return 1;
}

bool
OGLTexture::set_face_culling() const 
{
   // affects GL_ENABLE_BIT

   static bool no_cull = Config::get_var_bool("JOT_NO_FACE_CULLING",false);

   // If it's not VERBOTEN, and there's a reason to cull, then do:
   BMESH* m = get_cur_mesh(mesh());
   bool ret = (!no_cull && m && m->is_closed_surface());
   if (ret) glEnable (GL_CULL_FACE);        
   else     glDisable(GL_CULL_FACE);
   return ret;
}

int  
OGLTexture::draw_vis_ref() 
{
   // draw for visibility
   GTexture *tex = get_tex<ColorIDTexture>(_patch);

   // Draw simple, filled ID-colored triangles.
   // Some derived GTextures like Wireframe should override this and
   // draw wireframe ID-colored triangles (e.g.).
   return tex ? tex->draw(VIEW::peek()) : 0;
} 

int  
OGLTexture::draw_id_ref() 
{ 
   ColorIDTexture* tex = get_tex<ColorIDTexture>(_patch);
   if (tex) {
      // XXX - should draw black if the ID image isn't used 
      //       by this GTexture. we used to be able to find
      //       that out via use_ref_image(), but that's been
      //       changed. for now, drawing IDs (not black)...
      return tex->draw(VIEW::peek());
   }

   return 0;
}

int  
OGLTexture::draw_halo_ref(/*pass in the color*/) 
{ 
   //get solid color 
   SolidColorTexture* tex = get_tex<SolidColorTexture>(_patch);
   if (tex) {

      tex->set_color(HaloRefImage::lookup()->get_pass_color());
      return tex->draw(VIEW::peek());
   }

   return 0;
}

int  
OGLTexture::draw_id_sils(bool omit_concave_sils, GLfloat width) 
{
   // This is here for convenience because several GTextures use it.
   //
   // Draw silhouettes into the ID reference image, optionally
   // omitting the concave ones. This is useful in conjunction with
   // drawing filled triangles (black or ID colored) with polygon
   // offset, since the "backfacing" (concave) silhouette edges, which
   // should not be visible, show up incorrectly during rasterization,
   // which really sucks.

   // get the silhouette edges
   EdgeStrip& sils = _patch->cur_sils();

   if (omit_concave_sils) {
      // get an edge strip that omits the concave sil edges
      static EdgeStrip convex_sil_edges = sils.get_filtered(ConvexEdgeFilter());
      ColorIDTexture::draw_edges(&convex_sil_edges, width);
   } else {
      ColorIDTexture::draw_edges(&sils, width);
   }

   return 0;
}
   
int  
OGLTexture::draw_id_triangles(bool   use_polygon_offset, 
                              bool   draw_id_colors,
                              double offset_factor,
                              double offset_units) 
{
   // Another convenience method ... works well with the previous one.

   // draw triangles with ID colors, or black
   ColorIDTexture *tex = (ColorIDTexture*)
      _patch->get_tex(ColorIDTexture::static_name());
   if (tex) {
      if (use_polygon_offset)
         GL_VIEW::init_polygon_offset((float)offset_factor,
                                      (float)offset_units);
      if (draw_id_colors) tex->draw(VIEW::peek());
      else                tex->draw_black(VIEW::peek());
      if (use_polygon_offset)
         GL_VIEW::end_polygon_offset();
   } else
      err_msg( "OGLTexture::draw_id_triangles: warning -- no ColorIDTexture");

   return _patch->num_faces();
}

int
OGLTexture::draw_id_creases(GLfloat width) 
{
   // Yet another convenience method, for drawing crease id's this time.

   // get the crease edges
   EdgeStrip* creases = _patch->cur_creases();

   ColorIDTexture::draw_edges(creases, width);

   return 0;
}

void 
OGLTexture::draw_vert_and_edge_strips(
   bool disable_lights,
   GLfloat line_width,
   GLfloat point_size,
   bool set_color,
   CCOLOR& color
   ) const 
{
   // most textures need to check for vertex and edge strips, and draw
   // them if needed, and most do so in the same way, so this is here
   // for convenience.

   // edge strips
   if (_patch->num_edge_strips() > 0) {
      if (disable_lights) {
         glDisable(GL_LIGHTING);  // GL_ENABLE_BIT
         disable_lights = 0;
      }
      if (set_color) {
         glColor3dv(color.data());
         set_color = 0;
      }
      glLineWidth(line_width);
      _patch->draw_edge_strips(_cb);
      glLineWidth(1.0); // restore default
   }

   // vertex strips
   if (_patch->num_vert_strips() > 0) {
      if (disable_lights)
         glDisable(GL_LIGHTING);  // GL_ENABLE_BIT
      if (set_color)
         glColor3dv(color.data());
      glPointSize(point_size);
      _patch->draw_vert_strips(_cb);
      glPointSize(1.0); // restore default
   }
}

void 
OGLTexture::check_patch_texture_map()
{
   // XXX - only exists as a hack
   //
   // patches can read in the name of a texture file
   // to be used for the texture map associated with the
   // the patch. but patches can't know about the TEXTUREgl
   // class since lib mesh isn't supposed to know about
   // lib geom (tho it does, for now). anyway, this checks
   // whether the patch got assigned a texture file name
   // and if so it turns it into a TEXTUREgl that it assigns
   // to the patch.

   if (!_patch || _patch->has_texture())
      return;

   Cstr_ptr& n = _patch->texture_file();

   if (n && n != NULL_STR) {
      TEXTUREptr tex = new TEXTUREgl(n);

      if (tex->load_texture()) {
         _patch->set_texture(tex);
      }

      _patch->set_texture_file(NULL_STR);
   }
}

/**********************************************************************
 * BasicTexture:
 **********************************************************************/
int
BasicTexture::draw(CVIEWptr &v)
{
   // okay to draw if the display list time stamp
   // is more recent than the patch's time stamp

   int ret = dl_valid(v);
   
   if (ret==1) glCallList(_dl.dl(v));

   return ret;
}

int
BasicTexture::dl_valid(CVIEWptr &v)
{
   // okay to draw if the display list time stamp
   // is more recent than the patch's time stamp
   if (_dl.valid(v, (int) _patch->stamp())) {
      return 1;
   } else {
      return 0;
   }
}


//********Texture Unit Manager*************

int
Tex_Unit_Manager::get_free_tex_stage()
{

   return -1;
}

void 
Tex_Unit_Manager::free_tex_stage(int tex_stage)
{

}

bool 
Tex_Unit_Manager::is_used_tex_stage(int tex_stage)
{

   return false;
}

void 
Tex_Unit_Manager::make_this_stage_used(int tex_stage)
{

}

double
IDVisibilityTest::visibility(CWpt& p, double len)
{
   IDRefImage* id = IDRefImage::lookup();
   assert(id);

   Wpt hit;
   if (id->approx_wpt(NDCpt(PIXEL(p)), hit)) {
      // there is a surface out along the line of sight toward p
      Wpt eye = VIEW::eye();
      double dp = eye.dist(p);  // distance from cam to p
      double dh = eye.dist(hit);// distance from cam to hit

      // visibility is a fuzzy number depending on location of hit:
      //   if hit is much closer to camera, return 0
      //   if hit is in front of p, but near it, return nonzero
      //   if hit is farther than p, return 1

      return GtexUtil::smoothstep(dp-len, dp, dh);
   }
   // no surface out there, p is visible

   // XXX - not checking for in frustum
   return 1.0;
}

// end of file basic_texture.C
