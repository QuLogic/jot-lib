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
 * bbase.C:
 **********************************************************************/
#include "disp/colors.H"        // Color::grey7 etc.
#include "geom/gl_view.H"       // GL_VIEW::draw_pts()
#include "geom/world.H"         // WORLD::is_displayed()
#include "gtex/ref_image.H"     // for VisRefImage::lookup()
#include "mesh/mi.H"
#include "std/config.H"

#include "tess/tex_body.H"
#include "tess/bpoint.H"
#include "tess/bcurve.H"
#include "tess/bsurface.H"

#include "tess/uv_surface.H"         // debug: UVmeme
#include "tess/blending_meme.H"      // debug: BlendingMeme

using namespace mlib;

/**********************************************************************
 * Bbase
 **********************************************************************/
bool            Bbase::_show_memes = Config::get_var_bool("DEBUG_MEMES",false);
egg_timer       Bbase::_fade_timer(0);
Bbase*          Bbase::_last(0);
Bbase_list      Bbase::_selection_list(32);

Bbase::Bbase(CLMESHptr& m) :
   _selected(false),
   _parent(0),
   _child(0),
   _bbase_level(0),
   _res_level(0),
   _is_shown(true),
   _action(0)
{
   set_mesh(m);
   _last = this;
   activate();  // get in active list for smoothing callbacks
}

Bbase::~Bbase() 
{
   // Children don't outlive parents in this world:
   delete_child();

   // Parent may persist, unhook from it:
   set_parent(0);
   set_mesh(0);

   set_action(0);

   // Delete all the vert memes, which also removes them from the
   // vertices they are attached to:
   _vmemes.delete_all();

   if (_last == this)
      _last = 0;
}

bool 
Bbase::is_last() const 
{
   return _last ? (control() == _last->control()) : false;
}

uint 
Bbase::key() 
{
   static uint k = (uint) **static_name();
   return k;
}

void
Bbase::delete_elements()
{
   // Delete all mesh elements controlled by this, and all memes
   // (boss or not).

   // Deleting elements at this level causes finer level mesh
   // elements to be deleted too. But it won't catch the non-boss
   // child memes, so let's get them now:
   if (_child)
      _child->delete_elements();

   // The following works if the Bbase puts vert memes on all the
   // vertices it controls. Deleting a mesh vertex automatically
   // deletes all adjacent edges and faces.

   if (_vmemes.empty())
      return;

   // find verts controlled by this bbase:
   //Bvert_list mine = find_boss_vmemes(_vmemes.verts()).verts();
   Bvert_list mine;
   for (int i = 0; i < _vmemes.num(); i++)
      if (_vmemes[i]->is_boss())
         mine += _vmemes[i]->vert();

   // delete vert memes
   _vmemes.delete_all();

   // delete the verts controlled by this:
   assert(_mesh);
   _mesh->remove_verts(mine);
}

void
Bbase::set_parent(Bbase* parent)
{
   // Hook up to the given parent. In set_res_level() below,
   // calls virtual produce_child() if needed. That's why this
   // method should not be called in the Bbase constructor, where
   // produce_child is not yet defined.

   if (parent == _parent)
      return;

   // If there is an existing parent, we have to undo some stuff:
   if (_parent) {
      _parent = 0;
      _bbase_level = 0;
      assert(_mesh);    // how could we have a parent and no mesh??
      set_mesh(0);
   }

   // If the new parent is null, we're done:
   if (!parent)
      return;

   // get out now before relinquishing control
   exit_drawables();

   // (relinquish control):
   // Record the parent. Bbase::inputs() includes parent.
   _parent = parent;

   // One-child families only in this world:
   assert(_parent->_child == NULL || _parent->_child == this);
   _parent->_child = this;

   // Set each Bbase level to its parent level + 1.
   // (Tells how deep we are in Bbase hierarchy):
   if (_bbase_level != _parent->_bbase_level + 1) {
      for (Bbase* b = this; b != NULL; b = b->_child)
         b->_bbase_level = b->_parent->_bbase_level + 1;
   }

   // Set the mesh:
   assert(_parent->mesh() && _parent->mesh()->subdiv_mesh());
   set_mesh(_parent->mesh()->subdiv_mesh());

   // Set our res level = parent level - 1.
   // (Tells how many further times we can propagate.)
   // Parent needs a positive res level to propagate:
   if (_parent->_res_level > 0)
      set_res_level(_parent->_res_level - 1);
}

