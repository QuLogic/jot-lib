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
/*****************************************************************
 * inflate.C
 *****************************************************************/

/*!
 *  \file inflate.C
 *  \brief Contains the definition of the INFLATE widget and INFLATE_CMD.
 *
 *  \ingroup group_FFS
 *  \sa inflate.H
 *
 */

#include "disp/colors.H"                // Color::grey7 etc.
#include "geom/gl_view.H"               // for GL_VIEW::init_line_smooth()
#include "geom/world.H"                 // for WORLD::undisplay()
#include "gtex/ref_image.H"             // for VisRefImage
#include "gtex/basic_texture.H"         // for GLStripCB
#include "mesh/mi.H"
#include "std/config.H"
#include "std/run_avg.H"

#include "tess/ti.H"
#include "tess/skin.H"
#include "tess/tess_cmd.H"
#include "tess/tess_debug.H"
#include "tess/uv_surface.H"

#include "inflate.H"

using namespace mlib;

static bool debug = Config::get_var_bool("DEBUG_INFLATE_ALL",false);

/*****************************************************************
 * INLINE
 *****************************************************************/

inline bool
is_maximal_connected(CBface_list& set)
{
   return (set.is_connected() &&
           set.get_boundary().edges().all_satisfy(BorderEdgeFilter()));
}

/*****************************************************************
 * INFLATE
 *****************************************************************/
INFLATEptr INFLATE::_instance;

INFLATEptr
INFLATE::get_instance()
{
   if (!_instance)
      _instance = new INFLATE();
   return _instance;
}

INFLATE::INFLATE() :
   DrawWidget(),
   _d(0),
   _preview_dist(0)
{
   //*******************************************************
   // FSA states for handling GESTUREs:
   //
   //   Note: the order matters. First matched will be executed.
   //*******************************************************
   _draw_start += DrawArc(new TapGuard,     drawCB(&INFLATE::tap_cb));
   _draw_start += DrawArc(new LineGuard,    drawCB(&INFLATE::line_cb));
   _draw_start += DrawArc(new StrokeGuard,  drawCB(&INFLATE::stroke_cb));

   // Set up the clean up routine
   atexit(clean_on_exit);
}

void
INFLATE::clean_on_exit()
{
   _instance = 0;
}

int
INFLATE::cancel_cb(CGESTUREptr&, DrawState*& s)
{
   err_adv(debug, "INFLATE::cancel_cb");

   // clear cached info:
   reset();

   // Turn off widget:
   deactivate();

   // Reset state to start:
   s = &_draw_start;

   return 1; // This means we DID use up the gesture
}

int
INFLATE::tap_cb(CGESTUREptr& gest, DrawState*& state)
{
   err_adv(debug, "INFLATE::tap_cb");

   // Tracks if the tap was near a guideline
   bool near_guidelines = false;

   PIXEL pdummy;
   int idummy;
   if ( PIXEL_list(_lines).closest( PIXEL(gest->start()), pdummy, idummy ) < 5 )
      near_guidelines = true;

   // Check if gesture hits a BFace
   Bface* face = 0;
   Bsurface::hit_ctrl_surface(gest->start(), 1, &face);

   // Fail if Gesture missed guidelines and geometry
   if ( !face && !near_guidelines )
      return cancel_cb(gest,state);

   // Check that we are trying to inflate
   if ( _orig_face ) {

      // Find the reachable faces from the starting point
      Bface_list set
         = _mode ? _faces : Bface_list::reachable_faces(_orig_face);

      // verify that the user tapped a face that is part of the inflation region
      if ( face && !set.contains( face ) ) {
         return cancel_cb(gest,state);
      }

      // Attempt to inflate the surface
      INFLATE_CMDptr cmd = _mode ? (new INFLATE_CMD( _faces, _preview_dist )) :
         (new INFLATE_CMD( _orig_face, _preview_dist ));
      WORLD::add_command(cmd);
   }

   // On fail, cancel
   return cancel_cb(gest,state);
}

