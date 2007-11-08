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
 * primitive.C:
 **********************************************************************/

/*!
 *  \file primitive.C
 *  \brief Contains the definition of the Primitive class.
 *
 *  Contains the Primitive class, as well as the static methods
 *  for creating new Primitives.
 *  \sa primitive.H
 *
 */

#include "geom/world.H"
#include "mesh/edge_frame.H"
#include "mesh/mi.H"
#include "std/config.H"

#include "tess/action.H"
#include "tess/skel_frame.H"
#include "tess/skin.H"
#include "tess/panel.H"
#include "tess/tex_body.H"
#include "tess/ti.H"
#include "tess/xf_meme.H"
#include "tess/primitive.H"

using namespace mlib;
using namespace tess;

static bool debug = Config::get_var_bool("DEBUG_PRIMITIVE",false);

/*****************************************************************
 * SHOW_COORD_FRAME
 *****************************************************************/
class SHOW_COORD_FRAME : public GEL {
 public:

   //******** MANAGERS ********

   SHOW_COORD_FRAME(CoordFrame* f) : _frame(f) {}

   void set_frame(CoordFrame* f) { _frame = f; }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("SHOW_COORD_FRAME", SHOW_COORD_FRAME*, GEL, CDATA_ITEM *);

   //******** GEL VIRTUAL METHODS ********

   virtual bool needs_blend()    const { return true; }
   virtual int  draw(CVIEWptr &);

   //******** DATA_ITEM VIRTUAL METHODS ********

   virtual DATA_ITEM* dup() const { return 0; }

 protected:
   CoordFrame*  _frame;
};

int
SHOW_COORD_FRAME::draw(CVIEWptr& v) 
{
   if (!_frame)
      return 0;

//    MeshGlobal::select(((DiskMap*)_frame)->faces());

   Wpt o = _frame->o();

   const double PIX_LEN = 30;
   const double len     = world_length(o, PIX_LEN);
   const GLfloat width    = 2;

//    Wpt t = _frame->xf() * Wpt(len,0,0);
//    Wpt b = _frame->xf() * Wpt(0,len,0);
//    Wpt n = _frame->xf() * Wpt(0,0,len);

   Wpt t = o + _frame->t() * len;
   Wpt b = o + _frame->b() * len;
   Wpt n = o + _frame->n() * len;

   GL_VIEW::init_line_smooth(width, GL_CURRENT_BIT);
   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT

   glBegin(GL_LINES);

   // draw t, b, n in red, yellow, blue

   GL_COL(Color::red, 1);
   glVertex3dv(o.data());
   glVertex3dv(t.data());

   GL_COL(Color::yellow, 1);
   glVertex3dv(o.data());
   glVertex3dv(b.data());

   GL_COL(Color::blue, 1);
   glVertex3dv(o.data());
   glVertex3dv(n.data());

   glEnd();

   GL_VIEW::end_line_smooth();
   return 0;
}

//! Returns the length in PIXELs of the screen-space
//! projection of a line segment of length 'len', which
//! has one endpoint lying along the camera's line of
//! sight, at a depth determined by input point 'c', and
//! which is parallel to the "film plane."
inline double
screen_rad(CWpt& c, double len)
{

   // Direction to offset 2nd endpoint
   // (parallel to film plane):
   Wvec X = VIEW::peek_cam()->data()->right_v();

   PIXEL a = PIXEL(XYpt());                 // 1st endpoint (center of window)
   PIXEL b = PIXEL(Wpt(XYpt(), c) + X*len); // 2nd endpoint

   return a.dist(b);
}

//! Given Wpt_lists or PIXEL_lists (or similar types) A and B,
//! find the projection of B onto A in the form of a "span";
//! i..e. a sub-portion of A defined by starting and ending
//! parameter values s0 and s1. Also return the "average error",
//! which is the average distance from each sample on B to its
//! projection on A.
inline bool
get_projected_span(PIXEL_list A, PIXEL_list B, double& s0, double& s1,
                   double& avg_err)
{

   // Note: with minor changes, this could be templated to work
   // with Wpt_lists and other polyline types. In that case the
   // "step_size" parameter (see below) should be passed in as
   // a variable.

   if (A.num() < 2 || B.num() < 2) {
      err_adv(debug, "get_projected_span: incomplete inputs");
      return false;
   }
   if (A.is_closed() || B.is_closed()) {
      // We know HOW to do it, but who has time?
      err_adv(debug, "get_projected_span: closed inputs: not implemented");
      return false;
   }
   A.update_length();
   B.update_length();
   double step_size = 2.0;
   if (A.length() < step_size || B.length() < step_size) {
      err_adv(debug, "get_projected_span: inputs too short");
      return false;
   }
   // Step along B in increments of the given step size.
   // At each point, find the parameter of the projected
   // point on A. Record the min and max parameter values
   // encountered, and name them s0 and s1 respectively.

   // The number of *samples* is num_steps + 1:
   int num_steps = (int)round(B.length()/step_size);
   assert(num_steps >= 1);
   double ds = B.length()/num_steps;
   PIXEL b = B.interpolate(0);
   s0 = s1 = A.invert(b);
   PIXEL a = A.interpolate(s0);
   RunningAvg<double> err(0);
   err.add(a.dist(b));
   for (int i=1; i<=num_steps; i++) {
      b = B.interpolate(ds*i);
      double s = A.invert(b);
      a = A.interpolate(s);
      err.add(a.dist(b));
      s0 = min(s0, s);
      s1 = max(s1, s);
   }
   avg_err = err.val();
   return true;
}

//! Return the avg distance from B to its projection on A
inline bool
get_distance(CPIXEL_list& A, CPIXEL_list& B, double& ret)
{

   double s0, s1;
   return get_projected_span(A, B, s0, s1, ret);
}

/*****************************************************************
 * Primitive
 *****************************************************************/
//! Create an empty Primitive associated with the given
//! surface LMESH and skeleton LMESH.
Primitive::Primitive(LMESH* mesh, LMESH* skel)
{

   // Ensure preconditions
   assert(mesh && skel && TEXBODY::isa(mesh->geom()));

   // Record the mesh
   set_mesh(mesh);

   // Record the skeleton mesh
   _skel_mesh = skel;
   if (skel != mesh) _skel_mesh->disable_draw();
}

Primitive::~Primitive()
{
   destructor();
}

//! Returns true if the given screen point p is considered
//! relatively near the base:
inline bool 
near_base_center(CBface_list& base, CPIXEL& p)
{

   // Reject if empty:
   if (base.empty())
      return false;

   // Reject if base centroid is not in frustum:
   Wpt      c = base.get_verts().center();
   Wtransf pm = base.mesh() ? base.mesh()->obj_to_ndc() : Identity;
   if (!NDCZpt(c, pm).in_frustum())
      return false;

   // Accept if given point is within a radius proportional
   // to the size of the quad:
   return p.dist(PIXEL(c)) <= screen_rad(c, base.boundary_edges().min_edge_len()/2);
}

inline Bface_list
create_polygon(CWpt_list& pts, Patch* p)
{
   assert(p && p->mesh());
   assert(pts.num() == 3 || pts.num() == 4);
   Bvert_list verts = p->mesh()->add_verts(pts);
   if (pts.num() == 3)
      return Bface_list(p->mesh()->add_face(verts[0], verts[1], verts[2], p));
   return Bface_list(
      p->mesh()->add_quad(
         verts[0],  verts[1],  verts[2],  verts[3],
         UVpt(0,0), UVpt(1,0), UVpt(1,1), UVpt(0,1), p
         )
      );
}

inline bool 
copy_edge(Bedge* a, CVertMapper& vmap)
{
   Bedge* b = vmap.a_to_b(a);
   if (!(a && b))
      return false;

   if (a->is_weak())
      b->set_bit(Bedge::WEAK_BIT);

   // more?

   return true;
}

inline bool 
copy_edges(CBedge_list& edges, CVertMapper& vmap)
{
   bool ret = true;
   for (int i=0; i<edges.num(); i++)
      if (!copy_edge(edges[i], vmap))
         ret = false;
   return ret;
}

inline Bface*
gen_flip_face(Bface* f, CVertMapper& vmap, Lpatch* p=0)
{
   assert(f && f->mesh() && f->patch());
   Bvert* v1 = vmap.a_to_b(f->v1());
   Bvert* v2 = vmap.a_to_b(f->v2());
   Bvert* v3 = vmap.a_to_b(f->v3());
   assert(v1 && v2 && v3);
   UVpt a, b, c;
   if (UVdata::get_uvs(f, a, b, c))
      return f->mesh()->add_face(v1, v3, v2, a, c, b, p ? p : f->patch());
   return f->mesh()->add_face(v1, v3, v2, p? p : f->patch());
}

inline Bface_list
gen_flip_side(CBface_list& top, Patch* p, VertMapper& vmap)
{
   static bool debug = ::debug || Config::get_var_bool("DEBUG_PAPER_DOLL",false);
   assert(p && p->mesh());
   Bvert_list A = top.get_verts();
   Bvert_list B = p->mesh()->add_verts(A.pts());
   err_adv(debug, "gen_flip_side: %d/%d verts", A.num(), B.num());
   vmap.set(A,B);
   Bface_list ret;
   for (int i=0; i<top.num(); i++)
      ret += gen_flip_face(top[i], vmap);
   copy_edges(top.get_edges(), vmap);
   err_adv(debug, "gen_flip_side: %d faces to %d faces", top.num(), ret.num());
   return ret;
}

