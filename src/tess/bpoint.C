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
 * bpoint.C:
 **********************************************************************/
#include "disp/colors.H"                // Color::grey7 etc.
#include "geom/gl_view.H"
#include "gtex/color_id_texture.H"
#include "gtex/ref_image.H"
#include "bpoint.H"
#include "bcurve.H"

#include "tex_body.H"

using namespace mlib;

TAGlist* Bpoint::_bpoint_tags = NULL;

inline void
print_locs(Lvert* v)
{
   cerr << "vert subdiv locations: " << endl;
   for (int k=0; v; v = v->subdiv_vert())
      cerr << "  level " << k++ << ": " << v->loc() << endl;
}

inline void
set_locs(Lvert* v)
{
   // set the subdiv locs to equal the top-level loc
   assert(v);
   CWpt& p = v->loc();
   while((v = v->subdiv_vert()))
      v->set_loc(p);
}

inline bool
is_dirty(VertMeme* v)
{
   assert(v && v->vert() && v->vert()->lmesh());
   return v->vert()->lmesh()->dirty_verts().contains(v->vert());
}

inline void 
print_debug(const char* msg, PointMeme* vm)
{
   assert(vm);
   Lvert* v = vm->vert(); assert(v);
   cerr << endl << "  " << msg << ":" << endl
        << vm->bbase()->identifier() << ": level "
        << vm->bbase()->subdiv_level() << endl
        << "  subdiv loc valid bit is clear: "
        << (v->is_clear(Lvert::SUBDIV_LOC_VALID_BIT) ? "true" : "false") << endl
        << "  thinks it is in dirty list: "
        << (v->is_set(Lvert::DIRTY_VERT_LIST_BIT) ? "true" : "false") << endl
        << "  is in dirty list: "
        << (::is_dirty(vm) ? "true" : "false") << endl;

   print_locs(v);

}

/*****************************************************************
 * PointMeme
 *****************************************************************/
PointMeme::PointMeme(Bpoint* b, Lvert* v) :
   VertMeme(b, v, true)
{
}

void 
PointMeme::vert_changed_externally()
{
   // Called when the vertex location changed but this meme
   // didn't cause it:

   // XXX - causes problems when applying transforms... in progress.
   return;

   if (!map()->set_pt(loc())) {
      // If this turns out to be an issue, we should set the
      // vertex back how we like it, dammit. (For now just
      // complain.)

      err_msg("PointMeme::vert_changed_externally: can't reset map");
   }
}

CWpt& 
PointMeme::compute_update()
{
   static bool debug = Config::get_var_bool("DEBUG_BPOINT_MEME",false);
   if (debug) {
      print_debug("PointMeme::compute_update", this);
   }
   if (is_cold() && CurvePtMap::isa(map()))
      map()->set_pt(loc());
   return (_update = map()->map());
}

bool
PointMeme::handle_subdiv_calc()
{
   static bool debug = Config::get_var_bool("DEBUG_BPOINT_MEME",false);
   if (debug) {
      print_debug("PointMeme::handle_subdiv_calc",this);
   }
   Lvert* v = vert();
   Lvert* c = v->subdiv_vertex();
   if (v && c) {
      do_update();
      c->set_loc(v->loc());
   }
   return true;
}

/*****************************************************************
 * Bpoint
 *****************************************************************/
Bpoint::Bpoint(
   CLMESHptr& mesh,
   CWpt&  o,
   CWvec& n,
   int res_lev) :
   _map(0)
{
   // The mesh has to be the control mesh:
   assert(mesh->is_control_mesh());
   set_mesh(mesh);
   
   // Create the map function if needed:
   _map = new WptMap(o, n);

   // create vertex, meme, vert strip, etc.:
   setup(res_lev);
}

Bpoint::Bpoint(CLMESHptr& mesh, Map0D3D* map, int res_lev) :
   _map(map)
{
   // The mesh has to be the control mesh:
   assert(mesh->is_control_mesh());
   assert(_map);

   set_mesh(mesh);

   // create vertex, meme, vert strip, etc.:
   setup(res_lev);
}