int
INFLATE::stroke_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "INFLATE::stroke_cb");

   reset_timeout();

   // Verify that we have a starting face
   if ( _orig_face ) {
      Bface* face = 0;

      // Check that the stroke is straight enough to represent a line
      if (!(gest->straightness() > 0.8)) {
         err_adv(debug, "INFLATE::stroke_cb: gesture not straight");
         return false;
      }

      // Check that the gesture starts on the mesh
      Bsurface::hit_ctrl_surface(gest->start(), 1, &face);
      if (!(face)) {
         err_adv(debug, "INFLATE::stroke_cb: can't get hit face");
         return false;
      }

      // create VEXELs for the gesture and the face normal
      VEXEL fvec  = VEXEL(face->v1()->loc(), face->norm());
      VEXEL fgest = gest->endpt_vec();

      // If gesture nearly parallel to normal:
      double a = rad2deg(line_angle(fvec,fgest));
      err_adv(debug, "INFLATE::stroke_cb: angle: %f %s",
              a, (a > 15) ? "(bad)" : "(good)");
      if (a > 15) {
         // Fail if angle is too extreme
         WORLD::message("Bad angle");
         return 0;
      }

      // calculate extrude width
      double dist = fgest.length()/fvec.length();

      err_adv(debug, "INFLATE::stroke_cb: strong_edge_len: %f, gest_len: %f, \
fvect_len: %f", avg_strong_edge_len(face), fgest.length(), fvec.length() );

      // Convert to relative to local edge length
      dist /= avg_strong_edge_len(face);
      if (fvec*fgest<0)
         dist=-dist; // Get the sign right

      // Store the new inflate distance
      _preview_dist = dist;
   }

   return 1;    // we used up the gesture...
}

int
INFLATE::line_cb(CGESTUREptr& gest, DrawState*& s)
{
   // Activity occurred to extend the deadline for fading away:
   reset_timeout();

   err_adv(debug, "INFLATE::line_cb");

   // Verify that we have a starting face
   if ( _orig_face ) {
      // Check that the stroke is straight enough to represent a line
      Bface* face = 0;
      if (!(gest->straightness() > 0.8)) {
         err_adv(debug, "INFLATE::line_cb: gesture not straight");
         return false;
      }

      // Check that the gesture starts on the mesh
      Bsurface::hit_ctrl_surface(gest->start(), 1, &face);
      if (!(face)) {
         err_adv(debug, "INFLATE::line_cb: can't get hit face");
         return false;
      }

      // create VEXELs for the gesture and the face normal
      VEXEL fvec  = VEXEL(face->v1()->loc(), face->norm());
      VEXEL fgest = gest->endpt_vec();

      // If gesture nearly parallel to normal:
      double a = rad2deg(line_angle(fvec,fgest));
      err_adv(debug, "INFLATE::line_cb: angle: %f %s",
              a, (a > 15) ? "(bad)" : "(good)");
      if (a > 15) {
         return false;
      }

      // calculate extrude width
      double dist = fgest.length()/fvec.length();
      if (fvec*fgest<0)
         dist=-dist; // Get the sign right

      _preview_dist = dist;
   }

   // get here if nothing happened...
   // don't cancel (which would deactivate the widget),
   // just return 1 to indicate that we used up the gesture:
   return 1;
}

//! Given boundary curve (near-planar), activate the
//! widget to inflate ...
bool
INFLATE::setup(CBface_list& faces, double dist, double dur)
{
   reset();

   BMESH* m = faces.mesh();
   if (!m) {
      err_adv(debug, "INFLATE::setup: Error: null mesh");
      return 0;
   }
   if (!LMESH::isa(m)) {
      err_adv(debug, "INFLATE::setup: Error: non-LMESH");
      return 0;
   }
   if (!faces.is_consistently_oriented()) {
      err_adv(debug, "INFLATE::setup: Error: inconsistently oriented faces");
      return 0;
   }

   // We ensured the mesh in an LMESH so this is okay:
   _boundary=faces.get_boundary();

   if (_boundary.edges().empty()) {
      err_adv(debug, "INFLATE::setup: Error: No boundary. Quitting.");
      _boundary.reset();
      return 0;
   }

   // ******** From here on, we accept it ********

   err_adv(debug, "Inflating... %d boundary loops",
           _boundary.num_line_strips());

   _faces = faces;
   _mode = true;
   _d     = _boundary.cur_edges().avg_len();
   _mesh  = (LMESH*)m;
   _orig_face = faces[0];
   _preview_dist = dist;

   // Set the timeout duration
   set_timeout(dur);

   // Become the active widget and get in the world's DRAWN list:
   activate();

   return true;
}

