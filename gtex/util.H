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
#ifndef GTEX_UTIL_H_IS_INCLUDED
#define GTEX_UTIL_H_IS_INCLUDED

#include "geom/gl_view.H"
#include "geom/world.H"
#include "geom/show_tris.H"
#include "mesh/edge_strip.H"
#include "mesh/stripcb.H"
#include "mesh/lmesh.H"

/**********************************************************************
 * namespace TexUnit:
 *
 *   Used to coordinate what texture stages are used for
 *   what purposes. Texture units 1 - 3 are reserved for
 *   paper texture, perlin noise, and reference images in
 *   texture memory (when those effects are active).
 **********************************************************************/
namespace TexUnit {
   enum tex_stage_t { APP=0, PAPER, PERLIN, REF_IMG, PATTERN_TEX, REF_IMG2, REF_HALO};
};

class TriStrip;
/**********************************************************************
 * GLStripCB:
 *
 *      provides default callbacks to use for drawing
 *      triangle, line or point strips with OpenGL.
 **********************************************************************/
class GLStripCB : public StripCB {
 public:

   // most derived classes will not need to override these:
   virtual void begin_faces(TriStrip*)  { glBegin(GL_TRIANGLE_STRIP); } 
   virtual void end_faces  (TriStrip*)  { glEnd(); }

   virtual void begin_edges(EdgeStrip*) { glBegin(GL_LINE_STRIP); }
   virtual void end_edges  (EdgeStrip*) { glEnd(); }

   virtual void begin_verts(VertStrip*) { glBegin(GL_POINTS); }
   virtual void end_verts  (VertStrip*) { glEnd(); }

    // for triangles
   virtual void begin_triangles()       { glBegin(GL_TRIANGLES); }  
   virtual void end_triangles  ()       { glEnd(); }

   // most derived classes will override these to specify vertex
   // normals, colors, and/or texture coordinates in addition to
   // vertex coordinates:
   virtual void faceCB(CBvert* v, CBface*) { glVertex3dv(v->loc().data()); }
   virtual void edgeCB(CBvert* v, CBedge*) { glVertex3dv(v->loc().data()); }
   virtual void vertCB(CBvert* v)          { glVertex3dv(v->loc().data()); }
};

/*****************************************************************
 * VertNormStripCB:
 *
 *   Convenience: sends both vertex normal and position to OpenGL.
 *   Used for basic rendering styles that use lighting but don't
 *   send texture coordinates or colors (etc.) to OpenGL.
 *****************************************************************/
class VertNormStripCB : public GLStripCB {
 public:
   virtual void faceCB(CBvert* v, CBface* f) {
      glNormal3dv(f->vert_normal(v).data());
      glVertex3dv(v->loc().data());
   }
   virtual void edgeCB(CBvert* v, CBedge* e) {
      // note: we're sending vertex normal along with vertex
      // when drawing line strips (in case lighting is enabled:
      glNormal3dv(v->norm().data());
      glVertex3dv(v->loc().data());
   }
};

/*****************************************************************
 * utilities
 *****************************************************************/
namespace GtexUtil {

   inline double
   smoothstep(double e1, double e2, double x) {
      // same as the GLSL smoothstep() function
      double t = clamp((x - e1)/(e2 - e1), 0.0, 1.0);
      return t * t * (3 - 2*t);
   }

   inline str_ptr toon_path() {
      return Config::JOT_ROOT() + "nprdata/toon_textures/"; 
   }
   inline str_ptr toon_name(Cstr_ptr& name) { return toon_path() + name; }

   inline str_ptr toon_1D_path() {
      return Config::JOT_ROOT() + "nprdata/toon_textures_1D/"; 
   }
   inline str_ptr toon_1D_name(Cstr_ptr& name) { return toon_1D_path() + name; }

   // sent material properties to OpenGL
   inline void 
   setup_material(APPEAR* a)
   {
      // XXX - not handling emmissive color
      assert(a);
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,
                   float4(a->ambient_color(),a->transp()));
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,
                   float4(a->color(),a->transp()));
      glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,
                   float4(a->specular_color(),a->transp()));
      glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, (GLfloat)a->shininess());
   }

   // Draw an EdgeStrip
   inline void
   draw_strip(
      EdgeStrip& strip, // edge strip to render
      double width,     // width for gl lines
      CCOLOR& color,    // color 
      double alpha=1,   // alpha
      StripCB* cb=0     // callback for edge strip (uses GLStripCB by default)
      )
   {
      VIEWptr v = VIEW::peek();
      GL_VIEW::init_line_smooth(GLfloat(v->line_scale()*width), GL_CURRENT_BIT);
 
      glDisable(GL_LIGHTING);      // GL_ENABLE_BIT
      GL_COL(color, alpha);        // GL_CURRENT_BIT
 
      GLStripCB glcb;
      strip.draw(cb ? cb : &glcb);

      GL_VIEW::end_line_smooth();
   }
   inline void draw_strip(EdgeStrip& strip, double width=1, StripCB* cb=0) {
      return draw_strip(strip, width, Color::black, 1, cb);
   }

   // Helper used in show_tris() below:
   inline SHOW_TRIS::Triangle tri(Bface* f) {
      return f ?
         SHOW_TRIS::Triangle(f->v1()->wloc(),f->v2()->wloc(),f->v3()->wloc()) :
         SHOW_TRIS::Triangle();
   }
   // Draw a set of Bfaces, for debugging:
   inline GELptr show_tris(CBface_list& faces, CCOLOR& col=Color::yellow) {
      SHOW_TRISptr tris = new SHOW_TRIS();
      for (int i=0; i<faces.num(); i++) {
         tris->add(tri(faces[i]));
      }
      tris->set_fill_color(col);
      WORLD::create(tris,false); // false means not undoable
      return tris;
   }

   // show an edge:
   inline GELptr show(
      Bedge*            e,
      double            width=2,
      CCOLOR&           col=Color::blue,
      double            alpha=1,
      bool              depth_test=true
      ) {
      if (!e) return 0;
      return WORLD::show(
         e->v1()->wloc(), e->v2()->wloc(), width, col, alpha, depth_test
         );
   }

   // show a bunch of edges
   inline GELlist show(
      CBedge_list&      edges,
      double            width=2,
      CCOLOR&           col=Color::blue,
      double            alpha=1,
      bool              depth_test=true
      ) {
      GELlist ret(edges.num());
      for (int i=0; i<edges.num(); i++)
         ret += show(edges[i], width, col, alpha, depth_test);
      return ret;
   }
};

class IDVisibilityTest : public VisibilityTest {
 public:
   // does visibility test using ID image
   virtual double visibility(CWpt& p, double len);
};

#endif // GTEX_UTIL_H_IS_INCLUDED

// end of file util.H