inline void
displace(CBvert_list& verts, CARRAY<Wvec>& delt)
{
   for (int i=0; i<verts.num(); i++)
      verts[i]->offset_loc(delt[i]);
}

inline void
displace(CBvert_list& verts, CWvec&n)
{
   ARRAY<Wvec> delt(verts.num());
   for (int i=0; i<verts.num(); i++)
      delt += (n * (verts[i]->avg_strong_len()/2));
   displace(verts, delt);
}

inline Bface_list
gen_ribbon(CEdgeStrip& boundary, CVertMapper& vmap, Patch* p)
{
   BMESH* mesh = boundary.mesh();
   assert(mesh && p);

   // du is a step in UV-space corresponding to 1 edge length,
   // based on existing UV-coords in surrounding mesh region.
   double du = min_uv_delt(boundary.edges());
   double u = 0;
   Bface_list ret(boundary.num());
   for (int i=0; i<boundary.num(); i++) {
      
      //         --- CCW -->
      //    a0 ------------- a1      du                         
      //     |               |       ^                         
      //     |     make      |       |                          
      //     |     this      |       | v                        
      //     |     quad      |       |                          
      //     |               |       |                         
      //    b0 ------------- b1      0                          
      //         --- CW -->
      //     u ------------> u+du

      if (boundary.has_break(i)) {
         u = 0; // restart u-coordinate
      }
      Bvert* a0 = boundary.vert(i);
      Bvert* a1 = boundary.next_vert(i); 
      Bvert* b0 = vmap.a_to_b(a0);
      Bvert* b1 = vmap.a_to_b(a1);

      assert(b0 && b1 && a0 && a1);
      UVpt ub0(u   ,  0);
      UVpt ub1(u+du,  0);
      UVpt ua0(u   , du);
      UVpt ua1(u+du, du);
      ret += mesh->add_quad(b0, b1, a1, a0, ub0, ub1, ua1, ua0, p);
      u += du;
   }

   return ret.quad_complete_faces();
}

bool
Primitive::create_wafer(CWpt_list& pts, Bpoint* skel)
{
   assert(skel);
   assert(mesh() && patch());

   SimplexFrame* f = new BpointFrame((uint)this, skel);

   // create top verts and faces
   VertMapper vmap;
   Bface_list top = create_polygon(pts, patch());
   Bface_list bot = gen_flip_side (top, patch(), vmap);
   add_face_memes(top + bot);

   // add normal displacement
   displace(vmap.A(),  f->n());
   displace(vmap.B(), -f->n());

   // activate xf memes:
   create_xf_memes(vmap.A(), f);
   create_xf_memes(vmap.B(), f);

   // create ribbons
   add_face_memes(gen_ribbon(top.get_boundary(), vmap, patch()));

   mesh()->changed();

   return true;
}

Primitive* 
Primitive::init(LMESH* skel_mesh, Wpt_list pts, CWvec& n, MULTI_CMDptr cmd)
{
   bool debug = ::debug || Config::get_var_bool("DEBUG_PRIMITIVE_INIT",false);
   err_adv(debug, "Primitive::init");

   assert(skel_mesh);
   assert(cmd);
   assert(pts.is_closed());
   assert(pts.is_planar());
   assert(pts.first().is_equal(pts.last()));

   pts.pop();
   pts.update_length();
   assert(pts.num() == 3 || pts.num() == 4);

   Wpt  o = pts.average();
   Wvec t = (pts[2] - pts[1]).normalized();
   int R = 0;
   Bpoint* skel = BpointAction::create(skel_mesh, o, n, t, R, cmd);
   LMESHptr mesh = skel->get_inflate_mesh();
   assert(mesh);
   Primitive* ret = new Primitive(mesh, skel->mesh());
   assert(ret);
   if (ret->create_wafer(pts, skel)) {
      show_surface(ret,cmd);
      return ret;
   }
   assert(0);
   return 0;
}

//! If conditions are favorable, create a ball primitive with given
//! screen-space radius (in PIXELs) around an isolated skeleton point:
Primitive*
Primitive::create_ball(Bpoint* skel, double pix_rad, MULTI_CMDptr cmd)
{
   bool debug = ::debug ||
      Config::get_var_bool("DEBUG_PRIMITIVE_CREATE_BALL",false);

   // Check out this so-called "skeleton"
   if (!(skel && skel->mesh())) {
      err_adv(debug, "Primitive::create_ball: bad skeleton/mesh");
      return false;
   } else if (!(skel->vert() && skel->vert() && skel->vert()->degree() == 0)) {
      err_adv(debug, "Primitive::create_ball: non-isolated  skeleton");
      return false;
   } else if (!skel->ndc().in_frustum()) {
      err_adv(debug, "Primitive::create_ball: skeleton outside frustum");
      return false;
   }

   LMESHptr mesh = skel->get_inflate_mesh();
   if (!mesh) {
      err_adv(debug, "Primitive::create_ball: can't get inflate mesh");
      return 0;
   }

   Primitive* ret = new Primitive(mesh, skel->mesh());
   if (!ret) {
      err_ret("Primitive::create_ball: can't allocate Primitive");
      return 0;
   }

   if (!ret->build_ball(skel, pix_rad)) {
      assert(0); // the plan cannot fail
      delete ret;
      return 0;
   }

   // Succeeded
   if (cmd) {
      show_surface(ret,cmd);
   }

   return ret;
}

void
init_name(Bbase* b, Cstr_ptr& name)
{
   if (b && b->name() == NULL_STR)
      b->set_name(name);
}

bool
Primitive::build_ball(Bpoint* skel, double pix_rad)
{
   // Called by create_ball().

   bool debug = ::debug ||
      Config::get_var_bool("DEBUG_PRIMITIVE_CREATE_BALL",false);

   // These are all screened in the public method create_ball():
   assert(skel && skel->mesh() && skel->mesh() == _skel_mesh);
   assert(skel->vert() && skel->vert() && skel->vert()->degree() == 0);
   assert(_mesh && is_control() && _patch && bfaces().empty());

   static int ball_num=0;
   set_name(str_ptr("ball_") + str_ptr(++ball_num));
   init_name(skel, name() + "_skel_point");
   absorb_skel(skel);

   VertFrame* frame = new VertFrame(
      (uint)this,
      skel->vert(),
      skel->frame().X(),
      skel->frame().Y()
      );

   // World-space radius of ball
   double R = world_length(skel->loc(), pix_rad);

   err_adv(debug, "pixel radius: %f, world length: %f", pix_rad, R);

   // Correct for expected shrinkage in subdivision?
   double rscale = Config::get_var_dbl("PRIMITIVE_BALL_RSCALE",0.8,true);
   R *= rscale;

   // Create the "ball"
   _mesh->Cube(Wpt(-R,-R,-R), Wpt(R,R,R), _patch); // set up in local coords
   MOD::tick();
   _mesh->transform(frame->xf(), MOD());           // rotate/translate the mesh

   // Find the our verts and create memes on them
   Bvert_list verts = _patch->verts();
   assert(verts.num() == 8);
   for (int i=0; i<verts.num(); i++)
      create_xf_meme((Lvert*)verts[i], frame);

   add_face_memes(_mesh->faces());

   // where is the command??
   finish_build(0);

   if (debug) {
      cerr << "Primitive::build_ball: inputs for " << identifier() << ":"
           << endl << "  "; inputs().print_identifiers();
   }

   return true;
}

//! Return a Primitive to manage a portion of the given mesh.
Primitive* 
Primitive::get_primitive(BMESH* mesh)
{
   //
   // We need to find the "skeleton" mesh associated w/ this mesh,
   // then create a new (blank) Primitive instance.

   static bool debug = ::debug ||
      Config::get_var_bool("DEBUG_GET_PRIMITIVE",false);

   // Ensure it is an LMESH:
   if (!LMESH::isa(mesh)) {
      err_adv(debug, "Primitive::get_primitive: error: non LMESH");
      return 0;
   }

   // To find the skeleton we first need the TEXBODY owning the mesh.
   TEXBODY* tex = TEXBODY::upcast(mesh->geom());
   if (!tex) {
      err_adv(debug, "Primitive::get_primitive: error: non TEXBODY");
      return 0;
   }

   // Check all the meshes in the TEXBODY looking for one to use
   // as the skeleton mesh:
   LMESH* skel = 0;
   CBMESH_list& meshes = tex->meshes();
   for (int i=0; i<meshes.num() && skel==0; i++) {
      if (meshes[i] != mesh &&
         !meshes[i]->is_surface() &&
          (skel = LMESH::upcast(meshes[i]))) {
         err_adv(debug, "Primitive::get_primitive: found existing skel mesh");
         break;
      }
   }
  
   if (!skel) {
      // Didn't find one, create a new one.
      // Maintain the policy that skeleton meshes come first,
      // because TEXBODY::cur_rep() selects the last one as
      // the "primary" mesh.
      skel = new LMESH;
      tex->push(skel);  // add the skel before the surface mesh
      err_adv(debug, "Primitive::get_primitive: created new skel mesh");
   }

   // Create and return the Primitive
   return new Primitive((LMESH*)mesh, skel);
}