//! Given boundary curve (near-planar), activate the
//! widget to inflate ...
bool
INFLATE::setup(Bface *bf, double dist, double dur)
{

   reset();

   // Given the starting face f and an offset amount h,
   // determine the appropriate edit level, re-map f to that
   // level, and "inflate" the portion of the mesh reachable
   // from f by h, relative to the local edge length.

   // reject garbage
   if (!(bf && LMESH::isa(bf->mesh()))) {
      err_adv(debug, "INFLATE::setup: error: bad face");
      return 0;
   }
   Lface* f = (Lface*)bf;

   err_adv(debug, "setup: face at level %d", bf->mesh()->subdiv_level());

   // get avg edge length of edges in face:
   double avg_len = avg_strong_edge_len(f);
   if (avg_len < epsAbsMath()) {
      err_adv(debug, "INFLATE::setup: bad average edge length: %f", avg_len);
      return 0;
   }


   // Get set of reachable faces:
   Bface_list set
      = Bface_list::reachable_faces(f);
   assert(set.mesh() != NULL);
   err_adv(debug, "reachable faces: %d, subdiv level: %d, total faces: %d",
           set.num(), set.mesh()->subdiv_level(), set.mesh()->nfaces());

   // given face set should be an entire connected piece.
   if (!is_maximal_connected(set
          )) {
      err_adv(debug, "INFLATE::setup: rejecting subset of surface...");
      return 0;
   }

   Bface_list p = set;//get_top_level(set);
   assert(LMESH::isa(p.mesh()));
   err_adv(debug, "top level: %d faces (out of %d)",
           p.num(), p.mesh()->nfaces());

   BMESH* m = p.mesh();
   if (!m) {
      err_adv(debug, "INFLATE::setup: Error: null mesh");
      return 0;
   }
   if (!LMESH::isa(m)) {
      err_adv(debug, "INFLATE::setup: Error: non-LMESH");
      return 0;
   }
   if (!p.is_consistently_oriented()) {
      err_adv(debug, "INFLATE::setup: Error: inconsistently oriented faces");
      return 0;
   }

   // We ensured the mesh in an LMESH so this is okay:
   _boundary=p.get_boundary();

   if (_boundary.edges().empty()) {
      err_adv(debug, "INFLATE::setup: Error: No boundary. Quitting.");
      _boundary.reset();
      return 0;
   }

   // ******** From here on, we accept it ********

   err_adv(debug, "Inflating... %d boundary loops",
           _boundary.num_line_strips());

   _faces = p;
   _mode = false;
   _d     = _boundary.cur_edges().avg_len();
   _mesh  = (LMESH*)m;
   _orig_face = bf;
   _preview_dist = dist;

   // Set the timeout duration
   set_timeout(dur);

   // Become the active widget and get in the world's DRAWN list:
   activate();

   return true;
}

void
INFLATE::reset()
{
   // Stop observing the mesh:
   if (bmesh() && bmesh()->geom())
      unobs_display(bmesh()->geom());

   // Clear cached data.
   _lines.clear();
   _orig_face = 0;
   _faces.clear();
   _boundary.reset();
   _d = 0;
   _mesh = 0;
   _preview_dist = 0;
}

void
INFLATE::build_lines()
{
   // Convenience method for building the guidelines for
   // inflating a set of faces
   Wpt_list draw_lines;
   //ARRAY<Bvert_list> boundry_loops;

   if (_faces.empty())
      return;

   // start with a clean _lines
   _lines.clear();
   draw_lines.clear();

   const EdgeStrip *source = _boundary.cur_strip();

   // Check that there is a boundary, else return
   if (source->empty()) {
      return;
   }

   // preallocate enough lines
   _lines.realloc(source->num());
   draw_lines.realloc(source->num());

   int k=0;
   for (k=0; k<source->num(); k++) {

      // Check if this is a break in the boundry
      if ( source->has_break(k) && k ) {
         // Add the endpoint
         _lines += (source->next_vert(k-1)->loc() +
                    _preview_dist*(source->next_vert(k-1)->norm()));
         draw_lines += (source->next_vert(k-1)->loc() +
                        _preview_dist*(source->next_vert(k-1)->norm()));
         // Render the current loop
         GL_VIEW::draw_wpt_list(
            draw_lines,
            Color::yellow,
            alpha(),
            3,
            true
            );

         // Start a new loop
         draw_lines.clear();

      }
      // Add the current preview point to the line set
      _lines += (source->vert(k)->loc() + _preview_dist*(source->vert(k)->norm()));
      draw_lines +=
         (source->vert(k)->loc() + _preview_dist*(source->vert(k)->norm()));
   }

   // If there is a loop pending
   if (!_lines.empty() && k) {
      _lines +=
         (Wpt)(source->next_vert(k-1)->loc() +
               _preview_dist*(source->next_vert(k-1)->norm()));
      draw_lines +=
         (Wpt)(source->next_vert(k-1)->loc() +
               _preview_dist*(source->next_vert(k-1)->norm()));
      // Draw it
      GL_VIEW::draw_wpt_list(
         draw_lines,
         Color::yellow,
         alpha(),
         3,
         true
         );
   }

   _lines.update_length();

}

