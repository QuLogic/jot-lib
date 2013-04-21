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
 * tex_body.C:
 **********************************************************************/
#include "disp/ray.H"
#include "geom/gl_view.H"
#include "geom/texturegl.H"
#include "geom/world.H"
#include "gtex/glsl_toon.H"
#include "gtex/glsl_halo.H"
#include "mesh/lmesh.H"
#include "net/io_manager.H"

#include "tex_body.H"
#include "panel.H"
#include "action.H"

using namespace mlib;

//******** STATIC MEMBER DATA ********
TEXBODY*   TEXBODY::null          = 0;
TAGlist*   TEXBODY::_texbody_tags = 0;
TAGlist*   TEXBODY::_action_tags  = 0;
TEXBODYptr TEXBODY::_focus;

HaloBase* HaloBase::_instance    = 0;

static int foo = DECODER_ADD(TEXBODY);

//******** CONSTRUCTORS ********
TEXBODY::TEXBODY() :
   _apply_xf(0),
   _skel_curves_visible(false),
   _mesh_file(NULL_STR),
   _mesh_update_file(NULL_STR),
   _script(0)
{
   // disallow a mesh from being added to the list twice
   _meshes.set_unique();
}

TEXBODY::TEXBODY(CGEOMptr& geom, Cstr_ptr& name) :
   GEOM(geom, WORLD::unique_name(name)),
   _apply_xf(0),
   _skel_curves_visible(false),
   _mesh_file(NULL_STR),
   _mesh_update_file(NULL_STR),
   _script(0)
{
   // disallow a mesh from being added to the list twice
   _meshes.set_unique();

   if (geom) {
      // copy the shape:
      if (geom->body()) {
         // copy it into an LMESH and add it to the representations
         BMESHptr mesh = new LMESH;
         *mesh = *geom->body();
         add
            (mesh);
      } else {
         err_msg("TEXBODY::TEXBODY: geom has null body");
      }

      // copy the transform:
      set_xform(geom->xform());
   }
}

TEXBODY::TEXBODY(CBMESHptr& m, Cstr_ptr& name) :
   GEOM(WORLD::unique_name(name)),
   _apply_xf(0),
   _skel_curves_visible(false),
   _mesh_file(NULL_STR),
   _mesh_update_file(NULL_STR),
   _script(0)
{
   // disallow a mesh from being added to the list twice
   _meshes.set_unique();

   // add the mesh to the representations
   if (m)
      add(m);
}

TEXBODY::~TEXBODY()
{
   // stop observing self
   end_xform_obs();
   if (_focus == this)
      set_focus(0);
}

Script* 
TEXBODY::get_script() 
{
   if (!_script)
      _script = new Script();
   assert(_script);
   return _script;
}

bool 
TEXBODY::needs_blend() const 
{
   // Need blending if any of the meshes are partially transparent,
   // or if haloing is enabled:
   return (_meshes.min_alpha() < 1 || _do_halo_view);
}

void
TEXBODY::display(CBMESHptr& mesh, MULTI_CMDptr cmd)
{
   // Create the TEXBODY with a (probably) unique name

   if (!mesh) {
      err_msg("TEXBODY::display: error: null mesh");
      return;
   }

   // See if the mesh already has a TEXBODY
   if (bmesh_to_texbody(mesh))
      return; // all done

   // If not, create one
   TEXBODYptr tex = new TEXBODY(mesh, mesh->get_name().c_str());

   // Put it in the WORLD's DRAWN and EXIST lists:
   WORLD::create(tex, !cmd);
   if (cmd)
      cmd->add(new DISPLAY_CMD(tex));
}

void
TEXBODY::undisplay(CBMESHptr& mesh, MULTI_CMDptr cmd)
{
   if (!mesh) {
      err_msg("TEXBODY::undisplay: error: null mesh");
      return;
   }

   GELptr gel = bmesh_to_gel(mesh);
   if (gel) {
      WORLD::undisplay(gel, !cmd);
      if (cmd)
         cmd->add
            (new UNDISPLAY_CMD(gel));
   } else {
      err_msg("TEXBODY::undisplay: warning: mesh has no GEL");
   }
}

void
TEXBODY::init_xform_obs()
{
   // signup for notification when GEOM::xform changes

   // need to lock the REFptr in case this is called with a
   // current ref count of zero
   REFlock lock(this)
      ;
   xform_obs(this);
}