void
Bbase::set_mesh(CLMESHptr& mesh)
{
   // Record the new mesh, and also:
   //   For old mesh: get out of drawables and stop observing.
   //   For new mesh: get into drawables and start observing.

   static bool debug = Config::get_var_bool("DEBUG_BNODE_UPDATE", false);

   if (mesh == _mesh)
      return;

   // Get out of old mesh drawables list, and cancel
   // notifications from old mesh:
   if (_mesh) {
      unsubscribe_mesh_notifications(_mesh);

      if (debug) {
         cerr << identifier() << " unsubscribed from mesh notifications" << endl;
      }

      // Control Bbases do their own drawing.
      // They get put in the control mesh drawables list.
      exit_drawables();
   }

   // Set it
   _mesh = mesh;

   // Re-subscribe
   if (_mesh) {
      subscribe_mesh_notifications(_mesh);
      if (debug) {
         cerr << identifier() << " subscribed to mesh notifications" << endl;
      }
      // Control Bbases can do their own drawing.
      // They have to get in the control mesh drawables list.
      enter_drawables();

      BMESH::set_focus(_mesh->cur_mesh(), 0);
   }
}

void
Bbase::set_action(Action* a)
{
   // we can transition from no previously recorded action
   // to one recorded now, or from a previously recorded
   // action to none now.
   if (_action) {
      assert(!a);
      // XXX - should notify old action here
   }
   _action = a;
}

void
Bbase::set_res_level(int r)
{
   static bool debug = Config::get_var_bool("DEBUG_BBASE_RES_LEVEL",false);
   err_adv(debug, "%s::set_res_level %d", **identifier(), r);

   // Set the resolution level:
   // (no negative numbers)
   if (r < 0)
      err_adv(Config::get_var_bool("DEBUG_BBASE_RES_LEVEL",false),
         "Bbase::set_res_level: negative value (%d), ignored",
         r);
   r = max(r,0);

   // no change? no-op
   if (r == _res_level)
      return;

   // set it
   _res_level = r;

   // deal w/ child
   if (_child) {
      // Perhaps the res level has been cut back... e.g. the
      // child may be unwanted now:
      if (_res_level == 0) {
         delete_child(); // XXX - should be undo-able
      } else {
         _child->set_res_level(_res_level - 1);
      }
   } else if (_res_level > 0 && _mesh && _mesh->subdiv_mesh() != NULL) {
      // If we got in the game late, after subdivision has been
      // going on, get our subdiv children set up:

      produce_child();
   }
}

Bbase* 
Bbase::cur_subdiv_bbase() const 
{
   // Return child Bbase (if any) operating on the current subdiv mesh:
   return subdiv_bbase(_mesh->rel_cur_level());
}

bool
Bbase::is_skel() const
{
   TEXBODY* tex = texbody();
   return (tex && tex->is_skel(mesh()));
}

bool
Bbase::is_inner_skel() const
{
   TEXBODY* tex = texbody();
   return (tex && tex->is_inner_skel(mesh()));
}

LMESHptr 
Bbase::get_inflate_mesh() const
{
   TEXBODY* tex = texbody();
   if (!tex) {
      err_msg("Bbase::get_inflate_mesh: Error: geom is non-TEXBODY");
      return 0;
   }

   LMESHptr ret = LMESH::upcast(tex->get_inflate_mesh(mesh()));
   if (!ret) {
      return 0;
   }

   // Use the same subdivision scheme:
   ret->set_subdiv_loc_calc(_mesh->loc_calc()->dup());

   return ret;
}

bool
Bbase::apply_update() 
{
   if (_vmemes.apply_update()) {
      _mesh->changed(BMESH::VERT_POSITIONS_CHANGED);

      // tell the downstream nodes the mesh changed
      outputs().invalidate();

      return 1;
   } else return 0;
}

void 
Bbase::recompute() 
{
   // Recompute vertex positions:
   compute_update();
   apply_update();
}

bool
Bbase::tick()
{
   static bool debug = Config::get_var_bool("DEBUG_BNODE_TICK", false);

   // undisplayed objects stop sucking CPU cycles:
   if (!WORLD::is_displayed(geom()))
      return false;

   // Before recomputing, update upstream nodes
   inputs().update();

   VertMemeList active_memes(_vmemes.num());
   for (int i=0; i<_vmemes.num(); i++) {
      VertMeme* v = _vmemes[i];
      if (v && v->is_boss() && v->is_warm())
         active_memes += v;
   }
   if (debug) {
      cerr << class_name() << "::tick: called for " << identifier()
           << ", with " << active_memes.num() << " active memes" << endl;
   }
   if (!active_memes.do_relax()) {
      return active_memes.is_any_warm();
   }

   outputs().invalidate();
   assert(mesh());
   mesh()->changed(BMESH::VERT_POSITIONS_CHANGED);
   return true;
}