//! Return the existing skeleton point that is nearest to
//! the given screen point p, and within radius r:
Bpoint*
Primitive::find_skel_point(CPIXEL& p, double rad) const 
{
   Bpoint* ret = 0;
   VIEWptr v = VIEW::peek();
   double min_dist = 0, d;
   for (int k=0; k<_skel_points.num(); k++) {
      NDCZpt ndc = _skel_points[k]->ndc();
      if (ndc.in_frustum() &&
          ((d = p.dist(PIXEL(ndc))) < rad) &&
          (!ret || d < min_dist)) {
         ret = _skel_points[k];
         min_dist = d;
      }
   }
   return ret;
}

//! Return the existing skeleton curve that most closely
//! matches the given screen-space curve, provided the average
//! distance between them does not exceed the given error
//! threshold.  The distance is measured between the given
//! curve and its "projection" on the skeleton curve.
Bcurve* 
Primitive::find_skel_curve(CPIXEL_list& crv, double err_thresh) const
{

   bool debug = ::debug ||
      Config::get_var_bool("DEBUG_PRIMITIVE_FIND_SKEL_CURVE",false);
   err_adv(debug, "Primitive::find_skel_curve: err thresh %f", err_thresh);

   Bcurve* ret = 0;
   double min_err = 0, d;
   for (int k=0; k<_skel_curves.num(); k++) {
      PIXEL_list skel;
      if (!_skel_curves[k]->cur_pixel_trail(skel)) {
         err_adv(debug, "Primitive::find_skel_curve: can't get pixel trail %d", k);
         continue;
      }
      if (!get_distance(skel, crv, d)) {
         err_adv(debug, "Primitive::find_skel_curve: get_distance failed");
         continue;
      }
      err_adv(debug, "distance: %f, threshold: %f", d, err_thresh);
      if (d < err_thresh && (!ret || d < min_err)) {
         ret = _skel_curves[k];
         min_err = d;
      }
   }
   if (ret)
      err_adv(debug, "Primitive::find_skel_curve: succeeded. Avg err: %f",
              min_err);
   return ret;
}

bool 
Primitive::is_ball() const 
{
   return _skel_curves.empty() && _skel_points.num() == 1; 
}

bool 
Primitive::is_tube() const 
{
   return _skel_curves.num() == 1 && _skel_points.num() == 2; 
}

bool
Primitive::is_roof() const
{
   return _skel_curves.num() == 0 && _skel_points.num() == 0;
}

//! to create a roof based on input parameters
Primitive*
Primitive::build_roof(
   PIXEL_list pixels,   //!< pixel trail we're working with (passed by copy)
   ARRAY<int> corners,  //!< should produce creases at corners (passed by copy)
   CWvec& n,            //!< normal of plane to project into
   CBface_list& bases,  //!< rectangular base of the roof
   CEdgeStrip& side,    //!< one side of the rect base
   MULTI_CMDptr cmd     //!< command list for undo/redo
   )
{
   static bool debug = ::debug ||
      Config::get_var_bool("DEBUG_ROOF",false);

   if (!cmd) {
      err_adv(debug, "Primitive::build_roof: multi-command is NULL");
      return 0;
   }

   // Need > 1 point:
   if (pixels.num() < 2) {
      err_adv(debug, "Primitive::build_roof: too few pixels");
      return 0;
   }

   // Shouldn't happen:
   if (bases.empty() || side.empty()) {
      err_adv(debug, "Primitive::build_roof: no base or side");
      return 0;
   }

   // The stroke must have point(s) that are near the end of the strip
   //
   // Stroke must start near the start of the strip:
   if (!(pixels[0].dist(side.first()->wloc()) < 10)) {
      pixels.reverse(); // This is why 'pixels' is passed by copy
      for (int i = 0; i < corners.num(); i++)
         corners[i] = pixels.num() - 1 - corners[i];
   }

   if (!(pixels[0].dist(side.first()->wloc()) < 10)) {
      err_adv(debug, "Primitive::build_roof: stroke too far from the side");
      return 0;
   }
   // stroke must end near the end of the strip:
   if (!(pixels.last().dist(side.last()->wloc()) < 10)) {
      err_adv(debug, "Primitive::build_roof: stroke too far from the side");
      return 0;
   }

   // Create a Primitive
   LMESH* m = (LMESH*)(bases.mesh());
   Primitive* ret = new Primitive(m, m);
   if (!(ret && ret->_skel_mesh)) {
      err_adv(debug, "Primitive::extend_branch: no %s found",
              ret ? "skeleton" : "Primitive");
      delete ret;
      return 0;
   }

   // Looks good -- hand off to the Primitive
   if (ret->extend(pixels, corners, bases, side, n, cmd))
      return ret;

   delete ret;
   return 0;
}

//! Define a new branch of the Primitive from the given pixel
//! trail, starting at base1 and ending at base2. The screen
//! points will be projected to 3D in the plane defined by the
//! given normal vector n, and containing the centroid of the
//! first base, and possibly the second base, if it is not null.
Primitive*
Primitive::extend_branch(
   PIXEL_list pixels,   //!< pixel trail we're working with (passed by copy)
   CWvec& n,            //!< normal of plane to project into
   Bface_list base1,        //!< first attachment base
   Bface_list base2,        //!< optional 2nd attachment base
   MULTI_CMDptr cmd     //!< command list for undo/redo
   )
{

   static bool debug = ::debug ||
      Config::get_var_bool("DEBUG_EXTENDER",false);

   if (!cmd) {
      err_adv(debug, "Primitive::extend_branch: multi-command is NULL");
      return 0;
   }

   // Need > 1 point:
   if (pixels.num() < 2) {
      err_adv(debug, "Primitive::extend_branch: too few pixels");
      return 0;
   }

   // Shouldn't happen:
   if (base1.empty()) {
      err_adv(debug, "Primitive::extend_branch: no base");
      return 0;
   }

   // Check base 2
   if (!base2.empty()) {
      if (!(base2.mesh() == base1.mesh() &&           // same mesh as base1
            (!base2.contains_any(base1)) &&  // distinct from base1
            base2.boundary_edges().num() == base1.boundary_edges().num())) { // matching bases
         err_adv(debug, "Primitive::extend_branch: bad base 2");
         return 0;
      }
   }

   // The stroke must have point(s) that are near base(s)
   // If one base selected, it must start at that base
   // If two bases, it must start at one and end at the other.
   //
   // Stroke must start at center of first base:
   if (!near_base_center(base1, pixels[0]))
      pixels.reverse(); // This is why 'pixels' is passed by copy

   if (!near_base_center(base1, pixels[0])) {
      err_adv(debug, "Primitive::extend_branch: stroke too far from base");
      return 0;
   }
   // If second base is not empty, stroke must end there:
   if (!base2.empty() && !near_base_center(base2, pixels.last())) {
      err_adv(debug, "Primitive::extend_branch: stroke too far from base");
      return 0;
   }

   // get best fit planes
   Wplane P1, P2;
   if (!base1.get_boundary().verts().pts().get_plane(P1, 0.1)) {
      err_adv(debug, "Primitive::extend_branch: base1 not planar enough");
      return 0;
   }
   if (!base2.empty() && !base2.get_boundary().verts().pts().get_plane(P2, 0.1)) {
      err_adv(debug, "Primitive::extend_branch: base2 not planar enough");
      return 0;
   }

   // Base normal(s) must be reasonably perpendicular to the
   // proposed plane normal the stroke will project into:
   const double MAXDOT = 0.71;
   if (fabs(P1.normal() * n) > MAXDOT ||
      (!base2.empty()) && fabs(P2.normal() * n) > MAXDOT) {
      err_adv(debug, "Primitive::extend_branch: surface normals too incompatible");
      return 0;
   }

   // Create a Primitive
   // (must determine a suitable skeleton mesh):
   Primitive* ret = get_primitive(base1.mesh());
   if (!(ret && ret->_skel_mesh)) {
      err_adv(debug, "Primitive::extend_branch: no %s found",
              ret ? "skeleton" : "Primitive");
      delete ret;
      return 0;
   }

   // Find length of projected stroke compared to base dimensions
   // XXX - duplicates code in extend(), below
   double xfactor = Config::get_var_dbl("EXTENDER_SAMPLING_FACTOR", 0.8,true);
   double w1 = xfactor*base1.boundary_edges().avg_len();
   double w2 = (!base2.empty()) ? (xfactor*base2.boundary_edges().avg_len()) : w1;
   Wpt_list pts;
   pixels.project_to_plane(Wplane(base1.get_verts().center(), n), pts);
   if (pts.length() < 0.7* (w1 + w2)/2) {
      err_adv(debug, "Primitive::extend_branch: input stroke too short");
      return 0;
   }

   // Looks good -- hand off to the Primitive
   if (ret->extend(pixels, base1, base2, n, cmd))
      return ret;

   delete ret;
   return 0;
}