/// Create a bpoint that is goverened by a SurfacePtMap, with given
/// Map2D3D and UV coordinate and given mesh
Bpoint::Bpoint(CLMESHptr& mesh, Map2D3D* surf, CUVpt& uvp, int res_lev) :
   _map(0)
{
   assert(mesh);
   set_mesh(mesh);

   // new way of doing things.. this constructor now just creates a
   // Bpoint that lies inside the given map2d3d by constructing a 
   // SurfacePtMap

   _map = new SurfacePtMap(surf, uvp);

   // create vertex, meme, vert strip, etc.:
   setup(res_lev);
}

/// Create a bpoint that is goverened by a SurfacePtMap, with given
/// vertex, UV coordinate, Map2D3D and resolution level
Bpoint::Bpoint(Lvert* vert, CUVpt& uv, Map2D3D* map, int res_lev) :
   _map(0)
{
   assert(vert);
   LMESH* m = vert->lmesh();
   assert(m);
   set_mesh(m);

   assert(map);
   _map = new SurfacePtMap(map, uv);

   // Create the meme and put it in the vertex:
   new PointMeme(this, vert);

   // Reset the strip and add the vertex:
   assert(_strip.empty());
   _strip.add(vert);

   // Get in on the drawing action:
   _mesh->drawables().add_uniquely(this);

   // XXX - should use setup(),
   //       taking into account vert is already created.
   set_res_level(res_lev);

   hookup();
}

/// Create a bpoint that is goverened by a CurvePtMap, with given
/// vertex, t, Map1D3D and resolution level
Bpoint::Bpoint(Lvert* vert, double& t, Map1D3D* map, int res_lev) :
   _map(0)
{
   assert(vert);
   LMESH* m = vert->lmesh();
   assert(m);
   set_mesh(m);

   assert(map);
   _map = new CurvePtMap(map, t);

   // Create the meme and put it in the vertex:
   new PointMeme(this, vert);

   // Reset the strip and add the vertex:
   assert(_strip.empty());
   _strip.add(vert);

   // Get in on the drawing action:
   _mesh->drawables().add_uniquely(this);

   // XXX - should use setup(),
   //       taking into account vert is already created.
   set_res_level(res_lev);

   hookup();
}

// XXX - seems redundant
Bpoint::Bpoint(Lvert* vert, int res_lev)
{
   assert(vert);
   LMESH* m = vert->lmesh();
   assert(m);
   set_mesh(m);

   _map = new BsimplexMap(vert);
   assert(_map);

   // Create the meme and put it in the vertex
   // the meme cannot be the boss
   Meme* temp = Bbase::find_boss_meme(vert);
   (new PointMeme(this, vert))->get_demoted();
   if (temp) temp->take_charge();

   // Reset the strip and add the vertex:
   assert(_strip.empty());
   _strip.add(vert);

   // Get in on the drawing action:
   _mesh->drawables().add_uniquely(this);

   // XXX - should use setup(),
   //       taking into account vert is already created.
   set_res_level(res_lev);

   hookup();
}

Bpoint::~Bpoint()
{
   destructor();
 
   // XXX - don't delete map
   //       if you do, make sure curves delete their maps too
   _map = 0;
}

void
Bpoint::setup(int res_lev)
{
   // common functionality for both constructors

   // Require a map function:
   assert(_map);

   // Create the vertex
   Lvert* v = (Lvert*)_mesh->add_vertex(_map->map());
   _mesh->changed();

   // Create the meme and put it in the vertex:
   new PointMeme(this, v);

   // Reset the strip and add the vertex:
   assert(_strip.empty());
   _strip.add(v);

   // Get in on the drawing action:
   _mesh->drawables().add_uniquely(this);

   set_res_level(res_lev);

   hookup();
}

void 
Bpoint::delete_elements() 
{
   // Base class should have it covered
   Bbase::delete_elements();

   _strip.reset();

   assert(_strip.empty());
}

Wvec
Bpoint::norm() const 
{ 
   return _map ? _map->norm()   : Wvec::null(); 
}

Wvec
Bpoint::tan() const 
{ 
   return _map ? _map->tan()    : Wvec::null(); 
}

Wvec
Bpoint::binorm() const 
{ 
   return _map ? _map->binorm() : Wvec::null(); 
}

Wtransf
Bpoint::frame() const 
{ 
   return _map ? _map->frame() : Identity; 
}