int
INFLATE::draw(CVIEWptr& v)
{
   int ret=0;

   // If needed, draw the boundary curve
   if (!_boundary.empty()) {
      // antialiased, 2 pixels wide
      GL_VIEW::init_line_smooth(GLfloat(v->line_scale()*2.0), GL_CURRENT_BIT);
      glDisable(GL_LIGHTING);              // GL_ENABLE_BIT
      GL_COL(Color::orange, alpha());      // GL_CURRENT_BIT
      GLStripCB cb;
      _boundary.draw(&cb);

      // Draw the preview boundary
      build_lines();
      GL_VIEW::end_line_smooth();
   }

   // When all faded away, undraw
   if (alpha() == 0)
      WORLD::undisplay(this, false);

   return ret;
}

inline UVpt_list
uvlist(CUVpt& uv1, CUVpt& uv2)
{
   UVpt_list ret;
   ret += uv1;
   ret += uv2;
   ret.update_length();
   return ret;
}

//*******************************************************
// Inflating a surface
//*******************************************************

//! The inflate op was initiated at the given face.
//! Based on the offset amount ("dist"), decide an
//! appropriate subdivision level at which to generate
//! the inflated surface. We want the edge length of
//! the inflated surface to match the offset amount.
inline bool
choose_level(Lface* face, double dist, int& rel_level)
{

   // Get avg edge length of edges in face:
   double edge_len = avg_strong_edge_len(face);
   if (edge_len < epsAbsMath()) {
      err_adv(debug, "choose_level: bad average edge length: %f", edge_len);
      return false;
   }

   // Compute appropriate subdiv level
   // RELATIVE to level of starting face
   //
   // .5 to the power of level equals fabs(dist/edge_len)
   rel_level = (int)round(-log2(fabs(dist/edge_len)));

   if (rel_level > 0) {
      // Small offset: go to finer level.
      //
      // Ensure mesh subdivision is updated to the chosen level:
      update_subdivision(face->mesh(), subdiv_level(face) + rel_level);
      return true;
   } else if (rel_level == 0) {
      // Stick with current level.
      return true;
   }

   // Wide inflate: coarser level?
   // Choose finest level under Bbase control:
   // XXX - not trusting following line:
//   rel_level = Bbase::max_res_level_on_region(face) - subdiv_level(face);
   return true;
}

//! Given the starting face "orig_face" and an offset amount "dist",
//! determine the appropriate edit level, re-map orig_face to that
//! level, and "inflate" the portion of the mesh reachable from
//! orig_face by amount dist. The offset may be made relative to the
//! local edge length.
bool
INFLATE::do_inflate(
   Bface*               orig_face,
   double               dist,
   Bsurface*&           output,
   Bface_list*&         reversed_faces,
   MULTI_CMDptr         cmd)
{

   // Reject garbage
   if (fabs(dist) < epsAbsMath()) {
      err_adv(debug, "INFLATE::do_inflate: bad offset distance: %f", dist);
      return 0;
   }

   // Throw out trash
   if (!get_lmesh(orig_face)) {
      err_adv(debug, "INFLATE::do_inflate: error: bad face");
      return 0;
   }
   Lface* face = (Lface*)orig_face;

   err_adv(debug, "INFLATE::do_inflate: face at level %d", subdiv_level(face));

   // Decide which level to do inflation -- should match offset dist

   int rel_level = 0;
   // XXX - it may not be a good idea
   //if (!choose_level(face, dist, rel_level))    // defined above
   //   return 0;

   // Remap face to chosen level
   face = remap(face, rel_level);
   if (!face) {
      err_adv(debug, "INFLATE::do_inflate: can't remap %d from level %d",
              rel_level, subdiv_level(face));
      return 0;
   }
   assert(face && face->mesh());
   err_adv(debug, "chosen edit level: %d", subdiv_level(face));

   // Get set of reachable faces:
   Bface_list set = Bface_list::reachable_faces(face);
   assert(set.mesh() != NULL);
   err_adv(debug, "reachable faces: %d, subdiv level: %d, total faces: %d",
           set.num(), set.mesh()->subdiv_level(), set.mesh()->nfaces());

   if (!set.is_consistently_oriented()) {
      err_msg("INFLATE::do_inflate: rejecting inconsistently \
              oriented surface...");
      return 0;
   }

   // XXX -
   //   need to deal w/ cases:
   //     1. surface with boundary (sheet)
   //     2. subset of existing surface
   // for now starting with 1. and assuming
   // given face set is an entire connected piece.
   //if (set.get_boundary().empty()) {
   //   err_adv(debug, "INFLATE::do_inflate: rejecting closed surface...");
   //   return 0;
   //}
   //if (!is_maximal_connected(set)) {
   //   err_adv(debug, "INFLATE::do_inflate: rejecting subset of surface...");
   //   return 0;
   //}

   //******** BEGIN OPERATION ********

   bool ret = Skin::create_inflate(set, dist, cmd, false);
   err_adv(debug &&  ret, "created inflate via skin");
   err_adv(debug && !ret, "could not create inflate via skin");
   return ret;
}