void
TEXBODY::end_xform_obs()
{
   // get out of the xform notification list

   REFlock lock(this)
      ;
   unobs_xform(this);
}

CBMESHptr&
TEXBODY::cur_rep() const
{
   // This method exists for convenience and backwards compatibility.
   // Returns the last draw-enabled mesh (if any), or a null pointer.

   for (int i=_meshes.num()-1; i>=0; i--)
      if (_meshes[i]->draw_enabled())
         return _meshes[i];

   static BMESHptr null_ptr; // needed to return a reference
   null_ptr = 0;
   return null_ptr;
}

bool
TEXBODY::add(BMESHptr m)
{
   if (!m) {
      err_msg("TEXBODY::add: can't add null mesh");
      return false;
   }
   if (m->geom()) {
      // We require that each mesh has only one GEOM ...
      // so we have to take it away from the other GEOM.

      if (TEXBODY::isa(m->geom())) {
         // we know how to deal with a TEXBODY...
         ((TEXBODY*)m->geom())->remove
            (m);
      } else {
         err_msg("TEXBODY::add: can't add mesh with pre-existing owner");
         return false;
      }
   }
   m->set_geom(this);
   _meshes += m;   // adds uniquely

   if (_meshes.num() == 1) {
      // we just got our first mesh... turn on xform observations.
      //
      // XXX - the following line was in the first TEXBODY
      // constructor, but it was getting called in certain static
      // initialization code before a certain static HASH table was
      // initialized, which didn't work. putting the line here is
      // sort of hacky, but the idea is that a TEXBODY isn't ever
      // used until it gets assigned a mesh, and so it can wait to
      // sign up for xform notifications until after a mesh is
      // assigned...
      init_xform_obs();
   }

   return true;
}

bool
TEXBODY::push(BMESHptr m)
{
   if (!m) {
      err_msg("TEXBODY::add: can't add null mesh");
      return false;
   }
   if (m->geom()) {
      // We require that each mesh has only one GEOM ...
      // so we have to take it away from the other GEOM.

      if (TEXBODY::isa(m->geom())) {
         // we know how to deal with a TEXBODY...
         ((TEXBODY*)m->geom())->remove
            (m);
      } else {
         err_msg("TEXBODY::add: can't add mesh with pre-existing owner");
         return false;
      }
   }
   m->set_geom(this);
   _meshes.push(m);   // adds uniquely

   // see comment in TEXBODY::add
   if (_meshes.num() == 1) {
      init_xform_obs();
   }

   return true;
}

void
TEXBODY::remove(CBMESHptr& m)
{
   // remove the given mesh from the list of representations.

   if (_meshes.empty()) {
      err_msg("TEXBODY::remove: list is already empty");
   } else if (_meshes -= m) {
      // turn off xform observations if there are no more meshes
      if (_meshes.empty())
         end_xform_obs();
   } else {
      err_msg("TEXBODY::remove: unknown mesh");
   }
}

BMESH_list
TEXBODY::all_drawn_meshes()
{
   // Get all currently displayed meshes:
   // (static method)

   BMESH_list ret;
   for (int i=0; i<DRAWN.num(); i++) {
      TEXBODY* tex = upcast(DRAWN[i]);
      if (tex)
         ret += tex->meshes();
   }
   return ret;
}

inline bool
is_just_pts(BMESHptr& m)
{
   return m && m->is_points() && !(m->is_polylines() || m->is_surface());
}

inline bool
is_curves(BMESHptr& m)
{
   return m && m->is_polylines() && !m->is_surface();
}

inline bool
is_surface(BMESHptr& m)
{
   return m && m->is_surface();
}

inline bool
is_panels(BMESHptr& m)
{
   // XXX - temporary solution
   return is_surface(m) && Panel::find_controller(m->faces()[0]);
}

inline bool
is_skel(BMESHptr& m)
{
   return is_curves(m) || is_just_pts(m) || is_panels(m);
}

bool
TEXBODY::is_inner_skel(BMESH* m) const
{
   // Is it a skeleton enclosed in a surface?
   // (Hence, maybe it can skip drawing??)
   // Here, we are assuming that the surface meshes
   // come later in the list than the skeleton meshes...

   if (!is_skel(m)) return false;
   int i = _meshes.get_index(m);
   if (i<0) return false;
   for (++i; i<_meshes.num(); i++)
      if (_meshes[i]->is_surface())
         return true;
   return false;
}