Bcurve_list
Bpoint::adjacent_curves() const
{
   // for each edge adjacent to the vertex of the Bpoint,
   // looks for a Bcurve (maybe at another subdivision level)
   // that controls the edge; returns all such Bcurves in a list:
   // XXX - policy re: subdivision levels may change.

   Bcurve_list ret;
   Bcurve* bc = 0;
   Bvert* v = vert();
   for (int i=0; i<v->degree(); i++)
      if ((bc = Bcurve::find_controller(v->e(i))))
         ret += bc;
   return ret;
}

Bcurve* 
Bpoint::other_curve(Bcurve* c) const
{
   // if there are 2 adjacent curves and one is the given curve,
   // return the other
   Bcurve_list nbrs = adjacent_curves();
   if (nbrs.num() != 2)
      return 0;
   if (nbrs[0] == c)
      return nbrs[1];
   if (nbrs[1] == c)
      return nbrs[0];
   return 0;
}

Bcurve*
Bpoint::lookup_curve(CBpoint* p) const
{
   Bcurve_list nbrs = adjacent_curves();
   for (int k=0; k<nbrs.num(); k++)
      if (nbrs[k]->other_point((Bpoint*) p) == this)
         return nbrs[k];
   return 0;
}

void 
Bpoint::set_res_level(int r)
{
   // Set the "resolution level":
   // since Bpoints don't generate children, but instead
   // just set the "corner value" on their vertex, we
   // just adjust the corner value to equal the res level.

   Bbase::set_res_level(r);

   Lvert* lv = vert();
   if (lv)
      lv->set_corner((short)r);
}


void
Bpoint::recompute()
{
   // Compute and apply the update via the PointMeme
   Bbase::recompute();
}

void 
Bpoint::set_selected() 
{
   Bbase::set_selected();
   if (is_skel())
      texbody()->add_post_drawer(this);
}

void 
Bpoint::unselect() 
{
   Bbase::unselect();
   if (is_skel())
      texbody()->rem_post_drawer(this);
}

Bpoint_list 
Bpoint::selected_points()
{
   return Bpoint_list(_selection_list);
}

Bpoint* 
Bpoint::selected_point()
{
   Bpoint_list points = selected_points();
   return (points.num() == 1) ? points[0] : 0;
}

CCOLOR&
Bpoint::selection_color() const 
{
   return Color::blue_pencil_d;
}

CCOLOR&
Bpoint::regular_color() const 
{
   return Color::grey2;
}

bool
Bpoint::should_draw() const
{
   if (!(vert() && _is_shown && vert()->ndc().in_frustum()))
      return false;

   if (is_selected())
      return true;

   if (is_inner_skel())
      return false;

   // if connected to curves/surfaces, don't draw:
   if (vert()->degree() > 0)
      return false;

   return true;
}

static const double BPOINT_SIZE = 8;

double 
Bpoint::standard_size()
{
   // Point size normally used in rendering:
   return BPOINT_SIZE;
}

int
Bpoint::draw(CVIEWptr& v)
{
   // draw to the screen:
   // XXX - should use some kind of stroke?

   update();

   // draw if on-screen
   if (should_draw()) {
      // turn on antialiased and set point size
      GLfloat w = GLfloat(BPOINT_SIZE * v->line_scale());
      GL_VIEW::init_point_smooth(w);
      glDisable(GL_LIGHTING);              // GL_ENABLE_BIT
      glColor3dv(draw_color().data());     // GL_CURRENT_BIT

      // draw with a simple strip callback object:
      GLStripCB cb;
      _strip.draw(&cb);

      GL_VIEW::end_point_smooth();

      // draw shadow

      if (is_selected())
         draw_axes();
      draw_debug();
   }

   return 1;
}

void
Bpoint::draw_debug()
{
   static bool debug = Config::get_var_bool("DEBUG_BPOINT_FRAME",false);
   if (debug)
      draw_axes();
}