void
Bbase::activate()
{
   Bnode::activate();
   _vmemes.set_hot();
   if (_res_level > 0 && child())
      child()->activate();
}

void
Bbase::deactivate()
{
   Bnode::deactivate();
}

Bbase* 
Bbase::find_controller(CBsimplex* s)
{
   // Find the Bbase owner of this simplex, or if there is none,
   // find the Bbase owner of its subdivision parent (if any),
   // continuing up the subdivision hierarchy until we find a
   // Bbase owner or hit the top trying.

   Bbase* ret = 0;
   while (s && !(ret = find_owner(s)))
      s = get_parent_simplex(s);
   return ret;
      
}

// Templated to find Bbase controllers for a set of Bverts,
// Bedges or Bfaces:
template <class T>
void
_find_controllers(const T& set, Bbase_list& ret)
{
   Bbase* bb=0;
   for (int i=0; i<set.num(); i++)
      if ((bb = Bbase::find_controller(set[i])))
         ret.add_uniquely(bb);
}

Bbase_list 
Bbase::find_controllers(CBvert_list& verts)
{
   Bbase_list ret;
   _find_controllers(verts, ret);
   return ret;
}

Bbase_list 
Bbase::find_controllers(CBedge_list& edges)
{
   Bbase_list ret = find_controllers(edges.get_verts());
   _find_controllers(edges, ret);
   return ret;
}

Bbase_list 
Bbase::find_controllers(CBface_list& faces)
{
   Bbase_list ret = find_controllers(faces.get_edges());
   _find_controllers(faces, ret);
   return ret;
}

Meme* 
Bbase::find_meme(CBsimplex* s) const 
{
   // Returns the meme (if any) put on the simplex by *this* Bbase:

   if (!s)
      return 0;

   // Try the boss meme first:
   Meme* ret = find_boss_meme(s);
   if (ret && ret->bbase() == this)
      return ret;

   // So we're not in control of this simplex, but might still
   // have an inactive meme on it. Search using key == this:
   return (Meme*)s->find_data((uint)this);
}

VertMemeList 
Bbase::find_boss_vmemes(CBvert_list& verts) 
{
   // Convenience: lookup boss memes for a whole list of vertices

   VertMemeList ret(verts.num());
   for (int i=0; i<verts.num(); i++) {
      VertMeme* vm = find_boss_vmeme(verts[i]);
      if (vm)
         ret += vm;
   }
   return ret;
}

EdgeMemeList 
Bbase::find_boss_ememes(CBedge_list& edges) 
{
   // Convenience: lookup boss memes for a whole list of edges

   EdgeMemeList ret(edges.num());
   for (int i=0; i<edges.num(); i++) {
      EdgeMeme* vm = find_boss_ememe(edges[i]);
      if (vm)
         ret += vm;
   }
   return ret;
}

// Templated to find Bbase owners for a set of Bverts,
// Bedges or Bfaces:
template <class T>
void
_find_owners(const T& set, Bbase_list& ret)
{
   Bbase* bb=0;
   for (int i=0; i<set.num(); i++)
      if ((bb = Bbase::find_owner(set[i])))
         ret.add_uniquely(bb);
}

Bbase_list 
Bbase::find_owners(CBvert_list& verts)
{
   Bbase_list ret;
   _find_owners(verts, ret);
   return ret;
}

Bbase_list 
Bbase::find_owners(CBedge_list& edges)
{
   Bbase_list ret = find_owners(edges.get_verts());
   _find_owners(edges, ret);
   return ret;
}

Bbase_list 
Bbase::find_owners(CBface_list& faces)
{
   Bbase_list ret = find_owners(faces.get_edges());
   _find_owners(faces, ret);
   return ret;
}

Bbase_list 
Bbase::find_owners(CLMESHptr& mesh)
{
   Bbase_list ret;
   ret.set_unique();
   if (mesh) {
      ret += find_owners(mesh->verts());
      ret += find_owners(mesh->edges());
      ret += find_owners(mesh->faces());
   }
   return ret;
}