//! Chop off the first part of the Wpt_list up to length d.
inline void
chop(Wpt_list& pts, double d)
{
   //
   // Prerequisite: pts.update_length() should have been
   // called

   assert(d > 0);

   if (pts.length() < d) {
      err_adv(debug, "chop: Wpt_list is too short");
      return;
   }

   // Parameter along pts in range [0,1]:
   double s = d / pts.length();

   int seg;             // Index of last point before the chop
   Wpt_list foo;        // temporary Wpt_list

   // Add 1st point
   foo += pts.interpolate(s, 0, &seg);

   // Add vertices following trim location
   for (int i=seg+1; i<pts.num(); i++)
      foo += pts[i];

   // Overwrite points with trimmed point list
   pts = foo;

   // Restore length data for the point list
   pts.update_length();
}

//! Trim away the last part of the Wpt_list of length d.
inline void
trim(Wpt_list& pts, double d)
{
   //
   // Prerequisite: pts.update_length() should have been
   // called

   assert(d > 0);

   if (pts.length() < d) {
      err_adv(debug, "trim: Wpt_list is too short");
      return;
   }

   // Parameter along pts in range [0,1]:
   double s = d / pts.length();

   // Find out where to do the trim:
   int seg; // Index of last point before the trim
   Wpt foo = pts.interpolate(1 - s, 0, &seg);

   // Chop off the vertices after the trim:
   if (pts.valid_index(++seg))
      pts.truncate(seg);

   // Add the trim point
   pts += foo;

   // Restore length data for the point list
   pts.update_length();
}

//! Specialized helper function used by
//! min_dist_permutation(), below.
inline double
sum_dist(CWpt_list& pts1, CWpt_list& pts2, int p=0)
{
   //
   // Returns the summed distance between the two point
   // lists if pts2 is cycled right by amount p.

   assert(pts1.num() == pts2.num());
   int n = pts1.num();
   assert(p >= 0 && p < n);

   double ret = 0;
   for (int i=0; i<n; i++)
      ret += pts2[(i + n - p)%n].dist(pts1[i]);
   return ret;
}

//! Return the cyclic permutation of pts2 (i.e. number of
//! slots to shift right), to yield the least summed
//! distance between the point lists.
inline int
min_dist_permutation(CWpt_list& pts1, CWpt_list& pts2)
{

   assert(pts1.num() == pts2.num());
   int n = pts1.num();

   int ret = 0;
   double min_dist = sum_dist(pts1, pts2), d;
   for (int p=1; p<n; p++) {
      if ((d = sum_dist(pts1, pts2, p)) < min_dist) {
         min_dist = d;
         ret = p;
      }
   }

   return ret;
}

/*******************************************************
 * Pcalc:
 *
 *   Deals with affine sequences t(i) = a*t(i-1) + b.
 *
 *   Given a > 0, b, and t0, let
 *
 *     t(0) = t0   (given)
 *   
 *     t(i) = a*t(i-1) + b
 *
 *   In closed form:
 *
 *             a^i*t0 + b*(a^i - 1)/(a - 1)   if  a != 1
 *     t(i) =
 *             t0 + b*i                       if  a == 1
 *******************************************************/
class Pcalc {

 protected:
   double       _t0;    // First term in sequence
   double       _a;     // multiplier
   double       _b;     // addend
   double       _c;     // c = b/(a - 1)
   int          _N;     // chosen so t(N) approximates given L
   double       _tN;    // t(N)

   // Return given value x, unless it's very close to t,
   // then return t.
   double snap(double x, double t) const {
      return (fabs(x-t) < gEpsAbsMath) ? t : x;
   }

   // Finds i for which t(i) == L
   double solve(double L) const {
      if (is_bad()) {
         err_msg("Pcalc::solve: no solution");
         return 0.0;
      }
      if (_b == 0)
         return log(L/_t0)/log(_a);
      if (_a == 0)
         return L/_b;
      if (_a == 1)
         return (L - _t0)/_b;
      return log((L + _c)/(_t0 + _c))/log(_a);
   }
      
 public:

   //******** MANAGERS ********

   Pcalc(double a, double b, double t0, double L) { set(a,b,t0,L); }
   Pcalc() : _t0(0), _a(0), _b(0), _c(0), _N(0), _tN(0) {}

   void set(double a, double b, double t0, double L) {
      _a  = max(snap(a,1),0.0);
      _b  = snap(b,0);
      _t0 = snap(t0,0);
      _c = (_a == 1) ? 0.0 : _b/(_a - 1);
      double alpha = solve(L);
      _N = max(1,(int)round(alpha));
      _tN = t(_N);
   }

   //******** ACCESSORS ********
   double N() const { return _N; }

   bool is_bad() const {
      return (
         (_a < gEpsAbsMath)                     ||
         (_b == 0 && (_a == 1 || _t0 == 0))
         );
   }

   // For integer i, returns sequence value t_i
   double t(double i) const {
      if (_a == 1)
         return _t0 + i*_b;
      return pow(_a,i)*(_t0 + _c) - _c;
   }

   // Normalized so u(0) = 0, u(N) = 1:
   double u(double i) const { return (t(i) - _t0)/(_tN - _t0); }

   // Control point locations on original curve, normalized in [0,1]:
   double c(double i) const { return (u(i) + u(i+1))/2; }

   // Control point locations on chopped curve, normalized in [0,1]:
   double d(double i) const { return (c(i) - c(0))/(c(_N-1) - c(0)); }
};

MAKE_PTR_SUBC(STERILIZE_MEMES_CMD,COMMAND);
class STERILIZE_MEMES_CMD : public COMMAND {
 public:

   //******** MANAGERS ********

   STERILIZE_MEMES_CMD(CVertMemeList& m, bool done) : COMMAND(done), _memes(m) {}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("STERILIZE_MEMES_CMD",
                        STERILIZE_MEMES_CMD*, COMMAND, CCOMMAND*);

   //******** COMMAND VIRTUAL METHODS ********

   // Sterilize the memes
   virtual bool doit() {
      if (is_done())
         return true;
      _memes.sterilize();
      return COMMAND::doit();      // update state in COMMAND
   }

   // Unsterilize the memes
   virtual bool undoit() {
      if (!is_done())
         return true;
      _memes.unsterilize();
      return COMMAND::undoit();    // update state in COMMAND
   }

 protected:
   VertMemeList _memes;
};


inline void
sterilize_verts(CBvert_list& verts, MULTI_CMDptr cmd)
{
   VertMemeList memes = Bbase::find_boss_vmemes(verts);
   if (!memes.empty()) {
      memes.sterilize();
      if (cmd)
         cmd->add(new STERILIZE_MEMES_CMD(memes, true));
   }
}

inline Pcalc
gen_pcalc(double w1,	//!< w1: edge lengths at beginning
	double w2,			//!< w2: edge lengths at end
	double L			//!< L:  total length
	)
{
   
   
   

   // Goal: create a Bcurve with sampling that varies with
   // the tube's cross-sectional size.
   //
   // For a tube with initial cross sectional size w1, final
   // size w2, built along a 3D curve of length L connecting
   // the centers of the 2 quads, we'll compute parameter
   // values u0 = 0, u1, ..., uN = 1, whose spacing along
   // the curve matches the tube's cross-sectional size at
   // each point. 
   //                                            
   //         c0        c1        ...       cN-1 
   //     |--- o ---|--- o ---|--- o ---|--- o ---|
   //     u0=0      u1       u2        ...       uN=1
   //                                            
   //                                              
   //     q1   o+++++++++o+++++++++o+++++++++o    q2
   //  (size w1)      skeleton curve           (size w2)
   //
   //     |<----------------- L ----------------->|
   //
   // The control points of the skeleton curve occur at
   // parameter values c0, c1, ..., cN-1 shown above, using:
   //
   //    c_i = (u_i + u_i+1)/2.
   //
   // The values are given with respect to the original 3D
   // curve of length L. In building the curve, we have to
   // convert them to parameter values with respect to the
   // chopped & truncated curve, using the v() method of
   // Pcalc.

   double r = (w2 - w1)/L;     // Increase in cross-sectional size per length
   double a = 1 + r/(1 - r/2); // Factor of affine sequence (see Pcalc)
   double b = w1/(1 - r/2);    // Addend of affine sequence (see Pcalc)
   Pcalc pcalc(a, b, 0, L);    // 0 means t(0) == 0

   assert(!pcalc.is_bad());

   if (0 && debug) {
      err_msg("\n****************************************");
      for (int j=0; j<=pcalc.N(); j++)
         err_msg("t%d: %f", j, pcalc.u(j));
      err_msg("****************************************");
   }
   return pcalc;
}

inline Bpoint*
create_skel_point(const LMESHptr& mesh, CBface_list& disk, CWvec& n, CWpt& p)
{
   return new Bpoint(mesh, create_disk_map(disk, n, p, false));
}

inline SimplexFrame*
get_end_frame(uint key, Bpoint* b, Bcurve* c, int bnum, CWvec& n)
{
   if (b)
      return new BpointFrame(key, b);
   if (c) {
      if      (bnum == 1) b = c->b1();
      else if (bnum == 2) b = c->b2();
      else assert(0);
      assert(!c->edges().empty());
      Bedge* e = ((bnum == 1) ? c->edges().first() : c->edges().last());
      return new SkelFrame(key, b->vert(), n, e);
   }
   return 0;
}