void
Bpoint::draw_axes()
{
   const double PIX_LEN = 30;
   const double len     = world_length(loc(), PIX_LEN);

   Wtransf xf = map()->xf();
   Wpt o = xf * Wpt::Origin();
   Wpt x = xf * Wpt(len,0,0);
   Wpt y = xf * Wpt(0,len,0);
   Wpt z = xf * Wpt(0,0,len);

   GLfloat w = GLfloat(1.0 * VIEW::peek()->line_scale());
   GL_VIEW::init_line_smooth(w, GL_CURRENT_BIT);
   glDisable(GL_LIGHTING);      // GL_ENABLE_BIT

   glBegin(GL_LINES);

   // draw x, y, z in red, yellow, blue

   GL_COL(Color::red, 1);       // GL_CURRENT_BIT
   glVertex3dv(o.data());
   glVertex3dv(x.data());

   GL_COL(Color::yellow, 1);    // GL_CURRENT_BIT
   glVertex3dv(o.data());
   glVertex3dv(y.data());

   GL_COL(Color::blue, 1);      // GL_CURRENT_BIT
   glVertex3dv(o.data());
   glVertex3dv(z.data());

   glEnd();

   GL_VIEW::end_line_smooth();
}

int 
Bpoint::draw_vis_ref()
{
   // draw to the visibility reference image (for picking)

   if (!vert())
      return 0;

   if (!_is_shown)
      return 0;

   update();

   // draw if on-screen
   VIEWptr v = VIEW::peek();
   if (vert()->ndc().in_frustum()) {
      // draw this
      ColorIDTexture::draw_verts(&_strip, 6.0);
   }

   return 1;
}

bool 
Bpoint::apply_xf(CWtransf& xf, CMOD& mod)
{
   bool debug = Config::get_var_bool("DEBUG_BBASE_XFORM");
   if (!_map) {
      err_adv(debug, "Bpoint::apply_xf: error: null map");
      return false;
   }
   if (!is_control())
      return true;
   if (!_map->can_transform()) {
      if (debug) {
         cerr << "Bpoint::apply_xf: can't transform map: "
              << _map->class_name() << endl;
      }
      return false;
   }
   _map->transform(xf, mod);
   return true;
}

bool
Bpoint::move_to(CWpt& p) 
{
   // let's try code re-use and hope it doesn't break anything:
   return apply_xf(Wtransf::translation(p - loc()), MOD());
/*
   bool ret = true;
   bool debug = Config::get_var_bool("DEBUG_BPOINT_MOVE",false);
   if (debug && is_control()) {
      print_debug("Bpoint::move_to: before move", get_meme());
   }
   if (!_map) {
      assert(vert());
      vert()->set_loc(p);
   } else if (!_map->set_pt(p) ) {
      err_msg("Bpoint::move_to() Couldn't set new point");
      ret = false;
   }
   // XXX - needed?
//    update();
//    mesh()->changed(BMESH::VERT_POSITIONS_CHANGED);
//    vert()->mark_dirty();

   if (child_point()) {
      err_adv(debug, "Bpoint::move_to: forwarding to child");
      ret = child_point()->move_to(p) && ret;
   }

   if (debug && is_control()) {
      print_debug("Bpoint::move_to: after move", get_meme());
   }
   return ret;
//*/
}

bool
Bpoint::move_to(CXYpt& xy) 
{
   // If we have a shadow, move along its axis.
   // Otherwise move in our plane

   Wpt new_loc;
   PlaneMap* p = PlaneMap::upcast(constraining_surface());
   if (p) {
      new_loc = p->plane().intersect(Wline(xy));
   } else if (has_shadow()) {
      Wline wl(loc(), _shadow_plane.normal());
      new_loc = wl.intersect(Wline(xy));
   } else {
      new_loc = Wplane(loc(), _map->norm()).intersect(Wline(xy));
   }
   
   if (new_loc.in_frustum())
      return move_to(new_loc);
   cerr << "new point was off screen" << endl;
   return false;
}

void 
Bpoint::remove_constraining_surface()
{
  if ( !(constraining_surface()) ){
    cerr << "Bpoint::remove_constraining_surface() "
         << "has no surface constraint" << endl;
    return;
  }

  // save the normal
  Wvec n = norm();

  // remove the shadow, if any 
  remove_shadow();
  set_map(new WptMap(loc()), false);

  if (!n.is_null())
    _map->set_norm(n);
}

