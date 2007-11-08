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
/*!
 *  \file flat_shade.C
 *  \brief Contains the implementation fo the FlatShadeTexture GTexture and
 *  related classes.
 *
 *  \sa flat_shade.H
 *
 */

#include "disp/colors.H"
#include "geom/texturegl.H"
#include "geom/gl_view.H"
#include "gtex/util.H"
#include "mesh/uv_data.H"
#include "mesh/lmesh.H"
#include "mesh/ledge_strip.H"

#include "flat_shade.H"

bool FlatShadeTexture::_debug_uv = false;

static bool debug = Config::get_var_bool("DEBUG_DEBUG_UV",false);
/**********************************************************************
 * FlatShadeStripCB:
 **********************************************************************/
void 
FlatShadeStripCB::faceCB(CBvert* v, CBface* f) 
{
   // normal
   glNormal3dv(f->norm().data());

   if (v->has_color()) {
      GL_COL(v->color(), alpha*v->alpha());
   }

   // texture coords
   if (_do_texcoords) 
      if (_use_auto) { //force automatic 
         //use spherical text coord gen
         glTexCoord2dv(_auto_UV->uv_from_vert(v,f).data());
      } else {
         // the patch has a texture... try to find
         // appropriate texture coordinates...

         // use patch's TexCoordGen if possible,
         // otherwise use the texture coordinates stored
         // on the face (if any):
         TexCoordGen* tg = f->patch()->tex_coord_gen();
         if (tg)
            glTexCoord2dv(tg->uv_from_vert(v,f).data());
         else if (UVdata::lookup(f))
            glTexCoord2dv(UVdata::get_uv(v,f).data());
      }
  
   // vertex coords
   glVertex3dv(v->loc().data());
}

/**********************************************************************
 * UVDiscontinuousEdgeFilter:
 **********************************************************************/
class UVDiscontinuousEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && !UVdata::is_continuous((Bedge*)s);
   }
};

/**********************************************************************
 * HasUVFaceFilter:
 **********************************************************************/
class HasUVFaceFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_face(s) && UVdata::has_uv((Bface*)s);
   }
};

/**********************************************************************
 * FlatShadeTexture:
 **********************************************************************/
FlatShadeTexture::~FlatShadeTexture()
{
}