inline void
remove_end_kinks(
   Wpt_list& pts,
   double w1,
   double w2
   )
{
   // try to remove small zig-zags at end 1
   while (pts.num() > 2 && pts[0].dist(pts[1]) < w1 && pts.valid_index(1))
      pts.pull_index(1);

   // try to remove small zig-zags at end 2
   while (pts.num() > 2 &&
          pts.last().dist(pts[pts.num()-2]) < w2 && pts.valid_index(pts.num()-2))
      pts.pull_index(pts.num()-2);
   pts.update_length();
}

inline void
process_skel_wpts(
   Wpt_list& pts,
   double w1,
   double w2,
   double& L,
   Pcalc& pcalc,
   Wvec& tan1,
   Wvec& tan2
   )
{
   remove_end_kinks(pts, w1, w2);

   L  = pts.length();    // Total length of 3D curve
   pcalc = gen_pcalc(w1, w2, L);

   // Chop off ends of the curve
   // I.e., from t0 to u0 and uN-1 to tN in diagram in gen_pcalc() above.
   chop(pts, L*pcalc.c(0));                 // before 1st control point
   trim(pts, L*(1 - pcalc.c(pcalc.N()-1))); // after last control point

   // Compute starting and ending vectors for the curve:
   tan1 = pts.tan(0);
   tan2 = pts.tan(pts.num()-1);
}

//! Generate skeleton curve (undoable part happens with skel curve)
inline Bcurve*
create_skel_curve(
   LMESHptr skel_mesh,
   CWpt_list& pts,
   CWvec& n,
   const param_list_t& tvals,
   Bpoint* b1,
   Bpoint* b2,
   Cstr_ptr& name,
   MULTI_CMDptr cmd
   )
{
   Bcurve* skel_curve = new Bcurve(skel_mesh, pts, n, tvals, 0, b1, b2);
   skel_curve->set_name(name);
   init_name(skel_curve->b1(), name + "_b1");
   init_name(skel_curve->b2(), name + "_b2");
   cmd->add(new SHOW_BBASE_CMD(skel_curve));
   cmd->add(new SHOW_BBASE_CMD(skel_curve->b1()));
   cmd->add(new SHOW_BBASE_CMD(skel_curve->b2()));
   skel_curve->mesh()->update_subdivision(1);
   return skel_curve;
}

void
Primitive::create_skel_curve(
   const Pcalc& pcalc,
   param_list_t& tvals,
   CWpt_list& pts,
   Bpoint* bp1,
   Bpoint* bp2,
   CWvec& n,
   Bcurve*& skel_curve,
   Bedge_list& edges,
   MULTI_CMDptr cmd
   )
{
   err_adv(debug, "Primitive::create_skel_curve");

   assert(bp1 && bp2 && cmd && !pcalc.is_bad());

   tvals.clear();
   // Compute t-vals for Bcurve control vertices WRT chopped curve
   for (int i=0; i<pcalc.N(); i++)
      tvals += pcalc.d(i);
   if (tvals[0] != 0.0) {
      tvals[0] = 0;
   }
   if (tvals.last() != 1.0) {
      tvals.last() = 1.0;
   }
   skel_curve =
      ::create_skel_curve(_skel_mesh, pts, n, tvals, bp1, bp2, name(), cmd);
   absorb_skel(skel_curve);
   edges = skel_curve->edges();
}

Bface_list
Primitive::build_cap(Bvert* a, Bvert* b, Bvert* c, Bvert* d, double du)
{
   UVpt ua(0,0), ub(du,0), uc(du,du), ud(0,du);
   Bface_list ret(add_quad(a,b,c,d,ua,ub,uc,ud)->face());
   add_face_memes(ret);
   return ret;
}

inline Wpt_list
recirc(CWpt_list& p, CWpt& o, double R)
{
   assert(p.num() > 2);
   Wpt_list ret(p.num());
   for (int i=0; i<p.num(); i++)
      ret += o + ((p[i] - o).normalized()*R);
   return ret;
}

inline Wpt_list
smoothed_pts(CWpt_list& p, CWpt& o)
{
   int n = p.num();
   assert(n > 2);
   Wpt_list ret(n);
   for (int i=0; i<n; i++) {
      Wvec v = p[i] - o;
      Wvec u0 = p[(i-1+n)%n] - p[i];
      Wvec u1 = p[(i+1  )%n] - p[i];
      Wvec t0 = (u0).orthogonalized(v).normalized()*(u0.length()*0.3);
      Wvec t1 = (u1).orthogonalized(v).normalized()*(u1.length()*0.3);
      ret += p[i] + t0 + t1;
   }
   return ret;
}

//! tread the points as a ring, spread them evenly
//! around a circle about their center
inline Wpt_list
do_rounding(CWpt_list& pts)
{

   if (pts.num() < 3)
      return pts;
   Wpt      o = pts.average();
   double   R = avg_dist(pts, o);
   Wpt_list ret = recirc(pts, o, R);
   for (int i=0; i<10; i++) {
      ret = recirc(smoothed_pts(ret, o), o, R);
   }
   return ret;
}

void
Primitive::build_tube(
   CoordFrame* f1,      //!< frame at 1st end
   CoordFrame* f2,      //!< frame at last end
   Bvert_list& p1,      //!< 1st ring
   Bvert_list& p2,      //!< last ring
   CWpt_list&  u1,      //!< local coords at 1st end
   CWpt_list&  u2,      //!< local coords at last end
   CBvert_list& v1,
   CBvert_list& v2,
   double du,
   const Pcalc& pcalc,  //
   double L,
   CBedge_list& edges,  //!< edges of skel curve
   CWvec& n,
   Bcurve* skel_curve,
   Bpoint* skel_point,
   Bface_list& cap1,
   Bface_list& cap2,
   bool sleeve_needed,
   MULTI_CMDptr cmd
   )
{
   err_adv(debug, "Primitive::build_tube");

   Bvert_list prev(u1.num());
   Bvert_list cur (u1.num());

   Wpt_list r1 = u1;//do_rounding(u1);
   Wpt_list r2 = u2;//do_rounding(u2);

   // build 1st ring
   if (sleeve_needed)
      build_ring(f1, u1, p1);
   else {
      for (int i = 0; i < v1.num(); i++)
         p1 += v1[i];
   }
   prev = p1;

   // generalize from build_cap
   Bvert_list A = _base1.interior_verts(), B;
   Wpt_list cap_u1 = f1->inv() * A.pts();
   VertMapper vmap;
   if (sleeve_needed) {
      build_ring(f1, cap_u1, B);
      A += v1;
      B += p1;
      vmap.set(A, B);
      for (int i = 0; i < _base1.num(); i++)
         cap1 += gen_flip_face(_base1[i], vmap, _patch);
      add_face_memes(cap1);
      copy_edges(_base1.get_edges(), vmap);
   } else 
      push(_base1, cmd);

   // Do internal rings and bands (if any)
   for (int k=0; k<edges.num(); k++) {
      // Get frame for the ring
      SimplexFrame* frame = new EdgeFrame((uint)this, edges[k], n);

      // Generate the ring of verts.
      // Interpolate local coords if using 2 sets:
      if (u2.empty()) {
         build_ring(frame, r1, cur);
      } else {
         double t = pcalc.u(k+1);
         build_ring(frame, r1*(1-t) + r2*t, cur);
      }

      build_band(prev, cur, du, pcalc.u(k), pcalc.u(k+1));

      prev=cur;
   }

   // Last ring
   if (f2) {
      // build last ring
      if (sleeve_needed) {
         build_ring(f2, u2, p2);

         A = _base2.interior_verts();
         B.clear();
         Wpt_list cap_u2 = f2->inv() * A.pts();
         build_ring(f2, cap_u2, B);
         A += v2;
         B += p2;
         vmap.set(A, B);
         for (int i = 0; i < _base2.num(); i++)
            cap2 += gen_flip_face(_base2[i], vmap);
         copy_edges(_base2.get_edges(), vmap);

      } else {
            for (int i = 0; i < v2.num(); i++)
               p2 += v2[i];
            push(_base2, cmd);
      }

   } else {
      // no quad 2: use frame from skel point or curve (whichever exists):
      f2 = get_end_frame((uint)this, skel_point, skel_curve, 2, n);
      // use u1 local coords shifted forward by
      // displacement from q1 to 1st control point:
      double offset = L*pcalc.c(0);
      build_ring(f2, Wtransf::translation(Wvec(offset,0,0)) * r1, p2);

      Bface_list temp = sleeve_needed ? cap1 : _base1;
      A = temp.interior_verts();
      B.clear();
      Wpt_list cap_u2 = f1->inv() * A.pts();
      build_ring(f2, Wtransf::translation(Wvec(offset,0,0)) * cap_u2, B);
      A += p1;
      B += p2;
      vmap.set(A, B);
      for (int i = 0; i < temp.num(); i++)
         cap2 += gen_flip_face(temp[i], vmap);
      if (!sleeve_needed)
         cap2.reverse_faces();
      copy_edges(temp.get_edges(), vmap);
   }
   add_face_memes(cap2);
   cur = p2;

   // Last band:
   double vp = skel_point ? 0.0 : pcalc.u(edges.num()); // previous v-value
   build_band(prev, cur, du, vp, 1.0);

   //cap2 = build_cap(cur[0], cur[1], cur[2], cur[3], du);

   // done building, notify mesh of changes:
   _mesh->changed(BMESH::TRIANGULATION_CHANGED);

   hookup();

   if (debug) {
//       cerr << "Primitive::build_tube: ";
//       print_all_inputs();
      print_input_graph();
   }

   BMESH::set_focus(_mesh, _patch);

   show_surface(this, cmd);
}