void 
Bpoint::set_map(Map0D3D* map, bool update_curves)
{
  if (update_curves)
    adjacent_curves().replace_endpt(_map, map);

  unhook();
  //XXX -- should _map be deleted?
  _map = map;
  hookup();
  invalidate();
}

void
Bpoint::notify_vert_changed(VertMeme*)
{
   // probably never happens?
   cerr << "Bpoint::notify_vert_changed" << endl;
}

void
Bpoint::notify_vert_xformed(CWtransf& /* xf */)
{
   // transforming the vertex location generates a callback
   // to the PointMeme, which notifies the Bpoint with this
   // call...

   // reset local coordinate system.
   // the origin will already be reset via:
   //
   //   PointMeme::SimplexData::notify_simplex_changed();
   //     --> Bpoint::notify_vert_changed()
   //
   // so just transform the vectors of the frame.
//   Wpt  o = loc();
//   Wvec x = xf * _frame.X();
//   Wvec y = xf * _frame.Y();
//   _frame = Wtransf(o, x, y);

   // XXX - necessary?
   // invalidate Bnodes that depend on this:
//   invalidate();

   cerr << "Don't think we need this Bpoint::notify_vert_xformed()" << endl;
}

void 
Bpoint::notify_xform(BMESH*, CWtransf& xf, CMOD& mod)
{
   bool debug = Config::get_var_bool("DEBUG_BNODE_XFORM");
   if (is_control() && _map) {
      if (!_map->can_transform()) {
         err_adv(debug,
                 "Bpoint::notify_xform: can't transform the %s map",
                 **_map->class_name());
      } else 
         _map->transform(xf, mod);
   }
}


void
Bpoint::notify_vert_deleted(PointMeme* vd)
{
   // it's okay for this to get called re: one of the subdivision
   // vertices. at this time it would be unexpected for it to get
   // called for the top level vertex.

   // find out what subdivision level the deleted vertex lived at:
   int level = vd->vert()->lmesh()->subdiv_level();

   // clear the strip at that level:
   _strip.clear_subdivision(level);

   if (_strip.empty()) {
      // XXX - this isn't supposed to happen, but we should still
      // handle this better. apparently the TOP LEVEL vertex was
      // deleted, which means someone was messing with OUR vertex.
      err_msg("Bpoint::notify_vert_deleted: warning -- strip is reset");
      _mesh->drawables() -= this;
   }
}

Bnode_list 
Bpoint::inputs() const 
{
   Bnode_list ret = Bbase::inputs();

   if (_map)    ret += (Bnode*) _map;
   return ret;
}


Bpoint* 
Bpoint::hit_point(CNDCpt& p, double pix_radius)
{
   VisRefImage *vis_ref = VisRefImage::lookup(VIEW::peek());
   if (!vis_ref) {
      err_msg("hit_point: error: can't get vis ref image");
      return 0;
   }
   vis_ref->update();
   return lookup(
      (Bvert*)vis_ref->find_near_simplex(p, pix_radius, BpointFilter())
      );
}

void
Bpoint_list::grow_connected(Bpoint* b, Bpoint_list& ret)
{
  if ( !(b && b->vert() && b->vert()->flag()==0) ) {
    return;
  }

  b->vert()->set_flag(1);
  ret += b;

  Bcurve_list cl = b->adjacent_curves();
  for (int i=0; i<cl.num(); i++) {
    grow_connected(cl[i]->other_point(b), ret);
  }
}

Bpoint_list
Bpoint_list::reachable_points(Bpoint* b)
{
  Bpoint_list ret;

  if ( !(b && b->vert() && b->mesh()) ) {
    return ret;
  }

  // Ensure all reachable flags are cleared
  b->mesh()->verts().clear_flags();
  ret.grow_connected(b, ret);

  return ret;
}

/*****************************************************************
 * BsimplexMap:
 *****************************************************************/
bool
BsimplexMap::set_pt(Bsimplex* s, CWvec& bc)
{
   // Do nothing of it's the same
   if (s == _s && bc == _bc)
      return true;

   // Record the new point
   _s = s;
   _bc = bc;

   invalidate(); // tell dependents

   return true;
}

bool
BsimplexMap::set_pt(CWpt& p)
{
   return true;
}

// end of file bpoint.C