BMESH_list 
TEXBODY::point_meshes() const
{
   // Returns meshes consisting just of points

   BMESH_list ret;
   for (int i=0; i<_meshes.num(); i++)
      if (is_just_pts(_meshes[i]))
         ret += _meshes[i];
   return ret;
}

BMESH_list 
TEXBODY::curve_meshes() const
{
   // Returns meshes consisting of curves
   // (maybe with points) and no surfaces

   BMESH_list ret;
   for (int i=0; i<_meshes.num(); i++)
      if (is_curves(_meshes[i]))
         ret += _meshes[i];
   return ret;
}

BMESH_list 
TEXBODY::panel_meshes() const
{
   // Returns meshes consisting of panels
   // (maybe with points and curves)

   BMESH_list ret;
   for (int i=0; i<_meshes.num(); i++)
      if (is_panels(_meshes[i]))
         ret += _meshes[i];
   return ret;
}

BMESH_list 
TEXBODY::surface_meshes() const
{
   // Returns meshes consisting of surfaces
   // (maybe with points and curves)

   BMESH_list ret;
   for (int i=0; i<_meshes.num(); i++)
      if (_meshes[i]->is_surface())
         ret += _meshes[i];
   return ret;
}

bool
TEXBODY::has_surfaces() const
{
   for (int i=0; i<_meshes.num(); i++)
      if (_meshes[i]->is_surface())
         return true;
   return false;
}

bool
TEXBODY::has_skels() const
{
   for (int i=0; i<_meshes.num(); i++)
      if (is_skel(_meshes[i]))
         return true;
   return false;
}

BMESHptr
TEXBODY::get_inflate_mesh(BMESHptr mesh)
{
   // Return a mesh suitable to use for building a mesh by
   // inflation from given mesh. The plan is to return the next
   // mesh in the TEXBODY, creating a new one if there is no next
   // one. I.e., the policy is that meshes in the TEXBODY are
   // ordered by their inflation dimension, so to speak.

   if (!mesh) {
      return 0;
   }

   // Find position of mesh in the list:
   int i = _meshes.get_index(mesh);
   if (!_meshes.valid_index(i)) {
      return 0;
   }

   // Try for the next mesh, if any:
   if (_meshes.valid_index(i+1)) {
      return _meshes[i+1];
   }

   // Didn't work, so make a new one:
   if (add(BMESH::upcast(mesh->dup()))) {
      return _meshes.last();
   }
   return 0;
}

int
TEXBODY::draw(CVIEWptr &v)
{
   // Check for existence of at least 1 draw-enabled mesh:
   if (!_meshes.draw_enabled()) {
      // This policy can be annoying, but it only happens
      // when bugs occur, so we're sticking to it
      err_msg("TEXBODY::draw: no meshes worth drawing -- undisplaying");
      WORLD::undisplay(this, false);
      end_xform_obs();
      return 0;
   }

   // draw halo first, if needed:
   if (_do_halo_view)
      draw_halo(v);

   glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);

   // Default policy is we say YES to polygon offset.
   GL_VIEW::init_polygon_offset();  // GL_ENABLE_BIT

   // use glEnable(GL_NORMALIZE) only when needed
   if (!xform().is_orthonormal())
      glEnable (GL_NORMALIZE); // GL_ENABLE_BIT

   // GL_CURRENT_BIT
   GL_COL(_has_color ? _color : COLOR::white, _has_transp ? _transp : 1);

   // set xform:
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glMultMatrixd(xform().transpose().matrix());

   // draw the meshes normally:
   int ret = _meshes.draw(v);

   // now draw the "post drawers", e.g. skeleton
   // curves normally hidden inside the surface meshes.
   // these curves can be drawn now because they are 
   // being edited or whatever:
   if (!_post_drawers.empty()) {
      glDisable(GL_DEPTH_TEST);         // GL_ENABLE_BIT
      _post_drawers.draw(v);
   }
   
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();

   GL_VIEW::end_polygon_offset(); // GL_ENABLE_BIT

   glPopAttrib();

   return ret;
}