inline void
extract_sides(CBface_list& bases, CEdgeStrip& side, Bvert_list& side2, Bvert_list& side4)
{
   side2.clear();
   side4.clear();

   Bvert_list bound_verts = bases.get_boundary().verts();
   int start = bound_verts.get_index(side.first());
   bound_verts.shift(-start);
   assert(bound_verts[0] == side.first());

   int total = bound_verts.num();
   assert(total%2 == 0);
   int a = side.num();
   int b = total/2 - a;
   
   if (bound_verts[1] == side.vert(1) || bound_verts[1] == side.last()) { // same orientation
      for (int i = a+b; i >= a; i--)
         side2 += bound_verts[i];
      for (int i = 2*a+b; i <= total; i++)
         side4 += bound_verts[i%total];
   } else {
      for (int i = a+b; i <= a+2*b; i++)
         side2 += bound_verts[i];
      for (int i = b; i >= 0; i--)
         side4 += bound_verts[i];
   }
}

inline SimplexFrame*
get_frame(uint key, CBface_list& bases, CWvec& n, Bvert* a1, Bvert* a2, Bvert* a4, Bvert* a3, Bvert_list& vlist)
{
   Bedge* start = lookup_edge(a1, a2);
   Bedge* end = lookup_edge(a3, a4);
   
   Bface* f = start->f1();
   if (!bases.contains(f)) {
      f = start->f2();
      assert(bases.contains(f));
   }
   
   Bedge* e = start;
   vlist.clear();
   vlist += a1;
   while (e != end) {
      e = f->opposite_quad_edge(e);
      vlist += e->other_vertex(f->quad_opposite_vert(vlist.last()));
      assert(bases.get_verts().contains(vlist.last()));
      f = e->other_face(f->quad_partner());
   }

   if (vlist.num()%2 == 1)
      return new VertFrame(key, vlist[vlist.num()/2], Wvec(1,1,1), n);
   else {
      Bvert *v1 = vlist[vlist.num()/2-1], *v2 = vlist[vlist.num()/2];
      e = lookup_edge(v1, v2);
      return new EdgeFrame(key, e, n, 0.5, v1!=e->v1());
   }
}

inline void
set_creases(CBedge_list& edges)
{
   for (int i = 0; i < edges.num(); i++)
      edges[i]->set_crease();
}

bool
Primitive::extend(
   CPIXEL_list& pixels,
   ARRAY<int>& corners,
   CBface_list& bases,
   CEdgeStrip& side,
   CWvec& n,
   MULTI_CMDptr cmd     // command list for undo/redo
   )
{
   // The user drew a stroke from the first vertex of side strip to
   // the other end vertex of the strip.
   //
   // Now we'll project the stroke to the plane defined by the
   // given normal vector n.  From this we build a wpt_listmap
   // for the roof.

   assert(!bases.empty() && !side.empty());
   assert(cmd != NULL);

   static int roof_num=0;
   set_name(str_ptr("roof_") + str_ptr(++roof_num));

   static bool debug = ::debug ||
      Config::get_var_bool("DEBUG_ROOF",true);

   //
   // 1. project the stroke to 3D, correct it, 
   //    find parameterization
   //

   // Project screen points to the world-space plane:
   Wplane P((side.first()->loc()+side.last()->loc())*0.5, n);
   Wpt_list pts;
   pixels.project_to_plane(P, pts);

   // Need a correction so the skel curve actually begins
   // (and ends) at the ends of the side strip.
   pts.push(side.first()->loc());
   pts += side.last()->loc();
   for (int i = 1; i < corners.num(); i++)
      corners[i]++;
   corners[corners.num()-1]++;
   pts.update_length();

   // Decide spacing of the verts on the wpt_listmap
   //   xfactor < 1 packs in more edges.
   //   xfactor > 1 strings them out.
   double xfactor = Config::get_var_dbl("ROOF_SAMPLING_FACTOR", 0.5,true);
   Bvert_list side_verts = side.verts();
   side_verts += side.last();
   double w = xfactor*side_verts.pts().avg_len();

   //
   // 2. Create a wpt_listmap 
   //

   assert(_base1.empty());
   _base1.append(bases); // base1 is the rect area the roof is attached to

   BsimplexMap *p0, *p1;
   p0 = new BsimplexMap(side.first());
   p1 = new BsimplexMap(side.last());
   _other_inputs += Bbase::find_controller(side.first());
   _other_inputs += Bbase::find_controller(side.last());
   Wpt_listMap* map = new Wpt_listMap(pts, p0, p1, n);

   // 
   // 3. Build the primitive
   //

   //******** Build branch of the Primitive ********
   // Define local coordinates for the cross sections

   Bvert_list v1;   // list of vertices on the starting cross section
   Wpt_list   u1;   // Corresponding local coordinates
   ARRAY<int> crease_locs; // crease locations, indexed into v1

   // get simplex frame
   SimplexFrame* f1;
   if (side.num()%2 == 0)
      f1 = new VertFrame((uint)this, side.vert(side.num()/2), Wvec(1,1,1), n);
   else {
      Bedge* e = side.edge(side.num()/2);
      f1 = new EdgeFrame((uint)this, e, n, 0.5, side.vert(side.num()/2)==e->v1());
   }

   // create vertices of based on wpt_listmap:
   v1 += side.first();
   assert(corners.first() == 0 && corners.last() == pts.num()-1);
   for (int i = 1; i < corners.num(); i++) {
      double p_start = pts.partial_length(corners[i-1]);
      double p_end = pts.partial_length(corners[i]);
      int num_edges = ((int)((p_end-p_start)/w))+1;
      if (num_edges == 1) continue; // filter the noise
      double t_start = p_start/pts.length();
      double t_end = p_end/pts.length();
      double delta_t = (t_end-t_start)/num_edges;
      int end = (i==corners.num()-1)?(num_edges-1):num_edges;
      for (int j = 1; j <= end; j++) {
         v1 += _mesh->add_vertex(map->map(t_start+j*delta_t));
         add_edge(v1[v1.num()-2], v1.last());
         create_xf_meme((Lvert*)v1.last(), f1);
      }
      if (i!=corners.num()-1) crease_locs += (v1.num()-1);
   }
   add_edge(v1.last(), side.last());
   v1 += side.last();
   v1.reverse();
   for (int i = 0; i < crease_locs.num(); i++)
      crease_locs[i] = v1.num() - 1 - crease_locs[i];

   // Compute local coords
   for (int i = 1; i < v1.num()-1; i++)
      u1 += f1->inv() * v1[i]->loc();

   //******** Generate xsecs and bands ********

   Bvert_list prev(u1.num()+2);
   Bvert_list cur (u1.num()+2);
   Bedge_list creases;

   // build 1st cross section
   prev = v1;
   // create panel
   Bvert_list bound = v1;
   bound -= bound.last();
   bound += side.verts();
   // XXX - switch to using action...
   Bsurface* surf = Panel::create(bound);
   if (surf) {
      surf->set_res_level(0);
      if(debug) cerr << "panel created" << endl;
      cmd->add(new SHOW_BBASE_CMD(surf));
      creases += surf->bfaces().boundary_edges();
      surf->set_name("roof_panel");
   }

   // extract other sides of the rect area
   //       3
   //    ---------
   //    |        |
   //    |        |
   //  4 |        | 2
   //    |        |
   //    ---------
   //         1(side)
   Bvert_list side2, side4;
   extract_sides(bases, side, side2, side4);   
   assert(side2.num() == side4.num());
   assert(prev.first() == side2.last() && prev.last() == side4.last());
   Bvert_list cur_cross;

   // Do internal cross sections and bands (if any)
   for (int k=side2.num()-2; k>=0; k--) {
      // Get frame for the ring
      SimplexFrame* frame = get_frame((uint)this, bases, n, side2[k], side2[k+1], side4[k], side4[k+1], cur_cross);

      // Generate the cross section of verts.
      cur.clear();
      cur += side2[k];
      for (int i=0; i<u1.num(); i++) {
         cur += _mesh->add_vertex(frame->xf()*u1[i]);
         create_xf_meme((Lvert*)cur.last(), frame);
      }
      cur += side4[k];
      assert(cur.first() != cur.last());
      assert(cur.num() == prev.num());

      int n = cur.num();
      double vc = (side2.num()-1-k);
      double vp = vc - 1;
      for (int i=0; i<n-1; i++) {
         int j = (i+1) % n;
         double ui = i;            // u coordinate for p[i], c[i]
         double uj = ui + 1;       // u coordinate for p[j], c[j]
         add_quad(cur[i], cur[j], prev[j], prev[i],
                  UVpt(ui,vc), UVpt(uj,vc), UVpt(uj,vp), UVpt(ui,vp));
         if (crease_locs.contains(i))
            lookup_edge(cur[i], prev[i])->set_crease();
      }

      prev=cur;
   }

   // Last cross section
   bound = cur;
   assert(cur.last() == cur_cross.last());
   bound -= bound.last();
   cur_cross.reverse();
   bound += cur_cross;
   bound -= bound.last();
   bound.reverse();
   // XXX - switch to using action...
   Bsurface* other_surf = Panel::create(bound);
   if (other_surf) {
      other_surf->set_res_level(0);
      if(debug) cerr << "panel created" << endl;
      cmd->add(new SHOW_BBASE_CMD(other_surf));
      creases += other_surf->bfaces().boundary_edges();
      other_surf->set_name("roof_panel");
   }
   
   // done building, notify mesh of changes:
   push(bases, cmd);
   creases -= bases.boundary_edges();

   Bedge_list pedges = bases.boundary_edges();
   for (int i = 0; i < pedges.num(); i++) {
      Bedge* e = pedges[i];
      assert(e->can_promote() && e->adj());
      Bface* f = (*(e->adj()))[0];
      e->promote(f);
   }

   set_creases(creases);
   MeshGlobal::deselect(bases);
   finish_build(cmd);

   return true;
}

