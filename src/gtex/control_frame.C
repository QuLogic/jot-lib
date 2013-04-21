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
 * control_frame.C
 **********************************************************************/
#include "geom/gl_view.H"
#include "mesh/lpatch.H"
#include "mesh/mi.H"
#include "std/config.H"
#include "control_frame.H"

static bool debug = Config::get_var_bool("DEBUG_CONTROL_FRAME",false,true);

static const double SELECTED_SIMPLEX_ALPHA =
Config::get_var_dbl("SELECTED_SIMPLEX_ALPHA",0.5);

static const GLfloat SELECTED_EDGE_WIDTH = (GLfloat)
Config::get_var_dbl("SELECTED_EDGE_WIDTH",4);

// XXX - temporary, to determine good values:
static CCOLOR orange_pencil_l (255.0/255, 204.0/255, 51.0/255);
static CCOLOR orange_pencil_m (255.0/255, 153.0/255, 11.0/255);
static CCOLOR orange_pencil_d (243.0/255, 102.0/255, 0.0/255);

int
ControlFrameTexture::draw(CVIEWptr& v)
{
   if (_ctrl)
      return _ctrl->draw(v);

   // Create a strip for the given mesh type (BMESH or LMESH):
   assert(_patch && _patch->mesh());
   if (_strip)
      _strip->reset();
   else
      _strip = _patch->mesh()->new_edge_strip();

   if (!BasicTexture::draw(v)) {
     int dl = 0;
     if ((dl = _dl.get_dl(v, 1, _patch->stamp())))
       glNewList(dl, GL_COMPILE);

     // Draw control curves down to the current "edit level"
     int el = max(_patch->rel_edit_level(), 0);

     // Draw each level:
     for (int k = 0; k <= el; k++)
       draw_level(v, k);

      // end the display list here
      if (_dl.dl(v)) {
         _dl.close_dl(v);

         // the display list is built; now execute it
         BasicTexture::draw(v);
      }
   }
   draw_selected_faces();
   draw_selected_edges();
   draw_selected_verts();

   return _patch->num_faces();
}

void
ControlFrameTexture::draw_level(CVIEWptr& v, int k)
{
   // Draw the control curves for the Patch at level k,
   // which is relative to the control Patch.

   // Get the level-k edge strip:
   if (!build_strip(k)) {
      err_adv(debug, "ControlFrameTexture::draw_level: build_strip failed");
      return;
   }

   assert(_strip != NULL);

   // Set line thickness, color and alpha

   // Default top-level line thickness is 1.0:
   static double top_w = Config::get_var_dbl("CTRL_FRAME_TOP_THICKNESS", 2.0,true);
   // Get a scale factor s = r^k used to control line width.
   double r = Config::get_var_dbl("CONTROL_FRAME_RATIO", 0.6,true);
   assert(r > 0 && r < 1);
   GLfloat w = GLfloat( top_w * pow(r, k + mesh()->subdiv_level()));

   // 'a' is opacity (alpha) determined by GL_VIEW::init_line_smooth()
   // needed to simulate line widths below the minimum supported value:
   GLfloat a = 1;
   // Init line smoothing, set width, and push attributes:
   GL_VIEW::init_line_smooth(v->line_scale()*w, GL_CURRENT_BIT);//, &a);
   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT
   GL_COL(_color, a*alpha());   // GL_CURRENT_BIT

   err_adv(debug, "ControlFrameTexture::draw_level: level %d, width %f", k, w);

   _strip->draw(_cb);

   // Restore gl state:
   GL_VIEW::end_line_smooth();
}

inline Patch*
get_sub_patch(Patch* p, int k)
{
   return (k == 0) ? p : Lpatch::isa(p) ? ((Lpatch*)p)->sub_patch(k) : 0;
}

bool
ControlFrameTexture::build_strip(int k)
{
   if (!(_patch && _patch->mesh())) {
      err_adv(debug, "ControlFrameTexture::build_strip: no patch/mesh");
      return 0;
   }

   err_adv(debug, "ctrl frame: level %d/%d", k, mesh()->rel_edit_level());

   if (k < 0)
      return false;

   // Build edge strips from edges of level k Patch

   // Make sure we get the sub-patch at level k
   // (relative to our patch, i.e. the control patch):
   assert(_patch->ctrl_patch() == _patch);
   Patch* sub = get_sub_patch(_patch, k);
   if (!(sub && sub->rel_subdiv_level() == k)) {
      if (debug) {
         if (sub)
            err_msg( "ControlFrameTexture::draw_level: %s %d != %d",
                    "got patch level", sub->rel_subdiv_level(), k);
         else
            err_msg( "ControlFrameTexture::draw_level: can't get sub-patch");
      }
      return false;
   }
   Bedge_list edges = sub->edges();

   // Clear all edge flags
   edges.clear_flags();

   // If secondary edges shouldn't be drawn, set their flags
   // so they won't be drawn:
   if (!BMESH::show_secondary_faces())
      edges.secondary_edges().set_flags(1);

   // But now set flags for children of edges of the parent mesh.
   // (We don't draw these since they are drawn at some level < k):
   if (LMESH::isa(_patch->mesh())) {
      edges.filter(EdgeChildFilter()).set_flags(1);
   }

   // Get an edge strip to use
   if (_strip)
      _strip->reset();
   else
      _strip = mesh()->new_edge_strip();

   // Construct filter that accepts unreached strong edges of
   // the sub-patch
   UnreachedSimplexFilter    unreached;
   StrongEdgeFilter          strong;
   PatchEdgeFilter           mine(sub);
   _strip->build(edges, unreached + strong + mine);

   return !_strip->empty();
}
  