int
TEXBODY::draw_vis_ref()
{
   // save opengl state
   glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_TRANSFORM_BIT);

   // set modelview matrix:
   glMatrixMode(GL_MODELVIEW);          // GL_TRANSFORM_BIT
   glPushMatrix();
   glMultMatrixd(xform().transpose().matrix());

   GL_VIEW::init_polygon_offset();  // GL_ENABLE_BIT
   int ret = _meshes.draw_vis_ref();
   GL_VIEW::end_polygon_offset();   // GL_ENABLE_BIT
   if (!_post_drawers.empty()) {
      glDisable(GL_DEPTH_TEST);     // GL_ENABLE_BIT
      _post_drawers.draw_vis_ref();
   }

   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
   glPopAttrib();

   return ret;
}

int
TEXBODY::draw_color_ref(int i)
{
   // add a halo:
   if (_do_halo_ref) {
      glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
      glEnable(GL_BLEND);                                // GL_ENABLE_BIT
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // GL_COLOR_BUFFER_BIT
      draw_halo(VIEW::peek());
      glPopAttrib();
   }

   return GEOM::draw_color_ref(i);
}

/**********************************************************************
 * GEOM METHODS
 **********************************************************************/
RAYhit&
TEXBODY::intersect(RAYhit& ray, CWtransf&, int) const
{
   // XXX - should fix
   //
   // Not using bbox because it misses meshes of lone points.
   // Also, since we're probably using ref image picking it won't
   // be slow anyway.

   for (int i=0; i<_meshes.num(); i++) {
      if (_meshes[i]->draw_enabled()) {
         Wpt    near_pt;
         Wvec   n;
         double d, d2d;
         XYpt   texc;
         _meshes[i]->intersect(ray, xform(), near_pt, n, d, d2d, texc);
      }
   }

   return ray;
}

RAYnear&
TEXBODY::nearest(RAYnear &r, CWtransf&) const
{
   // XXX - should find out what this does
   err_msg("TEXBODY::nearest: not implemented");

   return r;
}

bool
TEXBODY::inside (CXYpt_list& list) const
{
   // XXX - needs work

   CBMESHptr& cur = cur_rep();
   if (!cur) {
      return 0;
   }

   int count = 0;
   for (int i=0; i<cur->nverts(); i++) {
      XYpt pt = xform() * cur->bv(i)->loc();
      count = list.contains(pt) ? count + 1 : count;
   }

   return count > (cur->nverts() * .75);
}

BBOX
TEXBODY::bbox(int) const
{
   return xform() * _meshes.bbox();
}

GEOMptr
TEXBODY::dup(Cstr_ptr &n) const
{
   // See also TEXBODY::dup() below

   // XXX - following copies color, transparency, texture,
   // and constraints that may exist for this GEOM...
   // not necessarily the desirable policy for dup()
   return GEOMptr(new TEXBODY((TEXBODY*)this,n));
}

/**********************************************************************
 * DATA_ITEM VIRTUAL FUNCTIONS
 **********************************************************************/
DATA_ITEM*
TEXBODY::dup() const
{
   return new TEXBODY();
}

static bool  save_actions = Config::get_var_bool("SAVE_ACTIONS", false);
static bool debug_actions = Config::get_var_bool("DEBUG_ACTION", false);
static bool debug_io      = Config::get_var_bool("DEBUG_IO", false);

CTAGlist &
TEXBODY::tags() const
{
   if (save_actions) {
      return action_tags();
   }
   if (!_texbody_tags) {
      _texbody_tags = new TAGlist;
      *_texbody_tags += GEOM::tags();

      *_texbody_tags += new TAG_meth<TEXBODY>(
         "mesh_data",
         &TEXBODY::put_mesh_data,
         &TEXBODY::get_mesh_data, 1);
      *_texbody_tags += new TAG_meth<TEXBODY>(
         "mesh_data_file",
         &TEXBODY::put_mesh_data_file,
         &TEXBODY::get_mesh_data_file, 1);
      *_texbody_tags += new TAG_meth<TEXBODY>(
         "mesh_data_update",
         &TEXBODY::put_mesh_data_update,
         &TEXBODY::get_mesh_data_update, 1);
      *_texbody_tags += new TAG_meth<TEXBODY>(
         "mesh_data_update_file",
         &TEXBODY::put_mesh_data_update_file,
         &TEXBODY::get_mesh_data_update_file, 1);

      //XXX - Next 4 tags are deprected in favor
      //of new file format... Will vanish one day...
      *_texbody_tags += new TAG_meth<TEXBODY>(
         "mesh",
         &TEXBODY::put_mesh,
         &TEXBODY::get_mesh, 1);
      *_texbody_tags += new TAG_meth<TEXBODY>(
         "mesh_file",
         &TEXBODY::put_mesh_file,
         &TEXBODY::get_mesh_file, 1);
      *_texbody_tags += new TAG_meth<TEXBODY>(
         "mesh_update",
         &TEXBODY::put_mesh_update,
         &TEXBODY::get_mesh_update, 1);
      *_texbody_tags += new TAG_meth<TEXBODY>(
         "mesh_update_file",
         &TEXBODY::put_mesh_update_file,
         &TEXBODY::get_mesh_update_file, 1);
   }
   return *_texbody_tags;
}