bool
INFLATE::do_inflate(
   Bface_list           orig_faces,
   double               dist,
   Bsurface*&           output,
   Bface_list*&         reversed_faces,
   MULTI_CMDptr         cmd)
{

   // Reject garbage
   if (fabs(dist) < epsAbsMath()) {
      err_adv(debug, "INFLATE::do_inflate: bad offset distance: %f", dist);
      return 0;
   }

   // Throw out trash
   if (!get_lmesh(orig_faces)) {
      err_adv(debug, "INFLATE::do_inflate: error: bad faces");
      return 0;
   }

   Bface_list set = orig_faces;
   assert(set.mesh() != NULL);
   err_adv(debug, "reachable faces: %d, subdiv level: %d, total faces: %d",
           set.num(), set.mesh()->subdiv_level(), set.mesh()->nfaces());

   if (!set.is_consistently_oriented()) {
      err_msg("INFLATE::do_inflate: rejecting inconsistently \
              oriented surface...");
      return 0;
   }

   if (set.get_boundary().empty()) {
      err_adv(debug, "INFLATE::do_inflate: rejecting closed surface...");
      return 0;
   }

   //******** BEGIN OPERATION ********

   bool ret = Skin::create_inflate(set, dist, cmd, true);
   err_adv(debug &&  ret, "created inflate via skin");
   err_adv(debug && !ret, "could not create inflate via skin");
   return ret;
}

bool
INFLATE_CMD::doit()
{
   if (is_done())
      return true;      // all set: no-op

   if (is_undone()) {
      // It was done, then undone.
      // To redo, just show the Rsurface (and its children):
      Bsurface *current = _output;
      if (current)
         do {
            if (current->can_show())
               current->show();
            else
               cerr << "INFLATE_CMD::doit: can't show the "
                  << current->class_name() << endl;
         } while ((current = current->child_surface()));
      if ( _reversed_faces )
         _reversed_faces->reverse_faces();
      _other_cmds->doit();
   } else if (is_clear()) {
      // It was never done... so do it now:
      if ( !_mode && INFLATE::do_inflate( _orig_face, _dist, _output,
                                _reversed_faces, _other_cmds ) ) {
         WORLD::message("Inflated mesh");
      } else if ( _mode && INFLATE::do_inflate( _orig_faces, _dist, _output,
                                _reversed_faces, _other_cmds ) ) {
         WORLD::message("Inflated mesh");
      } else {
         WORLD::message("Can't inflate mesh");
         return false;
      }
   }
   return COMMAND::doit();      // update state in COMMAND
}

bool
INFLATE_CMD::undoit()
{
   if (!is_done())
      return true;      // all set: no-op

   if ( _output ) {
      Bsurface *current = _output;
      do {
         if (current->can_hide())
            current->hide();
         else
            cerr << "INFLATE_CMD::doit: can't hide the "
                 << current->class_name() << endl;
      } while ((current = current->child_surface()));
      if ( _reversed_faces )
         _reversed_faces->reverse_faces();
   }
   _other_cmds->undoit();
   return COMMAND::undoit();    // update state in COMMAND
}

bool
INFLATE_CMD::clear()
{
   if (is_clear())
      return true;
   if (is_done())
      return false;

   if (!COMMAND::clear())       // update state in COMMAND
      return false;

   _other_cmds->clear();

   if ( _output ) {
      _output->delete_elements();
      delete _output;
      _output = 0;
   }
   if ( _reversed_faces )
      delete _reversed_faces;
   return true;
}

// end of file inflate.C