bool
Primitive::extend(
   CPIXEL_list& pixels,
   CBface_list& b1,
   CBface_list& b2,
   CWvec& n,
   MULTI_CMDptr cmd     // command list for undo/redo
   )
{
   // The user drew a stroke from the center of base b1 to
   // either the center of base b2 (if given) or to a point in
   // the air.
   //
   // Now we'll project the stroke to the plane defined by the
   // given normal vector n.  From this we build a skeleton
   // curve for a branch of the primitive connecting, at b1 and
   // optionally at b2.

   assert(!b1.empty());
   assert(cmd != NULL);

   static int branch_num=0;
   set_name(str_ptr("branch_") + str_ptr(++branch_num));

   static bool debug = ::debug ||
      Config::get_var_bool("DEBUG_EXTENDER",false);

   // determine if sleeve is needed
   bool sleeve_needed = b1.mesh()->subdiv_level() == _mesh->cur_level();

   //
   // 1. project the stroke to 3D, correct it, 
   //    find parameterization
   //

   // Project screen points to the world-space plane:
   Wplane P(b1.get_verts().center(), n);
   Wpt_list pts;
   pixels.project_to_plane(P, pts);

   // Need a correction so the base curve actually begins
   // (and ends) at the base center(s).
   pts.push(b1.get_verts().center());
   if (!b2.empty()) pts += b2.get_verts().center();
   pts.update_length();

   // Decide spacing of tube sections
   //   xfactor < 1 packs in more tube sections.
   //   xfactor > 1 strings them out.
   double xfactor = Config::get_var_dbl("EXTENDER_SAMPLING_FACTOR", 0.8,true);
   double w1 = xfactor*b1.boundary_edges().avg_len();
   double w2 = (!b2.empty()) ? (xfactor*b2.boundary_edges().avg_len()) : w1;

   double L=0;
   Pcalc pcalc;
   Wvec tan1, tan2;
   process_skel_wpts(pts, w1, w2, L, pcalc, tan1, tan2);

   //
   // 2. Create a skeleton curve (or single point)
   //

   Bpoint* skel_point = 0;
   Bcurve* skel_curve = 0;
   param_list_t tvals;
   Bedge_list edges;

   assert(_base1.empty());
   assert(_base2.empty());

   _base1 = b1;
   _base2 = b2;

   // For short stroke, it's just a point.
   if (L < 1.4 * (w1 + w2)/2) {
      err_adv(debug, "Primitive::extend: using skeleton point");
      skel_point = create_skel_point(_skel_mesh, _base1, n, pts.first());
      init_name(skel_point, name() + "-skel-point");
      absorb_skel(skel_point);
      cmd->add(new SHOW_BBASE_CMD(skel_point));
   } else {
      Bpoint* bp1 = create_skel_point(_skel_mesh, _base1, n, pts.first());
      Bpoint* bp2 = 0;
      if (!b2.empty()) {
         // make the last bpoint relative to the base at end 2:
         bp2 = create_skel_point(_skel_mesh, _base2, n, pts.last());
      } else {
         // make them both relative to the base at end 1:
         bp2 = create_skel_point(_skel_mesh, _base1, n, pts.last());
      }
      create_skel_curve(pcalc, tvals, pts, bp1, bp2, n, skel_curve, edges, cmd);
   }

   // 
   // 3. Build the primitive
   //

   //******** Build branch of the Primitive ********
   // Define local coordinates for the rings

   Bvert_list v1, v2;   // Matching rings of vertices for two quads
   Wpt_list   u1, u2;   // Corresponding local coordinates

   double du = min_uv_delt((_base1 + _base2).get_boundary().edges());
   err_adv(debug, "Primitive::extend: using %f for du", du);

   //******** Base 1 ********
   // Get vertices of base 1 in CCW order:
   v1 = _base1.get_boundary().verts();

   // get best fit planes
   Wplane P1, P2;
   if (!_base1.get_boundary().verts().pts().get_plane(P1, 0.1)) {
      err_adv(debug, "Primitive::extend: base1 not planar enough");
      return false;
   }
   if (_base1[0]->norm() * P1.normal() < 0)
      P1 = -P1;
   if (!_base2.empty() && !_base2.get_boundary().verts().pts().get_plane(P2, 0.1)) {
      err_adv(debug, "EXTENDER::compute_plane: base2 not planar enough");
      return false;
   }
   if (!_base2.empty() && (_base2[0]->norm()*P2.normal()<0))
      P2 = -P2;

   // Compute local coords
   // also Handle drawing 'into' the face properly: --Jim
   bool b1back = ((P1.normal() * tan1) < 0);
   DiskMap* f1 = create_disk_map(_base1, n, b1back);
   _other_inputs += f1;
   if (debug) {
      cerr << "created DiskMap f1: " << (CoordFrame*)f1 << endl;
      WORLD::create(new SHOW_COORD_FRAME(f1));
      f1->set_do_debug();
   }
   u1 = f1->inv() * v1.pts();

   //******** Base 2 ********
   DiskMap* f2 = 0;
   if (!b2.empty()) {
      // get vertices of base 2,
      // reverse order to match b1 verts if needed,
      // compute local coords
      // permute them to get the best match,
      
      v2 = _base2.get_boundary().verts();

      // b2 back is true if tangent points *along* normal
      bool b2back = ((P2.normal() * tan2) > 0);
      // only reverse order if it needs to be reversed: --Jim
      if (b2back == b1back)
         v2.reverse();

      f2 = create_disk_map(_base2, n, !b2back);
      _other_inputs += f2;
      u2 = f2->inv() * v2.pts();

      // Find the cyclic shift that best aligns them:
      int p = min_dist_permutation(u1, u2);

      // Shift them:
      v2.shift(p);
      u2.shift(p);
   }

   //******** Generate rings and bands ********

   Bface_list cap1, cap2;
   Bvert_list p1, p2; // 1st and last rings
   build_tube(f1, f2, p1, p2, u1, u2, v1, v2, du, pcalc, L, edges, n,
              skel_curve, skel_point, cap1, cap2, sleeve_needed, cmd);

   if (b2.empty()) {
      cap2.clear();     // don't have skin grow there
   }

   finish_build(cmd);

   if (!sleeve_needed) {
      return true;
   }

   // create a skin to blend between the primitive and base surfaces

   // pfaces are the faces of the new primitive that will
   // be covered up by the skin:
   Bface_list pfaces = p1.one_ring_faces();

   // include faces from the other end if the primitive is
   // attached there (b2 non-empty) and there is more than one band
   // (skel_curve != 0):
   if (!b2.empty() && skel_curve)
      pfaces += p2.one_ring_faces();

   // the skel faces passed to skin include all those except b1 and b2:
   Bface_list skel_faces = pfaces + (_base1 + _base2).exterior_faces();

   // tell how some skel verts are identified: p1 --> v1.
   // use a 1-way map in case the two base regions are adjacent;
   // then the vertices of (v1 + v2) are not unique, so there
   // is no valid 2-way map. (but we only need a 1-way map anyway.)
   // (v1 and v2 are the boundary verts of base 1 and base 2.)
   VertMapper skel_map(p1, v1, false); // false means 1-way map
   if (!v2.empty()) {
      // include the identification p2 --> v2
      skel_map.add(p2, v2);
   }

   if (Skin::create_multi_sleeve(_base1 + _base2 + cap1 + cap2, skel_map, cmd)) {
      if (debug) {
         cerr << "Primitive::extend: created " << identifier() << endl;
      }
      return true;
   }
   cmd->undoit();
   return false;
}