CTAGlist&
TEXBODY::action_tags() const
{
   if (!_action_tags) {
      _action_tags = new TAGlist;
      *_action_tags += GEOM::tags();

      *_action_tags += new TAG_meth<TEXBODY>(
         "script",
         &TEXBODY::put_script,
         &TEXBODY::get_script, 1);
   }
   return *_action_tags;
}

void
TEXBODY::put_script(TAGformat &d) const
{
   if (!has_script())
      return;
   assert(_script);

   // If saving an update .jot file (i.e. a frame of animation),
   // don't write out the action list.
   if (IOManager::state() == IOManager::STATE_PARTIAL_SAVE) {
      err_adv(debug_actions, "TEXBODY::put_script: skipping partial save...");
      return;
   }
   if (debug_actions) {
      cerr << "TEXBODY::put_script: writing "
           << _script->num() << " actions" << endl;
   }
   d.id();
   _script->format(*d);
   d.end_id();
}

void
TEXBODY::get_script(TAGformat &d)
{
   // read past the Script class name:
   str_ptr n;
   *d >> n;
   assert(n == Script::static_name());

   // XXX - hack: need to set up 2 meshes before reading Actions.
   //       maybe number of meshes can be instead be read from file.
   if (meshes().empty()) {
      add(create_mesh("skeleton"));
      add(create_mesh("surface"));
   }

   // set this as current TEXBODY:
   TEXBODYptr tx = get_focus(); // store the old one
   set_focus(this);

   get_script()->decode(*d);
   if (debug_actions) {
      cerr << "TEXBODY::get_script: read "
           << _script->num() << " actions, invoking..." << endl;
   }
   if (_script->can_invoke()) {
      _script->invoke();
   } else {
      err_adv(debug_actions, "TEXBODY::get_actions: can't invoke");
   }

   // restore previous focus:
   set_focus(tx);
}

bool 
TEXBODY::has_script() const 
{
   // is there a non-empty script?
   return _script && !_script->empty(); 
}

void 
TEXBODY::add_action(Action* a) 
{
   // add an action to the script:
   assert(a); get_script()->add(a); 
}

void
TEXBODY::get_mesh_data_file(TAGformat &d)
{
   BMESHptr cur = cur_rep();
   if (!cur)
      add
         (cur = new LMESH);
   assert(cur);

   str_ptr filename;
   *d >> filename;

   if (filename == "NULL_STR") {
      _mesh_file = NULL_STR;
      if (debug_io)
         err_msg("TEXBODY::get_mesh_data_file - Found NULL_STR");
   } else {
      _mesh_file = filename;
      str_ptr fname = IOManager::load_prefix() + _mesh_file;
      if (debug_io) {
         cerr << "TEXBODY::get_mesh_data_file: file: "
              << filename
              << ", path: "
              << fname << endl;
      }
      cur->read_file(**fname);
   }
}