// Templated to find Bbase owner of a set of Bverts,
// Bedges or Bfaces:
template <class T>
Bbase*
_find_owner(const T& set)
{
   Bbase_list owners = Bbase::find_owners(set);
   return (owners.num() == 1) ? owners[0] : 0;
}

Bbase*
Bbase::find_owner(CBvert_list& verts)
{
   return _find_owner(verts);
}

Bbase*
Bbase::find_owner(CBedge_list& edges)
{
   return _find_owner(edges);
}

Bbase*
Bbase::find_owner(CBface_list& faces)
{
   return _find_owner(faces);
}


VertMeme*
Bbase::add_vert_meme(VertMeme* v) 
{
   if (v && v->bbase() == this) {
      _vmemes += v;
      return v;
   }
   return 0;
}

void 
Bbase::rem_vert_meme(VertMeme* v) 
{
   if (!v)
      err_msg("Bbase::rem_vert_meme: Error: meme is nil");
   else if (v->bbase() != this)
      err_msg("Bbase::rem_vert_meme: Error: meme owner not this");
   else {
      _vmemes -= v;
      delete v;
      err_adv(Config::get_var_bool("DEBUG_MEME_DESTRUCTOR",false),
         "%s::rem_vert_meme: %d left at level %d",
         **identifier(),
         _vmemes.num(),
         bbase_level()
         );
   }
}

void
Bbase::add_face_memes(CBface_list& faces)
{
   if (!(_mesh && _mesh == faces.mesh())) {
      err_msg("Bbase::add_face_memes: Error: bad mesh");
      return;
   }

   for (int i=0; i<faces.num(); i++)
      add_face_meme((Lface*)faces[i]);
}

void
Bbase::add_edge_memes(CBedge_list& edges)
{
   if (!(_mesh && _mesh == edges.mesh())) {
      err_msg("Bbase::add_edge_memes: Error: bad mesh");
      return;
   }

   for (int i=0; i<edges.num(); i++)
      add_edge_meme((Ledge*)edges[i]);
}

inline COLOR
meme_color(VertMeme* vm, CCOLOR& hot_color, CCOLOR& cold_color)
{
   if (vm->do_debug())
      return Color::green;
   return interp(cold_color, hot_color, vm->heat());
}

void
Bbase::show_memes(CCOLOR& hot_color, CCOLOR& cold_color, float point_size)
{
   // Show where the memes are and what state they're in

   if (!_show_memes)
      return;

   Bbase* cur = cur_subdiv_bbase();
   if (!cur)
      return;

   const VertMemeList& vam = cur->vmemes();
   // Enable point smoothing and push gl state:
   GL_VIEW::init_point_smooth(point_size, GL_CURRENT_BIT);
   glDisable(GL_LIGHTING);           // GL_ENABLE_BIT
   glBegin(GL_POINTS);
   for (int i=0; i<vam.num(); i++) {
      if (vam[i]->is_boss()) {
         // GL_CURRENT_BIT
         glColor4fv(float4(meme_color(vam[i], hot_color, cold_color)));
         glVertex3dv(vam[i]->loc().data());
      }
   }
   glEnd();
   GL_VIEW::end_point_smooth();      // pop state
}

/*****************************************************************
 * SELECTION / TIMEOUT / COLOR
 *****************************************************************/
COLOR 
Bbase::draw_color() 
{
   // Return the color to use for drawing based on whether this
   // is selected, or if the selection already wore off or is
   // wearing off:

   if (!_selected)
      return regular_color();

   // Weight that varies from 0 to 1 over the fade interval:
   double w = fade_weight();

   // If fading is complete we shouldn't be selected:
   if (w == 1) 
      unselect();

   // Return the interpolated color:
   return interp(selection_color(), regular_color(), w);
}

CCOLOR& 
Bbase::selection_color() const 
{
   return Color::grey7;
}

CCOLOR& 
Bbase::regular_color() const 
{
   return Color::black; 
}

void 
Bbase::set_selected() 
{
   // Set selection flag:
   if (!_selected) {
      _selected = true;
      _selection_list += this;
      selection_changed();
   }
   BMESH::set_focus(cur_mesh(), 0);

   // Reset timer for the default timeout
   hold();
}

void 
Bbase::unselect() 
{
   // Unset selection flag:
   if (_selected) {
      _selected = false;
      _selection_list -= this;
      selection_changed();
   }
}

Bbase_list 
Bbase::selected_bbases()
{
   return _selection_list;
}