Primitive*
Primitive::build_simple_tube(
   LMESHptr     mesh,
   Bpoint*      b1,
   double       rad,
   CWvec&       n,
   CPIXEL_list& stroke,
   MULTI_CMDptr cmd     // command list for undo/redo
   )
{
   static bool debug = ::debug ||
      Config::get_var_bool("DEBUG_SIMPLE_TUBE",false);

   err_adv(debug, "Primitive::build_simple_tube");

   if (!(mesh && b1 && rad>0 && !n.is_null() && cmd)) {
      err_adv(debug, "  bad input");
      return 0;
   }

   // Project screen points to the world-space plane:
   Wplane P(b1->loc(), n);
   Wpt_list pts;
   stroke.project_to_plane(P, pts);

   // Decide spacing of tube sections
   //   xfactor < 1 packs in more tube sections.
   //   xfactor > 1 strings them out.
   double xfactor = Config::get_var_dbl("EXTENDER_SAMPLING_FACTOR", 0.8);
   // w is length of edges in the tube
   double w = xfactor * (2.0/sqrt(3.0)) * rad;

   if (pts.length() < w) {
      err_adv(debug, "  stroke too short compared to ball size");
      return 0;
   }

   // Need a correction so the skel curve actually begins at b1
   pts.push(b1->loc());
   pts.update_length();
   remove_end_kinks(pts, w, w);

   Primitive* ret = new Primitive(mesh, b1->mesh());
   if (!ret) {
      err_adv(debug, "  can't allocate new Primitive");
      return 0;
   }

   if (ret->build_simple_tube(b1, rad, n, pts, cmd)) {
      return ret;
   }

   err_adv(debug, "  can't build tube");
   delete ret;
   return 0;
}

//! generate 4 local coords in [t,b,n] frame,
//! separated by distance 2d, CCW from top right:
inline Wpt_list
gen_4_ring(double d)
{

   Wpt_list ret(4);
   ret += Wpt(0, d, d);
   ret += Wpt(0,-d, d);
   ret += Wpt(0,-d,-d);
   ret += Wpt(0, d,-d);
   return ret;
}

bool
Primitive::build_simple_tube(
   Bpoint*      b1,
   double       w,
   CWvec&       n,
   CWpt_list&   pts,
   MULTI_CMDptr cmd     // command list for undo/redo
   )
{
   assert(b1 && cmd);
   assert(!n.is_null());
   assert(w > 0 && pts.length() >= w);
   assert(b1->mesh() == _skel_mesh);

   static bool debug = ::debug ||
      Config::get_var_bool("DEBUG_SIMPLE_TUBE",false);

   static int tube_num=0;
   set_name(str_ptr("simple_tube_") + str_ptr(++tube_num));

   double L = pts.length();
   int num_segs = (int)round(L/w);
   err_adv(debug, "length %f, edge length %f, num skel curve segs %d",
           L, w, num_segs);
   param_list_t tvals = make_params(num_segs);

   // create 2nd point
   Bpoint* b2 = new Bpoint(_skel_mesh, pts.last(), n);

   // create curve
   Bcurve* skel_curve =
      ::create_skel_curve(_skel_mesh, pts, n, tvals, b1, b2, name(), cmd);
   absorb_skel(skel_curve);
   CBedge_list& edges = skel_curve->edges();

   // compute local coords
   Wpt_list u = gen_4_ring(w/2);

   Bvert_list prev(u.num());
   Bvert_list cur (u.num());

   // build 1st ring, add end cap
   CoordFrame* f1 = new SkelFrame((uint)this, b1->vert(), n, 0, edges.first());
   build_ring(f1, Wtransf::translation(Wvec(-w/2,0,0)) * u, prev);
   build_cap(prev[1], prev[0], prev[3], prev[2]);

   // Do internal rings and bands (if any)
   double dv = 1.0/(edges.num() + 1);
   for (int k=0; k<edges.num(); k++) {
      build_ring(new EdgeFrame((uint)this, edges[k], n), u, cur);
      build_band(prev, cur, 1.0/cur.num(), k*dv, (k+1)*dv);
      prev=cur;
   }

   // build last ring, add end cap
   CoordFrame* f2 = new SkelFrame((uint)this, b2->vert(), n, edges.last(), 0);
   build_ring(f2, Wtransf::translation(Wvec(w/2,0,0)) * u, cur);
   build_cap(cur[0], cur[1], cur[2], cur[3]);

   // Last band
   build_band(prev, cur, 1.0/cur.num(), 1.0 - dv, 1.0);

   finish_build(cmd);

   if (debug) {
      cerr << "Primitive::build_simple_tube: inputs for " << identifier() << ":"
           << endl << "  "; inputs().print_identifiers();
   }

   return true;
}

void
Primitive::finish_build(MULTI_CMDptr cmd)
{
   hookup();
   if (cmd) {
      show_surface(this,cmd);
   }
   _mesh->changed(BMESH::TRIANGULATION_CHANGED);
   BMESH::set_focus(_mesh, _patch);
   if (ctrl_mesh()->cur_level() < 2)
      ctrl_mesh()->update_subdivision(2);
}

//!< Generate vertices and memes that share a skeleton
//!< frame, using the given local coordinates.
void
Primitive::build_ring(
   CoordFrame* frame,	//!< skeleton frame to use
   CWpt_list& u,        //!< local coords for generating the ring
   Bvert_list& ring)    //!< return list of vertices created
{

   ring.clear();
   for (int i=0; i<u.num(); i++) {
      ring += _mesh->add_vertex(frame->xf()*u[i]);
      create_xf_meme((Lvert*)ring.last(), frame);
   }
}

//! Build a circular band of quads joining the given
//! vertex lists.
void
Primitive::build_band(
   CBvert_list& p,      //!< "previous" ring of vertices
   CBvert_list& c,      //!< "current" ring of vertices
   double du,           //!< increment of u coord
   double vp,           //!< 'v' coordinate for the p vertices
   double vc)           //!< 'v' coordinate for the c vertices
{
   //
   //  ... --- cn-1 ------- c0 -------- c1 -------- c2 --- ...
   //          |            |           |           |             
   //          |            |           |           |             
   //          |            |           |           |             
   //          |            |           |           |             
   //          |            |           |           |             
   //  ... --- pn-1 ------- p0 -------- p1 -------- p2 --- ...     
   //
   //   As shown, surface normal points toward you.

   assert(p.num() == c.num());
   double u = 0;
   int n = c.num();
   for (int i=0; i<n; i++) {
      int j = (i+1) % n;
      double ui = u;            // u coordinate for p[i], c[i]
      double uj = u + du;       // u coordinate for p[j], c[j]
      u += du;
      add_quad(p[i], p[j], c[j], c[i],
               UVpt(ui,vp), UVpt(uj,vp), UVpt(uj,vc), UVpt(ui,vc));
   }
}

void
Primitive::absorb_skel(Bsurface* s)
{
   if (s) {
      // XXX - skeleton needs to be generalized to Bbase
      _skel_surfaces.add_uniquely(s);
      for (int i = 0; i < s->curves().num(); i++)
         absorb_skel(s->curves()[i]);
      s->add_output(this);
   }
}

void 
Primitive::absorb_skel(Bcurve* c)
{
   if (c) {
      _skel_curves.add_uniquely(c);
      absorb_skel(c->b1());
      absorb_skel(c->b2());
      c->add_output(this);
   }
}

void 
Primitive::absorb_skel(Bpoint* p)
{
   if (p) {
      _skel_points.add_uniquely(p);
      p->add_output(this);
   }
}

void 
Primitive::hide()
{
   _skel_points.hide();
   _skel_curves.hide();
   Bsurface::hide();
}

void 
Primitive::show()
{
   _skel_points.show();
   _skel_curves.show();
   Bsurface::show(); 
}

Bnode_list 
Primitive::inputs() const
{
   return Bsurface::inputs() + _skel_points + _skel_curves + _skel_surfaces + _other_inputs;
}

/************************************************************
 * Primitive:
 *
 *      meme management
 ************************************************************/
XFMeme*
Primitive::create_xf_meme(Lvert* vert, CoordFrame* frame)
{
   Meme* m = find_meme(vert);
   if (m) {
      XFMeme* ret = XFMeme::upcast(m);
      assert(ret);
      return ret;
   }
   
   return new XFMeme(this, vert, frame);
}

void
Primitive::create_xf_memes(CBvert_list& verts, CoordFrame* f)
{
   assert(LMESH::isa(verts.mesh()));
   for (int i=0; i<verts.num(); i++)
      create_xf_meme((Lvert*)verts[i], f);
}

/************************************************************
 * Primitive:
 *
 *      drawing
 ************************************************************/
int 
Primitive::draw(CVIEWptr& v)
{
   // Draw like any other Bsurface:
   int ret = Bsurface::draw(v);

   // XXX - Hack until we have a better idea. Skeleton curves
   //       and points that are selected draw without depth
   //       testing so they can show up, to be oversketched.
   //       For that to work, they have to get drawn after
   //       this surface.
   if (_skel_curves.any_selected() || _skel_points.any_selected()) {
      _skel_curves.draw(v);
      _skel_points.draw(v);
   }
   return ret;
}

int
Primitive::draw_vis_ref()
{
   // draw to the visibility reference image (for picking)

   int ret =  _patch ? _patch->draw_vis_ref() : 0;

   // draw "controls"
   if (_skel_curves.any_selected() || _skel_points.any_selected()) {
      _skel_curves.draw_vis_ref();
      _skel_points.draw_vis_ref();
   }

   return ret;
}


inline Wtransf
get_xf(CWpt& o, CWvec& t, CWvec& n)
{
   Wvec b = cross(n,t).normalized();
   return Wtransf(o, t, b, cross(t,b).normalized());
}


// end of file primitive.C