void
TEXBODY::put_mesh_data_file(TAGformat &d) const
{

   // If saving an update .jot file (i.e. a frame of animation),
   //don't save the whole set of mesh data, wait for the
   //put_mesh_data_update or put_mesh_data_update_file tags...
   if (IOManager::state() == IOManager::STATE_PARTIAL_SAVE)
      return;

   if (_mesh_file == NULL_STR) {
      err_msg("TEXBODY::put_mesh_data_file - Writing NULL_STR to file.");
      d.id();
      *d << str_ptr("NULL_STR");
      d.end_id();
   } else {
      err_msg("TEXBODY::put_mesh_data_file - Writing '%s' to file",
               **_mesh_file);
      d.id();
      *d << _mesh_file;
      d.end_id();

      str_ptr fname = IOManager::save_prefix() + _mesh_file;

      err_msg("TEXBODY::put_mesh_data_file - Exporting mesh data to '%s'...",
               **fname);

      BMESHptr cur = cur_rep();
      if (!(cur && cur->write_file(**fname))) {
         err_msg("TEXBODY::put_mesh_data_file - Export FAILED to '%s'!!!", **fname);
      }
   }
}

void
TEXBODY::get_mesh_data(TAGformat &d)
{
   if (debug_io)
      err_msg("TEXBODY::get_mesh_data - Loading embedded mesh...");

   BMESHptr cur = cur_rep();
   if (!cur)
      add
         (cur = new LMESH);
   assert(cur);

   str_ptr str;
   *d >> str;

   DATA_ITEM *data_item = DATA_ITEM::lookup(str);

   if (!data_item) {
      err_msg("TEXBODY::get_mesh_data() - Class name '%s' could not be found!!!!!!!", **str);
      return;
   }
   if (!BMESH::isa(data_item)) {
      err_msg("TEXBODY::get_mesh_data() - Class name '%s' not a 'BMESH' or a derived class",
              **str);
      return;
   }

   cur->delete_elements();
   cur->decode(*d);

   //Implictly, _mesh_file should be NULL_STR since we loaded
   //the mesh data imbedded in the stream, not from external file
   //_mesh_file = NULL_STR;
}

void
TEXBODY::put_mesh_data(TAGformat &d) const
{
   // If saving an update .jot file (i.e. a frame of animation),
   //don't save the whole set of mesh data, wait for the
   //put_mesh_data_update or put_mesh_data_update_file tags...
   if (IOManager::state() == IOManager::STATE_PARTIAL_SAVE)
      return;
        
   BMESHptr cur = cur_rep();
   if (cur && (_mesh_file == NULL_STR)) {
      err_msg("TEXBODY::put_mesh_data() - Writing embedded mesh data.");

      d.id();
      cur->format(*d);
      d.end_id();
   } else {
      //Do nothing, put_mesh_data_file() will deal...
   }
}

void
TEXBODY::get_mesh_data_update_file(TAGformat &d)
{

   BMESHptr cur = cur_rep();
   if (!cur)
      add
         (cur = new LMESH);
   assert(cur);

   *d >> _mesh_update_file;

   str_ptr fname = IOManager::load_prefix() + _mesh_update_file;

   cur->read_file(**fname);
}

void
TEXBODY::put_mesh_data_update_file(TAGformat &d) const
{

   // If saving an update .jot file (i.e. a frame of animation),
   // save the mesh update data here... (if it's in external file)
   if (IOManager::state() != IOManager::STATE_PARTIAL_SAVE)
      return;

   if (_mesh_update_file == NULL_STR) {}
   else {
      err_msg("TEXBODY::put_mesh_data_update_file() - Writing '%s' to file.", **_mesh_update_file);

      d.id();
      *d << _mesh_update_file;
      d.end_id();

      str_ptr fname = IOManager::save_prefix() + _mesh_update_file;

      err_msg("TEXBODY::put_mesh_data_update_file() - Exporting mesh data to '%s'...", **fname);

      BMESHptr cur = cur_rep();
      if (!(cur && cur->write_file(**fname))) {
         err_msg("TEXBODY::put_mesh_data_update_file - Export FAILED to '%s'!!!", **fname);
      }
   }


}

void
TEXBODY::get_mesh_data_update(TAGformat &d)
{   
   BMESHptr cur = cur_rep();
   if (!cur)
      add(cur = new LMESH);
   assert(cur);

   str_ptr name;
   *d >> name;
   if (name != LMESH::static_name()) {
      cerr << "TEXBODY::get_mesh_data_update: is not an LMESH" << endl;
      return;
   }

   cur->decode(*d);

   //If the data's embedded in the TEXBODY, we're not expecting
   //an external file reference...
   _mesh_update_file = NULL_STR;
}

