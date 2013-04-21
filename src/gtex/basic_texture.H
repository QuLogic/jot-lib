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
#ifndef _JOT_GTEX_BASIC_TEXTURE_H
#define _JOT_GTEX_BASIC_TEXTURE_H

#include "dlhandler/dlhandler.H"
#include "mesh/gtexture.H"
#include "std/config.H"
#include "gtex/util.H"

/**********************************************************************
 *
 *  Texture Stage Manager Object
 *
 **********************************************************************/
class Tex_Unit_Manager {
 public:
  
   int get_free_tex_stage(); 
   void free_tex_stage(int tex_stage);  //marks texture stage as unused
   void make_this_stage_used(int tex_stage);
   bool is_used_tex_stage(int tex_stage);

 private:
   vector <int> _used_texture_stages;   //unsorted
};

/**********************************************************************
 * OGLTexture:
 *
 *   Add to GTexture the ability to draw an IDRefImage
 **********************************************************************/
class OGLTexture : public GTexture {
 protected:

   void check_patch_texture_map();

 public:
   //******** MANAGERS ********
   OGLTexture(Patch *patch = 0, StripCB* cb=0) :
      GTexture(patch, cb ? cb : new GLStripCB) {}
   virtual ~OGLTexture() {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("OGLTexture", OGLTexture*, GTexture, CDATA_ITEM*);

   // convenience (affects GL_ENABLE_BIT)
   bool set_face_culling() const;

   // Draw silhouettes into the ID reference image,
   // optionally omitting the concave ones. This is useful
   // in conjunction with draw_id_triangles() (see below),
   // which draws filled triangles (black or ID colored)
   // with polygon offset:
   int draw_id_sils(bool omit_concave_sils, GLfloat width);
   int draw_id_triangles(
      bool use_polygon_offset, 
      bool draw_id_colors,
      double offset_factor = Config::get_var_dbl("BMESH_OFFSET_FACTOR", 12.0,true),
      double offset_units =  Config::get_var_dbl("BMESH_OFFSET_UNITS", 1.0,true)
      );
   int draw_id_creases(GLfloat width);
        
   // draws the patch into the stencil buffer only
   int draw_stencil(); 
   int draw_halo_ref(/*pass in the color*/);

   // most textures need to check for vertex and edge strips, and draw
   // them if needed, and most do so in the same way, so this is here
   // for convenience:
   void draw_vert_and_edge_strips(
      bool disable_lights = 0,
      GLfloat line_width  = 3,
      GLfloat point_size  = 5,
      bool set_color      = 1,
      CCOLOR& color       = COLOR::black
      ) const;

   //******** RefImageClient METHODS: ********
   virtual int draw_vis_ref();
   virtual int draw_id_ref();
};

/**********************************************************************
 * BasicTexture:
 *
 *   Base class for "procedural textures" that keep and manage a
 *   display list.
 **********************************************************************/
class BasicTexture : public OGLTexture {
 public:
   //******** MANAGERS ********
   BasicTexture(Patch* patch = 0, StripCB* cb=0) : OGLTexture(patch,cb){}
   virtual ~BasicTexture() { delete_dl(); }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("BasicTexture", BasicTexture*, OGLTexture, CDATA_ITEM*);

   //******** VIRTUAL METHODS ********
   virtual void changed() { _dl.invalidate(); }

   // Returns 1 if display list is used to draw, 0 otherwise
   virtual int draw(CVIEWptr& v);
   // Returns 1 if display list is used to draw, 0 otherwise
   virtual int dl_valid(CVIEWptr& v);


 protected:
   DLhandler    _dl;

   void delete_dl() {
      _dl.delete_all_dl();
   }
};

#endif // _JOT_GTEX_BASIC_TEXTURE_H

/* end of file basic_texture.H */