void
Bbase::deselect_all()
{
   // Work backwards since unselecting each
   // Bbase also removes it from the list:
   for (int i=_selection_list.num()-1; i>=0; i--)
      _selection_list[i]->unselect();
}

TEXBODY* 
Bbase::texbody() const
{
   return TEXBODY::upcast(geom()); 
}

str_ptr
Bbase::identifier() const 
{
   // Identifier used in diagnostic output. Bbase types print their
   // class name and level in the subdivision hierarchy.

   if (is_control())
      return Bnode::identifier();
   return Bnode::identifier() + str_ptr("_sub") + str_ptr(subdiv_level());
}

/*****************************************************************
 * BMESHobs METHODS
 *****************************************************************/
void 
Bbase::notify_merge(BMESH* joined, BMESH* removed) 
{
   // if our mesh got merged into another, record the new mesh
   if (_mesh == removed) {
      assert(LMESH::isa(joined));

      if (Config::get_var_bool("DEBUG_MESH_MERGE",false))
         err_msg("%s::notify_merge: changing meshes...", **identifier());

      set_mesh((LMESH*)joined);
   }
}

void 
Bbase::notify_split(BMESH*, CARRAY<BMESH*>&) 
{
   err_msg("Bbase::notify_split: not implemented");
}

void
Bbase::notify_delete(BMESH*)
{
   err_msg("Bbase::notify_delete: not implemented");
   set_mesh(0);
}

void
Bbase::notify_sub_delete(BMESH*)
{
   // the subdiv mesh got deleted
   err_msg("Bbase::notify_sub_delete: level %d", _bbase_level);

   if (_child)
      delete_child();
}

void 
Bbase::notify_subdiv_gen(BMESH*) 
{
   if (Config::get_var_bool("DEBUG_BBASE_SUBDIV_GEN",false))
      err_msg("%s::notify_subdiv_gen: at res level %d",
              **identifier(), _res_level);

   // If res level > 0, produce a child Bbase 
   // of the correct type:
   if (_res_level > 0)
      produce_child();
}

void 
Bbase::notify_update_request(BMESH* m) 
{
   // The mesh wants to be updated, so update it:

   if (_dirty && Config::get_var_bool("DEBUG_BBASE_UPDATE",false)) {
      cerr << identifier() << ": update request (level "
           << _mesh->subdiv_level() << ")" << endl;
   }
   assert(_mesh == m);
   update();
}


Bnode_list 
Bbase::inputs() const 
{
  return Bnode_list();
}


/*****************************************************************
 * BbaseFilter:
 *
 *      Return the simplex that "controls" the given simplex
 *      (directly or via subdivision).
 *
 *      Used in Bbase::hit_bbase(), below.
 *
 *****************************************************************/
class BbaseFilter : public SimplexFilter {
 public:
   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const {
      return Bbase::find_controller(s) != 0;
   }
};

Bbase* 
Bbase::hit_bbase(CNDCpt& p, double rad, Wpt& hit, Bsimplex** simplex)
{
   // Get the visibility reference image:
   VisRefImage *vis_ref = VisRefImage::lookup(VIEW::peek());
   if (!vis_ref) {
      err_msg("Bbase::hit_bbase: Error: can't get vis ref image");
      return 0;
   }
   vis_ref->update();

   // Find the nearest simplex controlled by a Bbase in the
   // search region, and get the Bbase:
   Bsimplex* s = vis_ref->find_near_simplex(p, rad, BbaseFilter());
   Bbase* ret = find_controller(s);
   if (ret) {
      // fill in the hit point itself
      double dist, d2d; Wvec n; // dummies
      s->view_intersect(p, hit, dist, d2d, n);
      if (simplex)
         *simplex = s;
   }
   return ret;
}

/*****************************************************************
 * VertMemeList::print_debug:
 *
 *  put this here to access static_name() of various Meme
 *  subclasses
 *****************************************************************/
void
VertMemeList::print_debug(int iter)
{
   cerr << "\t";
   if (iter >= 0)
      cerr << iter << ": ";
   int n = meme_count(CurveMeme::static_name());
   if (n > 0)
      cerr << n << " curve, ";
   n = meme_count(BlendingMeme::static_name());
   if (n > 0)
      cerr << n << " panel, ";
   n = meme_count(UVmeme::static_name());
   if (n > 0)
      cerr << n << " uv, ";
   err_msg("%3d total", _num);
}

// end of file bbase.C