int
FlatShadeTexture::draw(CVIEWptr& v)
{
   if (_ctrl)
      return _ctrl->draw(v);
   _cb->alpha = alpha();

   if (!_patch)
      return 0;

   // Don't draw w/ texture maps if there are no uv coords at all.
   // but don't check every frame whether there are uv coords:
   
   if (_check_uv_coords_stamp != _patch->stamp()) {
      _has_uv_coords = _patch->cur_faces().any_satisfy(HasUVFaceFilter());
      _check_uv_coords_stamp = _patch->stamp();
   }

   // Set gl state (lighting, shade model)
   glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);
   glEnable(GL_LIGHTING);               // GL_ENABLE_BIT
   glShadeModel(GL_FLAT);               // GL_LIGHTING_BIT
   GL_COL(_patch->color(), alpha());    // GL_CURRENT_BIT
   
   if (debug_uv()) {

      //assign some texture coords if no tex_coord_gen is assigned
      if (!_patch->tex_coord_gen()) {
         _patch->set_tex_coord_gen(new GLSphirTexCoordGen);      
      }

      // decide what image file to use to show the uv-coords:
      str_ptr debug_uv_tex_name =
         Config::get_var_str("DEBUG_UV_TEX_MAP", "checkerboard.png",true);
      static str_ptr pre_path = Config::JOT_ROOT() + "nprdata/other_textures/";
      str_ptr path = pre_path + debug_uv_tex_name;

      // we only try a given pathname once. but if it fails
      // and they reset DEBUG_UV_TEX_MAP while running the
      // program we can pick up the change and try again.
      if (path != _debug_tex_path) {
         _debug_tex_path = path;
         TEXTUREglptr tex = new TEXTUREgl(_debug_tex_path);
         bool do_mipmap = Config::get_var_bool("DEBUG_TEX_USE_MIPMAP",false);
         tex->set_mipmap(do_mipmap);
         _debug_uv_tex = tex;
         _debug_uv_in_dl = false;

         if (debug) {
            cerr << "Loading debug uv texture " << **path << " ..." << endl;
         }

         if (!_debug_uv_tex->load_texture()) {
            cerr << "Can't load debug uv texture: "
                 << **_debug_tex_path
                 << endl
                 << "Set environment variable DEBUG_UV_TEX_MAP "
                 << "to an image file in "
                 << **pre_path
                 << " and try again."
                 << endl;
            _debug_uv_tex = 0;
         }
      }

      // Apply the texture if any faces have uv coordinates:
      if (_debug_uv_tex /*&& _has_uv_coords*/) //<- using auto UV
         _debug_uv_tex->apply_texture(); // GL_ENABLE_BIT

   } else {
      
      // XXX - dumb hack
      check_patch_texture_map();

      // if (_has_uv_coords)    <- using auto UV
      _patch->apply_texture(); // GL_ENABLE_BIT
   }

   // Set material parameters for OGL:
   GtexUtil::setup_material(_patch);

   // Try for the display list if it's valid
   if (!(_debug_uv == _debug_uv_in_dl && BasicTexture::draw(v))) {

      // Try to generate a display list
      int dl = _dl.get_dl(v, 1, _patch->stamp());
      if (dl) {
         glNewList(dl, GL_COMPILE);
         _debug_uv_in_dl = _debug_uv;
      }
      
      // Set up face culling for closed surfaces
      if (!set_face_culling()) 
         glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);  // GL_LIGHTING_BIT
      
      FlatShadeStripCB *flat_cb = dynamic_cast<FlatShadeStripCB*>(_cb);
      if (flat_cb) {
         // send texture coordinates?
         if ((_debug_uv && _debug_uv_tex) ||
             (_has_uv_coords && _patch->has_texture())) {
            flat_cb->enable_texcoords();
         } else {
            flat_cb->disable_texcoords();
         }
      }
      err_adv(debug && _debug_uv, "  drawing uvs in flat shade");
      
      // draw the triangle strips
      _patch->draw_tri_strips(_cb);

      // end the display list here
      if (_dl.dl(v)) {
         _dl.close_dl(v);

         // the display list is built; now execute it
         BasicTexture::draw(v);
      }
   }

   // restore GL state
   glPopAttrib();

   // Have to move this outside the display list and gl push/pop attrib
   // or else it's all:
/*
  FlatShadeTexture::draw - End ***NOTE*** OpenGL Error: [500] 'Invalid Enumerator'
  FlatShadeTexture::draw - End ***NOTE*** OpenGL Error: [500] 'Invalid Enumerator'
  FlatShadeTexture::draw - End ***NOTE*** OpenGL Error: [500] 'Invalid Enumerator'
  FlatShadeTexture::draw - End ***NOTE*** OpenGL Error: [500] 'Invalid Enumerator'
  FlatShadeTexture::draw - End ***NOTE*** OpenGL Error: [500] 'Invalid Enumerator'
  FlatShadeTexture::draw - End ***NOTE*** OpenGL Error: [500] 'Invalid Enumerator'
  FlatShadeTexture::draw - End ***NOTE*** OpenGL Error: [500] 'Invalid Enumerator'
  FlatShadeTexture::draw - End ***NOTE*** OpenGL Error: [500] 'Invalid Enumerator'
*/
   if (_debug_uv) {

      // Build the edge strip at the current subdivision level
      Bedge_list edges = _patch->cur_edges();
      edges.clear_flags();

      // If secondary edges shouldn't be drawn, set their flags
      // so they won't be drawn:
      if (!BMESH::show_secondary_faces())
         edges.secondary_edges().set_flags(1);

      // Draw uv-discontinuity boundaries as yellow lines.
      // Construct filter that accepts unreached uv-discontinuous
      // edges of this patch:
      UnreachedSimplexFilter    unreached;
      UVDiscontinuousEdgeFilter uvdisc;
      PatchEdgeFilter           mine(_patch->cur_patch());
      EdgeStrip disc_edges(edges, unreached + uvdisc + mine);
      if (!disc_edges.empty()) {
         GtexUtil::draw_strip(disc_edges, 3, Color::yellow, 0.8);
      }
   }

   GL_VIEW::print_gl_errors("FlatShadeTexture::draw - End");

   return _patch->num_faces();
}

// end of file flat_shade.C