void
TEXBODY::put_mesh_data_update(TAGformat &d) const
{
   // If saving an update .jot file (i.e. a frame of animation),
   // save the mesh update data here... (if it's not in an external file)
   if (IOManager::state() != IOManager::STATE_PARTIAL_SAVE)
      return;

   BMESHptr cur = cur_rep();

   if (cur && (_mesh_update_file == NULL_STR)) {
      err_msg("TEXBODY::put_mesh_data_update() - Writing embedded mesh data update.");

      d.id();
      cur->format(*d);
      d.end_id();
   } else {
      //Do nothing, put_mesh_data_update_file() will deal...
   }
}





// XXX - Next 8 methods cover deprecated file format support
// Will vanish one day...
void
TEXBODY::get_mesh_file(TAGformat &d)
{
   BMESHptr cur = cur_rep();
   if (!cur)
      add
         (cur = new LMESH);
   assert(cur);


   str_ptr filename;
   *d >> filename;

   if (filename == "NULL_STR") {
      _mesh_file = NULL_STR;
      err_msg("TEXBODY::get_mesh_file() - Found NULL_STR");
   } else {
      _mesh_file = filename;
      err_msg("TEXBODY::get_mesh_file() - Found '%s'", **filename);
      cur->read_file(**_mesh_file);
   }
}

void
TEXBODY::put_mesh_file(TAGformat &d) const
{
   // deprecated
}

void
TEXBODY::get_mesh(TAGformat &d)
{
   err_msg("TEXBODY::get_mesh() - Loading embedded mesh...");

   BMESHptr cur = cur_rep();
   if (!cur)
      add
         (cur = new LMESH);
   assert(cur);

   // XXX -- Assuming this is an ASCII stream
   assert((*d).ascii());
   cur->read_stream(*(*d).istr());

   //Implictly, _mesh_file should be NULL_STR since we loaded
   //the mesh data imbedded in the stream, not from external file
   //_mesh_file = NULL_STR;
}

void
TEXBODY::put_mesh(TAGformat &d) const
{
   // XXX - Deprecated
}

void
TEXBODY::get_mesh_update_file(TAGformat &d)
{

   BMESHptr cur = cur_rep();
   if (!cur)
      add
         (cur = new LMESH);
   assert(cur);

   str_ptr filename;
   *d >> filename;

   cur->read_update_file(**filename);
}

void
TEXBODY::put_mesh_update_file(TAGformat &d) const
{
   /* XXX - Deprecated
   //XXX - This tag is read only 
   */
}

void
TEXBODY::get_mesh_update(TAGformat &d)
{

   BMESHptr cur = cur_rep();
   if (!cur)
      add
         (cur = new LMESH);
   assert(cur);

   // XXX -- Assuming this is an ASCII stream
   assert((*d).ascii());

   cur->read_update_stream(*(*d).istr());

}

void
TEXBODY::put_mesh_update(TAGformat &d) const
{
   /* XXX - Deprecated
   //XXX - This tag is read only 
   */
}

/*************************************************************************
 * XFORMobs VIRTUAL FUNCTIONS
 *************************************************************************/
void
TEXBODY::notify_xform(CGEOMptr& geom, STATE state)
{
   // GEOM::_xform was changed.
   //
   // If _apply_xf == true, xform() != identity, and 'state' is
   // XFORMobs::END (meaning this is the end of the manipulation),
   // then apply the xform directly to each mesh and reset
   // GEOM::_xform to Identity.

   // make sure the geom == this (should always be true)
   if (geom != this) {
      err_msg("TEXBODY::notify_xform: error: %s geom",
              geom ? "unknown" : "null");
      return;
   }

   CWtransf& xf = xform();
   if (state == XFORMobs::END && _apply_xf && !xf.is_identity()) {
      static bool debug = Config::get_var_bool("DEBUG_TEXBODY_XFORM",false);
      if (debug) {
         cerr << "TEXBODY::notify_xform: applying xform:"
              << endl << xf << endl;
      }
      MOD::tick();
      _meshes.transform(xf);
      _xform  = Identity;
      _inv_xf = Identity;
      _inv_xf_dirty = false;
   }
}