/**********************************************************************
 * drawing selected elements
 **********************************************************************/
inline void
get_cur_sub_faces(Patch* p, Bface* f, Bface_list& ret)
{
  if (!(p && f &&  f->patch()->ctrl_patch() == p) )
    return;
 
  if (LMESH::isa(f->mesh())) {
     int lev = f->mesh()->rel_cur_level();
     if (lev >= 0)
        ((Lface*)f)->append_subdiv_faces(f->mesh()->rel_cur_level(), ret);
  } else {
    ret += f;
  }
}

inline void 
draw_face(Bface* f)
{
   if (!f) return;
//   glNormal3dv(f->norm().data());
   glVertex3dv(f->v1()->loc().data());
   glVertex3dv(f->v2()->loc().data());
   glVertex3dv(f->v3()->loc().data());
}

void
ControlFrameTexture::draw_selected_faces()
{
   // Start with ALL selected faces, from every mesh:
   CBface_list& sel_faces = MeshGlobal::selected_faces();
                                                                                
   // Remap to current subdiv level
   // keeping only those from our patch:
   Bface_list sub_faces;
   int i=0; // loop index
   for (i=0; i<sel_faces.num(); i++) {
     get_cur_sub_faces(patch(), sel_faces[i], sub_faces);
   }
   if (sub_faces.empty())
     return;

   glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
   glDisable(GL_CULL_FACE);                             // GL_ENABLE_BIT
   glDisable(GL_LIGHTING);                              // GL_ENABLE_BIT
   double a = SELECTED_SIMPLEX_ALPHA*alpha();
   if (a < 1) {
      glEnable(GL_BLEND);                               // GL_COLOR_BUFFER_BIT
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);// GL_COLOR_BUFFER_BIT
   } else {
      glDisable(GL_BLEND);                              // GL_COLOR_BUFFER_BIT
   }
   GL_COL(orange_pencil_l, a);                          // GL_CURRENT_BIT
   glBegin(GL_TRIANGLES);
   for (i=0; i<sub_faces.num(); i++) {
     draw_face(sub_faces[i]);
   }
   glEnd();
   glPopAttrib();
}

inline void
get_cur_sub_edges(Patch* p, Bedge* e, Bedge_list& ret)
{
  if ( !(p && e && e->patch()->ctrl_patch() == p) )
    return;

  if (LMESH::isa(e->mesh())) {
     int lev = e->mesh()->rel_cur_level();
     if (lev >= 0)
        ((Ledge*)e)->append_subdiv_edges(e->mesh()->rel_cur_level(), ret);
  } else {
    ret += e;
  }
}

inline void
draw_edge(Bedge* e)
{
   if (!e) return;
   glVertex3dv(e->v1()->loc().data());
   glVertex3dv(e->v2()->loc().data());
}

void
ControlFrameTexture::draw_selected_edges()
{
   // See comments in ControlFrameTexture::draw_selected_faces()
   // for additional verbiage.

   CBedge_list& sel_edges = MeshGlobal::selected_edges(); 

   // draw the selected edges at the current subdivision level
   Bedge_list sub_edges;

   int i=0; // loop index
   for (i=0; i<sel_edges.num(); i++) {
     get_cur_sub_edges(patch(), sel_edges[i], sub_edges);
   }

   if (sub_edges.empty())
     return;

   err_adv(false, "ControlFrameTexture::draw_selected_edges: drawing %d edges",
           sub_edges.num());

   GL_VIEW::init_line_smooth(SELECTED_EDGE_WIDTH, GL_CURRENT_BIT);
   glDisable(GL_LIGHTING);                              // GL_LIGHTING_BIT
   double a = SELECTED_SIMPLEX_ALPHA*alpha();
   GL_COL(Color::red_pencil, a);                        // GL_CURRENT_BIT
   glBegin(GL_LINES);
   for (i=0; i<sub_edges.num(); i++) {
     draw_edge(sub_edges[i]);
   }
   glEnd();
   GL_VIEW::end_line_smooth();
}

inline Patch* 
get_patch(CBvert* v)
{
   // Arbitrarily chose a face containing v and return its patch

   if (!v) return 0;
   Bface* f = v->get_face();
   if (!f) return 0;
   return f->patch();
}

inline void
get_cur_sub_vert(Patch* p, Bvert* v, Bvert_list& ret)
{
   Patch* q = get_patch(v);
   if ( !(p && q && q->ctrl_patch() == p) )
      return;

   if (LMESH::isa(v->mesh())) {
      int lev = v->mesh()->rel_cur_level();
      if (lev >= 0) {
         Lvert* u = ((Lvert*)v)->subdiv_vert(lev);
         if (u)
            ret += u;
      }
   } else {
      ret += v;
   }
}

void
ControlFrameTexture::draw_selected_verts()
{
   // See comments in ControlFrameTexture::draw_selected_faces()
   // for additional verbiage.

   CBvert_list& sel_verts = MeshGlobal::selected_verts(); 

   // draw the selected verts at the current subdivision level
   Bvert_list sub_verts;

   int i=0; // loop index
   for (i=0; i<sel_verts.num(); i++) {
     get_cur_sub_vert(patch(), sel_verts[i], sub_verts);
   }

   if (sub_verts.empty())
     return;

   // Draw the verts color tan 8 pixels wide
   double a = SELECTED_SIMPLEX_ALPHA*alpha();
   GL_VIEW::draw_pts(sub_verts.pts(), Color::tan, a, 8.0);
}

// end of file control_frame.C