static void
draw_sphere()
{
   // Draw a sphere via GLU
   // calling gluNewQuadric during static initialization
   // on Mac OS X causes a seg fault; so initialize to 0:
   static GLUquadric* quad = 0;
   if (!quad) {
      quad = gluNewQuadric();
   }
   assert(quad != 0);
   double rad = 1.0;    // radius of sphere
   GLint slices = 24;   // these numbers control the
   GLint stacks = 16;   //   tessellation of the sphere
   gluSphere(quad, rad, slices, stacks);
}

static TEXTUREglptr
toon_tex()
{
   static TEXTUREglptr ret;
   if (!ret) {
      ret = new TEXTUREgl(
         GtexUtil::toon_name(Config::get_var_str("HALO_TEX","halo_white2.png"))
         );
      ret->set_wrap_s(GL_CLAMP_TO_EDGE);
      ret->set_wrap_t(GL_CLAMP_TO_EDGE);
   }
   return ret;
}

int
TEXBODY::draw_halo(CVIEWptr &v) const
{
   if (!body())
      return 0;
   BBOX bb = bbox(); // NOTE: bbox is in world coordinates
   if (!bb.valid())
      return 0;

   bool use_glu = false;
   // set this environment variable to use GLSLHaloShader instead of HaloSphere
   bool _use_smaller_halo = Config::get_var_bool("USE_SMALLER_HALOS", false);
   if (use_glu) {
      // Activate GLSL program for toon shading
      GLSLToonShader::draw_start(toon_tex(), Color::white, 1.0, true);

      glDisable(GL_DEPTH_TEST); // GL_ENABLE_BIT

      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      const double scale = 1.2;
      Wtransf tran = Wtransf(bb.center()) * Wtransf::scaling(bb.dim()*scale);
      glMultMatrixd(tran.transpose().matrix());

      draw_sphere();
 
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();

      GLSLToonShader::draw_end();
   } else if (_use_smaller_halo){
      // draw current representation using GLSLHaloShader
      CBMESHptr& cur = cur_rep();
      CPatch_list& local_patches = cur->patches();

      glPushAttrib(GL_ENABLE_BIT);
      glDisable(GL_DEPTH_TEST); // GL_ENABLE_BIT
      
      // set xform so halo moves along with its model:
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glMultMatrixd(xform().transpose().matrix());

      GLSLHaloShader* halo;
      for(int i = 0; i < local_patches.num(); i++)
      {
         // we get the version of each patch as drawn with the Halo shader
         halo = get_tex<GLSLHaloShader>(local_patches[i]);
         assert(halo);
         halo->draw(v);
      }
      
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
      glPopAttrib();
   } else {
      if (HaloBase::get_instance()) {               
         HaloBase::get_instance()->draw_halo(v, bb, 0.75);
      }
   }
   return 1; // or whatever...
}

TEXBODYptr 
TEXBODY::get_ffs_tex_body(MULTI_CMDptr cmd)
{
   TEXBODYptr ret = TEXBODY::cur_tex_body();
   if (!ret) {
      ret = new TEXBODY(BMESHptr(0), "ffs");
      ret->add(create_mesh("skeleton"));
      ret->add(create_mesh("surface"));
      WORLD::create(ret, !cmd);
      if (cmd) {
         cmd->add(new DISPLAY_CMD(ret));
      }
      ret->set_apply_xf(1); // FFS TEXBODYs keep the identity xform
   }
   return ret;
}

LMESHptr
TEXBODY::get_ffs_mesh(mesh_t mesh_id, MULTI_CMDptr cmd)
{
   TEXBODYptr c = get_ffs_tex_body(cmd);
   assert(c);
   CBMESH_list& m = c->meshes();
   assert(m.num() == NUM_MESH_T);
   LMESHptr ret = LMESH::upcast(m[mesh_id]);
   assert(ret);
   return ret;
}

LMESHptr
TEXBODY::create_mesh(const string& base_name)
{
   LMESHptr ret = new LMESH;

   // Assign a unique name to the mesh:
   assert(ret && !ret->has_name());
   string base = (base_name == "") ? "ffs-mesh" : base_name;
   ret->set_unique_name(base);

   // Use the Hybrid subdivision, optionally w/ volume preservation
   bool vp = !Config::get_var_bool("JOT_NO_VOL_PRESERVE",false);
   ret->set_subdiv_loc_calc(vp ? new HybridVolPreserve : new HybridLoc);

   // Make this mesh the "focus"
   BMESH::set_focus(ret, 0);

   return ret;
}

// end of file tex_body.C
