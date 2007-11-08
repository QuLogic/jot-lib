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
 *  \file patch.C
 *  \brief Contains the implementation of the Patch class.
 *
 *  \sa patch.H
 *
 */

#include "ioblock.H"
#include "gtexture.H"
#include "bfilters.H"
#include "patch.H"
#include "std/config.H"
#include "sps/sps.H"

// example defined in src/jot.C:
init_fade_func_t Patch::_init_fade = 0;

TAGlist*        Patch::_patch_tags      = 0;
int             Patch::_next_stencil_id = 0;
Patch*          Patch::_focus           = 0;

// permits d2d stuff to be written out to file when saving:
static const bool patch_d2d =
   Config::get_var_bool("ENABLE_PATCH_D2D",true);

static const bool debug_samples =
   Config::get_var_bool("DEBUG_D2D_SAMPLES",false);

//********************** MANAGERS **********************
Patch::Patch(BMESH* mesh) :
   _mesh(mesh),
   _faces(0),
   _creases(0),
   _borders(0),   
   _textures(0),
   _cur_tex_i(0),
   _non_tex_i(0),
   _prev_tex(0),
   _pixels(0),
   _stamp(0),
   _mesh_version(0),
   _tri_strips_dirty(1),
   _tex_coord_gen(0),
   _data(0),
   _stencil_id(0),
   _sps_height(5),
   _sps_min_dist(0.35),
   _sps_regularity(20.0),
   _sample_spacing(0),
   _scale(1.0),
   _o(PIXEL(0,0)),
   _u_o(PIXEL(1,0)),
   _v_o(PIXEL(0,1)),
   _dynamic_stamp(UINT_MAX),
   _do_dynamic_stuff(true),
   _do_lod(true),
   _do_rotation(true),
   _lod_t(0),
   _target_scale(1.0),
   _down_lim(0.8),
   _up_lim(1.4),
   _transition_duration(0.4),
   _timed_lod_hi(2.0),
   _timed_lod_lo(1.0),
   _use_timed_lod_transitions(false),
   _transition_has_started(false),
   _saw_periodic_sync(false),
   _use_direction_vec(false),
   _use_weighted_ls(true),
   _use_visibility_test(true),
   _d2d_samples_valid(false)
{
   _textures.set_unique();

   //XXX - By default, APPEARs will look white if no
   //      color is defined, so let's not pollute the
   //      .jot files with yet more gunk...
   //set_color(COLOR::white);

   if (_mesh)
     _mesh->changed(BMESH::RENDERING_CHANGED);

   set_name(str_ptr("patch-") + str_ptr(_mesh->patches().num()));
}

Patch::~Patch()
{
   //XXX - Moved this up here...
   _textures.delete_all();

   // Get out of the patch list if needed:
   if (_mesh) _mesh->unlist(this);
   _mesh = 0;

   // The patch owns its tri_strips, crease strips, and Gtexures, 
   // but not its faces, edge strips, or vert strips. So it deletes
   // the former but not the latter.
   while (!_tri_strips.empty())
      delete _tri_strips.pop();
   delete _creases;
   delete _borders;   

   //XXX - Moved this higher so that GTextures that do things
   //      like unobserving meshes on destruction, still have
   //      a valid _mesh pointer...
   //_textures.delete_all();
   
   for (int i=0; i<_faces.num(); i++) {
      _faces[i]->set_patch(0);
      _faces[i]->set_patch_index(-1);
   }

   if (is_focus(this))
      set_focus(0);
}

int 
Patch::subdiv_level() const 
{
   return _mesh ? _mesh->subdiv_level() : 0; 
}

int 
Patch::rel_edit_level()
{
   return _mesh ? (_mesh->edit_level() - subdiv_level()) : 0; 
}

uint 
Patch::stamp() 
{
   if (_mesh && _mesh->version() != _mesh_version) {
      _mesh_version = _mesh->version();
      changed();
   }
   return _stamp;
}

void 
Patch::set_focus(Patch* p, bool use_control)
{
   _focus = use_control ? get_ctrl_patch(p) : p;

   static bool debug = Config::get_var_bool("DEBUG_PATCH_FOCUS",false);
   if (debug && _focus) {
      cerr << "Patch::set_focus: mesh: "
           << _focus->mesh()->get_name()
           << ", patch: " << _focus->mesh()->patches().get_index(_focus)
           << endl;

      // the following is for debugging, to verify these
      // utilities are working correctly:

      // yep, tests look good.

      // testing GTexture_list::get_all:
      cerr << "all GTextures: " << endl;
      GTexture_list all = _focus->gtextures().get_all();
      print_names<GTexture_list>(cerr, all);

      // testing get_sublist:
      //
      // (commented out so we don't have to reference
      //  ColorIDTexture in gtex library)
/*
      cerr << "Color ID GTextures: " << endl;
      print_names<GTexture_list>
         (cerr, get_sublist<GTexture_list, ColorIDTexture>(all));
*/
   }
}

void         
Patch::set_tex_coord_gen(TexCoordGen* tg) 
{
   _tex_coord_gen = tg; 
   triangulation_changed();
}


void 
Patch::triangulation_changed() 
{
   _tri_strips_dirty = 1;
   _d2d_samples_valid = false;

   creases_changed();
   borders_changed();   
   changed(); 
}

void 
Patch::creases_changed() 
{
   delete _creases;
   _creases = 0; 
}

void 
Patch::borders_changed() 
{
   delete _borders;
   _borders = 0; 
}

//********************** ACCESSING PATCH GEOMETRY **********************
EdgeStrip&
Patch::build_sils()
{
   // May be a no-op (after the first time in the current frame):
   _mesh->build_sil_strips();

   return _sils;
}

ZcrossPath&
Patch::build_zx_sils()
{
   // May be a no-op (after the first time in the current frame):
   _mesh->build_zcross_strips();

   return _zx_sils;
}

/*!
 *  Utility method used in build_creases() and build_borders().
 *  
 *  Fill the given edge strip with edges of a given type that
 *  belong to this patch.
 *
 */
EdgeStrip*
Patch::build_edge_strip(EdgeStrip* strip, CSimplexFilter& filter)
{

   if (strip) {
      strip->reset();
      strip->build_with_tips(edges(), filter + PatchEdgeFilter(this));
   }
   return strip;
}

EdgeStrip*
Patch::build_creases()
{
   // If _creases is non-null, it is assumed to be up-to-date.
   if (_creases)
      return _creases;

   // Attempt to generate an EdgeStrip of the correct type
   // (maybe LedgeStrip), depending on the mesh type (maybe LMESH):
   _creases = _mesh ? _mesh->new_edge_strip() : new EdgeStrip();

   // Fill it in and return it:
   return build_edge_strip(_creases, CreaseEdgeFilter());
}

EdgeStrip*
Patch::build_borders()
{
   // If _borders is non-null, it is assumed to be up-to-date.
   if (_borders)
      return _borders;

   // Attempt to generate an EdgeStrip of the correct type
   // (maybe LedgeStrip), depending on the mesh type (maybe LMESH):
   _borders = _mesh ? _mesh->new_edge_strip() : new EdgeStrip();

   // Fill it in and return it:
   return build_edge_strip(_borders, BorderEdgeFilter());
}

Bvert_list
Patch::verts() const
{
   // Return vertices of faces of this patch

   assert(_mesh);

   // Short-cut if patch is whole mesh
   if (_mesh->nfaces() == _faces.num() &&
       !(_mesh->is_polylines() || _mesh->is_points()))
      return _mesh->verts();

   return _faces.get_verts();
}

Bedge_list
Patch::edges() const
{
   // Return edges of faces of this patch

   assert(_mesh);

   // Short-cut if patch is whole mesh
   if (_mesh->nfaces() == _faces.num() && !_mesh->is_polylines())
      return _mesh->edges();

   return _faces.get_edges();
}

//********************** BUILDING **********************
void            
Patch::add(Bface* f) 
{
   // Check for no-op:
   if (!f || f->patch() == this)
      return;

   // Take it out of its current patch:
   if (f->patch())
      f->patch()->remove(f);

   // Add it to the face list etc.:
   f->set_patch_index(_faces.num());
   _faces += f;
   f->set_patch(this);

   triangulation_changed();
}

void            
Patch::remove(Bface* f) 
{
   if (!(f && f->patch() == this)) {
      err_msg("Patch::remove: error: face is NULL or not owned by this Patch");
      return;
   }

   int k = f->patch_index();
   if (_faces.valid_index(k)) {
      _faces.remove(k);
      _faces[k]->set_patch_index(k);
   }
   f->set_patch_index(-1);
   f->set_patch(0);
   triangulation_changed();
}

void            
Patch::add(VertStrip* vs) 
{
   if (vs->patch())
      vs->patch()->remove(vs);
   vs->set_patch(this);
   vs->set_patch_index(_vert_strips.num());
   _vert_strips += vs;
   changed();
}

void            
Patch::remove(VertStrip* vs) 
{
   assert(vs->patch() == this);
   int k = vs->patch_index();
   if (_vert_strips.valid_index(k)) {
      _vert_strips.remove(k);
      _vert_strips[k]->set_patch_index(k);
      changed();
   }
   vs->set_patch_index(-1);
   vs->set_patch(0);
}

void            
Patch::add(EdgeStrip* es) 
{
   if (es->patch())
      es->patch()->remove(es);
   es->set_patch(this);
   es->set_patch_index(_edge_strips.num());
   _edge_strips += es;
   changed();
}

void            
Patch::remove(EdgeStrip* es) 
{
   assert(es->patch() == this);
   int k = es->patch_index();
   if (_edge_strips.valid_index(k)) {
      _edge_strips.remove(k);
      _edge_strips[k]->set_patch_index(k);
      changed();
   }
   es->set_patch_index(-1);
   es->set_patch(0);
}

void
Patch::build_tri_strips()
{
   if (_tri_strips_dirty) {
      _tri_strips_dirty = 0;

      // get rid of old strips:
      int k;
      for (k=0; k<_tri_strips.num(); k++)
         delete _tri_strips[k];
      _tri_strips.clear();

      // clear face flags before finding triangle strips
      for (k=0; k<_faces.num(); k++) {
         _faces[k]->clear_flag();
         _faces[k]->orient_strip(0);
      }

      // If secondary faces shouldn't be drawn, set their flags
      // so they won't be drawn:
      if (!BMESH::show_secondary_faces())
         _faces.secondary_faces().set_flags(1);

      for (k=0; k<_faces.num(); k++)
         if (!_faces[k]->flag())
            TriStrip::get_strips(_faces[k], _tri_strips);

      static bool debug = Config::get_var_bool("PRINT_TRIS_PER_STRIP",false);
      if (debug)
         err_msg("tris/strip: %1.1f", tris_per_strip());
   }
}

//********************** DRAWING **********************
int
Patch::draw(CVIEWptr& v)
{
   GTexture* cur = cur_tex(v);
   if (!cur)
      return 0;

   // if we just switched GTextures then setup a fade from
   // the old one to the new one:
   static const double FADE_DUR = Config::get_var_dbl("FADE_DUR", 0.5,true);
   if (_prev_tex && _prev_tex != cur && _init_fade)
      (*_init_fade)(cur, _prev_tex, v->frame_time(), FADE_DUR);
   _prev_tex = cur;
   int ret = cur->draw(v);
   _pixels.clear();
   return ret;
}

int
Patch::draw_tri_strips(StripCB* cb)
{
   build_tri_strips();

   for (int k=0; k<_tri_strips.num(); k++)
      _tri_strips[k]->draw(cb);

   return _faces.num();
}

int
Patch::draw_n_ring_triangles(int n, StripCB* cb, bool exclude_interior)
{
   // draw the n-ring faces (no triangle strips)

   assert(cb);
   assert(n >= 0);
   Bface_list n_ring = cur_faces().n_ring_faces(n);
   if (exclude_interior) {
      n_ring = n_ring.minus(cur_faces());
   }
   cb->begin_triangles();
   for(int i=0; i<n_ring.num(); ++i) {                
      cb->faceCB(n_ring[i]->v1(), n_ring[i]);
      cb->faceCB(n_ring[i]->v2(), n_ring[i]);
      cb->faceCB(n_ring[i]->v3(), n_ring[i]);         
   }
   cb->end_triangles();      
   return n_ring.num();
}

int
Patch::draw_sil_strips(StripCB *cb)
{
   cur_sils().draw(cb);
   return cur_sils().num();
}

int
Patch::draw_crease_strips(StripCB *cb)
{
   EdgeStrip* strip = cur_creases();
   if (strip)
      strip->draw(cb);
   return 0;
}

int
Patch::draw_edge_strips(StripCB *cb)
{
   // Obsolete

   int ret=0;

   // draw edges as line strips
   for (int i=0; i<_edge_strips.num(); i++) {
      _edge_strips[i]->draw(cb);
      ret += _edge_strips[i]->num();
   }

   return ret;
}

int
Patch::draw_vert_strips(StripCB *cb)
{
   int ret=0;

   // draw point strips (vertices as dots):
   for (int i=0; i<_vert_strips.num(); i++) {
      _vert_strips[i]->draw(cb);
      ret += _vert_strips[i]->num();
   }

   return ret;
}

//********************** PROCEDURAL TEXTURES **********************
void
Patch::set_texture(GTexture* gtex)
{
   if (!gtex) {
      err_msg("Patch::set_texture: can't set nil texture");
      return;
   }
   _textures.add_uniquely(gtex);
   _cur_tex_i = _textures.get_index(gtex);
//    _mesh->changed(BMESH::RENDERING_CHANGED);
}

void
Patch::set_texture(Cstr_ptr& style)
{
   _cur_tex_i = get_tex_index(style);
//    _mesh->changed(BMESH::RENDERING_CHANGED);
}

void
Patch::next_texture()
{
   _cur_tex_i = (_cur_tex_i + 1) % _textures.num();
//    _mesh->changed(BMESH::RENDERING_CHANGED);
}

/*!
 *  Find an existing texture of the given type,
 *  but don't create one if it can't be found.
 *
 */
GTexture* 
Patch::find_tex(Cstr_ptr& tex_name) const
{

   //Hack so that both textures point to the same class
   str_ptr temp_tex_name;
   if(tex_name == "FFSTexture2")
       temp_tex_name = "FFSTexture";
   else
       temp_tex_name = tex_name;
       
   for (int i=0; i<_textures.num(); i++)
      if (_textures[i]->type() == temp_tex_name)
         return _textures[i];

   return 0;
}

/*!
 *  Returns a texture by class name.
 *  Finds one or gets one.
 *
 */
GTexture*
Patch::get_tex(Cstr_ptr& tex_name)
{

   // first try to find one
   GTexture* tex = find_tex(tex_name);
   if (tex)
      return tex;

   // if not found, instantiate it.
   tex = (GTexture*)DATA_ITEM::lookup(tex_name);
   if (!tex)
      return 0;        // can't look it up

   // got one of the right type, now duplicate it
   if ((tex = (GTexture*)tex->dup())) {
      tex->set_patch(this);
      _textures += tex;
      return tex;
   } else {
      err_msg("Patch::get_tex: tex->dup() returned nil");
      return 0;        // can't duplicate it
   }
}

/*!
 *  The texture name to use comes from the mesh, unless it's
 *  the null string, then it comes from the view. If the
 *  texture name is GTexture::static_name() then return the
 *  Patch's "current texture." Otherwise return a texture
 *  matching the given texture name.
 *
 */
GTexture*
Patch::cur_tex(CVIEWptr& v)
{

   // What name are we going for?
   str_ptr tex_name =
      (_mesh->render_style() == str_ptr::null_str()) ?
      v->rendering() :
      _mesh->render_style();

   if (tex_name == GTexture::static_name()) {
      GTexture* tex = cur_tex();
      if (tex)
         return tex;

      // If there is no current texture, go w/ flat shading as a
      // default:
      tex_name = RFLAT_SHADE;
   }

   return get_tex(tex_name);
}

int 
Patch::draw_vis_ref() 
{
   return VisRefDrawer( ).draw(cur_tex(VIEW::peek())); 
}

int 
Patch::draw_id_ref()
{
   return IDRefDrawer( ).draw(cur_tex(VIEW::peek())); 
}

int 
Patch::draw_id_ref_pre1()
{ 
   return IDPre1Drawer( ).draw(cur_tex(VIEW::peek())); 
}

int 
Patch::draw_id_ref_pre2()
{
   return IDPre2Drawer( ).draw(cur_tex(VIEW::peek())); 
}

int 
Patch::draw_id_ref_pre3()
{
   return IDPre3Drawer( ).draw(cur_tex(VIEW::peek())); 
}

int 
Patch::draw_id_ref_pre4()
{
   return IDPre4Drawer( ).draw(cur_tex(VIEW::peek())); 
}

int 
Patch::draw_color_ref(int i)
{
   return ColorRefDrawer(i).draw(cur_tex(VIEW::peek())); 
}

int 
Patch::draw_halo_ref()
{
   return HaloRefDrawer().draw(cur_tex(VIEW::peek()));
}

int 
Patch::draw_final(CVIEWptr &v) 
{
   return FinalDrawer(v).draw(cur_tex(VIEW::peek())); 
}

//********************** I/O FUNCTIONS **********************
int
Patch::write_stream(ostream &os)
{
   // Don't write an empty patch, because the convention
   // below is that a patch with ALL the faces signals it by
   // claiming to have "0" faces.
   //
   // XXX - must be a better way.

   if (_faces.empty())
      return 0;

   assert(_mesh != NULL);

   os << "#BEGIN PATCH" << endl;

   //******** FACES ********

   // Don't output any faces if this patch has all the faces
   if (_faces.num() > _mesh->nfaces()) {

      // Never happens
      assert(0);

   } else if (_faces.num() == _mesh->nfaces()) {

      // We have them all. "0" faces here is a secret code
      // meaning "all" faces.
      os << (int)0 << endl;

   } else {

      // If this is a subdivision mesh, we're writing out
      // the faces at the current level of subdivision:
      CBface_list& cfaces = cur_faces();

      // Number of faces in this patch
      os << cfaces.num() << endl;

      for (int f=0; f<cfaces.num(); f++) {
         assert(get_ctrl_patch(cfaces[f]) == this);
         os << cfaces[f]->index() << endl;
      }
   }
   
   //******** GTEXTURES ********
   os << endl<< _cur_tex_i << endl;
   _textures.write_stream(os);

   //******** NAME ********
   if (_name != NULL_STR) {
      os << "#BEGIN PATCHNAME" << endl;
      os << _name << endl;
      os << "#END PATCHNAME" << endl;
   }
   
   //******** COLOR ********
   if (_has_color) {
      os << "#BEGIN COLOR" << endl;
      os << _color << endl;
      os << "#END COLOR" << endl;
   }
   
   //******** TEXTURE MAP ********
   if (has_texture()) {
      os << "#BEGIN TEXTURE_MAP" << endl;
      os << _texture->file() << endl;
      os << "#END TEXTURE_MAP" << endl;
   }

   os << "#END PATCH" << endl;

   return 1;
}

int
Patch::read_stream(istream &is, str_list &leftover)
{
   leftover.clear();
   // Number of faces in this patch
   int num_faces;
   is >> num_faces;

   bool debug = Config::get_var_bool("DEBUG_PATCH_READ_STREAM",false);
   if (debug)
      err_msg("\n********\nPatch::read_stream: %d faces to read", num_faces);

   if (num_faces > 0) {

      if (debug)
         err_msg("Patch::read_stream: reading %d faces from file", num_faces);

      // for each face...
      int face;
      for (int f = 0; f < num_faces; f++) {
         is >> face;
         if (face < _mesh->nfaces()) {
            add(_mesh->bf(face));
         } else {
            cerr << "Patch::read_stream - face " << face << " > "
               << _mesh->nfaces()
               << ", at byte " << is.tellg() << endl;
         }
      }
   } else {
      if (debug)
         err_msg("Patch::read_stream: using all %d faces of mesh",
                 _mesh->nfaces());

      for (int f = 0; f < _mesh->nfaces(); f++)
         add(_mesh->bf(f));
   }

   is >> _cur_tex_i;
   if (debug)
      err_msg("Patch::read_stream: cur texture is number %d", _cur_tex_i);
   
   // Save old textures
   GTexture_list tmp_textures = _textures;
   // Clear texture list
   _textures.clear();
   

   // continue reading the file but now check for tokens
   static IOBlockList blocklist;
   if (blocklist.num() == 0) {
      blocklist += new IOBlockMeth<Patch>("GTEXTURE", &Patch::read_texture,
                                          this);
      blocklist += new IOBlockMeth<Patch>("COLOR",    &Patch::read_color,
                                          this);
      blocklist += new IOBlockMeth<Patch>("TEXTURE_MAP",
                                          &Patch::read_texture_map,
                                          this);
      blocklist += new IOBlockMeth<Patch>("PATCHNAME",&Patch::read_patchname,
                                          this);
   } else {
      for (int i = 0; i < blocklist.num(); i++) {
         ((IOBlockMeth<Patch> *) blocklist[i])->set_obj(this);
      }
   }
   int ret = IOBlock::consume(is, blocklist, leftover);

   changed();
   
   if (_textures.num() > 0) {

      if (debug) {
         err_msg("Patch::read_stream: read %d textures", _textures.num());
         err_msg(" ... deleting %d old textures", tmp_textures.num());
      }

      // Textures were loaded in, so get rid of previous textures
      tmp_textures.delete_all();

   } else {

      if (debug)
         err_msg("Patch::read_stream: read 0 textures, keeping old ones");

      // No textures were loaded in, go back to pre-existing texture list
      _textures = tmp_textures;
   }

   return ret;
}

int
Patch::read_texture(istream &is, str_list &leftover)
{
   leftover.clear();
   const int namelen = 256;
   char name1[namelen];
   char name[namelen];
   char *wherecr;
   is.getline(name1, namelen); // Finish "#BEGIN GTEXTURE" line 
   is.getline(name, namelen); // Get name of texture
   
   // Strip out CR
   // XXX - also in ViewStroke::read_stroke()
   while ((wherecr = strchr(name, '\015'))) {
      *wherecr = 0;
   }
   int texnum = get_tex_index(name);
   if (texnum < 0) {
      // Clear spaces at end
      // XXX - this should be in str_ptr
      while (strrchr(name, ' ') == name + strlen(name) - 1) {
         name[strlen(name) - 1] = '\0';
      }
      texnum = get_tex_index(name);
   }
   GTexture* tex = texnum >= 0 ? _textures[texnum] : 0;
   if (!tex) {
      cerr << "Patch::read_texture - could not find GTexture '"
           << name << "' (skipping)" << endl;
      return 1;
   }

   return tex->read_stream(is, leftover);
}

int
Patch::read_color(istream &is, str_list &leftover)
{
   leftover.clear();

   COLOR c;
   is >> c;
   set_color(c);

   return 1;
}

int
Patch::read_texture_map(istream &is, str_list &leftover)
{
   leftover.clear();

   char buf[126];
   is >> buf;
   _texture_file = str_ptr(buf);

   return 1;
}

int
Patch::read_patchname(istream &is, str_list &leftover)
{
   leftover.clear();
   const int namelen = 256;
   char name[256];
   is.getline(name, namelen); // Finish "#BEGIN PATCHNAME" line 
   is.getline(name, namelen); // Get name of patch
   set_name(name);

   return 1;
}

//********************** RefImageClient METHODS **********************
void
Patch::request_ref_imgs()
{
   GTexture* tex = cur_tex(VIEW::peek());
   if (tex)
      tex->request_ref_imgs();
}

//********************** DATA_ITEM METHODS **********************
CTAGlist &
Patch::tags() const
{
   if (!_patch_tags) {
      _patch_tags = new TAGlist(0);
      *_patch_tags += new TAG_meth<Patch>(
         "faces", &Patch::put_faces, &Patch::get_faces,  1
         );

      *_patch_tags += new TAG_val<Patch,int>("cur_tex", &Patch::cur_tex_i_);
      *_patch_tags += new TAG_val<Patch,str_ptr>("patchname", &Patch::name_);
      *_patch_tags += new TAG_meth<Patch>(
         "color",  &Patch::put_color, &Patch::get_color, 0
         );

      *_patch_tags += new TAG_meth<Patch>(
         "texture", &Patch::put_textures, &Patch::get_texture, 1
         );

      // XXX - should fix so it can always read in this info,
      //       but only writes it out when non-default values are used:
      if (patch_d2d){
         *_patch_tags +=
            new TAG_val<Patch,int>("sps_height",&Patch::sps_height);   
         *_patch_tags +=
            new TAG_val<Patch,double>("sps_min_dist",&Patch::sps_min_dist); 
         *_patch_tags +=
            new TAG_val<Patch,double>("sps_regularity",&Patch::sps_regularity);
         *_patch_tags +=
            new TAG_val<Patch,bool>("do_dynamic_stuff",&Patch::do_dynamic_stuff);
         *_patch_tags += new TAG_val<Patch,bool>("do_lod",&Patch::do_lod);
         *_patch_tags +=
            new TAG_val<Patch,bool>("do_rotation",&Patch::do_rotation);
         *_patch_tags += new TAG_val<Patch,bool>(
            "use_timed_lod_transitions",&Patch::use_timed_lod_transitions
            );
         *_patch_tags += new TAG_val<Patch,bool>(
            "use_direction_vec",&Patch::use_direction_vec
            );
         *_patch_tags += new TAG_val<Patch,Wpt_list>(
            "direction_stroke",&Patch::direction_stroke
            );
         *_patch_tags += new TAG_val<Patch,bool>(
            "use_weighted_ls",&Patch::use_weighted_ls
            );
         *_patch_tags += new TAG_val<Patch,bool>(
            "use_visibility_test",&Patch::use_visibility_test
            );
      }
   }
   return *_patch_tags;
}

DATA_ITEM*
Patch::dup() const
{
   // to be filled in...

   return 0;
}


//********************** APPEAR METHODS **********************
CCOLOR &
Patch::color() const
{
   return APPEAR::color();
}

bool
Patch::has_color() const
{
   return APPEAR::has_color();
}

void
Patch::set_color(CCOLOR  &c)
{
   // should be in APPEAR
   if (has_color() && color().is_equal(c))
      return;

   APPEAR::set_color(c);
}

void
Patch::unset_color()
{
   APPEAR::unset_color();
}

double
Patch::transp() const
{
   return APPEAR::transp();
}

bool
Patch::has_transp() const
{
   return APPEAR::has_transp();
}

void
Patch::set_transp(double t) 
{ 
   APPEAR::set_transp(t); 
}

void
Patch::unset_transp()       
{ 
   APPEAR::unset_transp(); 
}

void
Patch::set_texture(CTEXTUREptr& t)
{
   APPEAR::set_texture(t);
   changed();
}

void
Patch::unset_texture()
{
   APPEAR::unset_texture();
   changed();
}

void
Patch::get_texture(TAGformat &d)
{
   str_ptr str, str1;
   *d >> str;

   if ((*d).ascii())
   {
      while (str1 = (*d).get_string_with_spaces())
      {
         str = str + " " + str1;
      }
   }

   int texnum = get_tex_index(str);

   if (texnum < 0) 
   {
      err_msg("Patch::get_texture() - ERROR! Could not find texture '%s'", **str);
      return;
   }

   GTexture* tex = _textures[texnum];   assert(tex);

   err_mesg(ERR_LEV_SPAM, "Patch::get_texture() - Loaded: '%s'", **str);
   
   tex->decode(*d);

}

void
Patch::put_textures(TAGformat &d) const
{
   for (int t = 0; t < _textures.num(); t++) 
   {
      err_mesg(ERR_LEV_SPAM, "Patch::put_textures() - Writing texture #%d '%s'", 
         t, **_textures[t]->class_name());
      d.id();
      _textures[t]->format(*d);
      d.end_id();
   }
}

void
Patch::recompute()
{
   if (_faces.empty()) {
      // This patch has no faces, so we take all the faces...
      for (int k = 0; k<_mesh->nfaces(); k++) {
         add(_mesh->bf(k));
      }
   }
}

void
Patch::get_faces(TAGformat &d)
{
   ARRAY<int> faces;
   *d >> faces;

   // for each face...
   for (int f = 0; f < faces.num(); f++) 
   {
      if (faces[f] < _mesh->nfaces()) 
      {
         add(_mesh->bf(faces[f]));
      } 
      else 
      {
         err_msg("Patch::get_faces() - ERROR! face #%d > %d.", faces[f], _mesh->nfaces());
      }
   }
}

void
Patch::put_faces(TAGformat &d) const
{
   // XXX - no faces entry means all faces
   if (_faces.num() == ((Patch *) this)->mesh()->nfaces()) return;

   ARRAY<int> faces(_faces.num());

   for (int f=0; f<_faces.num(); f++) 
   {
      const Bface *outf = _faces[f];
      if (outf->patch() != this) 
      {
         err_msg("Patch::put_faces() - ERROR! face #%d has ownership issues...", outf->index());
      }
      faces += outf->index();
   }
   d.id();
   *d << faces;
   d.end_id();
}

// Dynamic Samples Stuff

inline VEXEL
cmult(CVEXEL& a, CVEXEL& b)
{
   // complex number multiplication:
   return VEXEL(a[0]*b[0] - a[1]*b[1], a[0]*b[1] + a[1]*b[0]);
}

inline double
compute_weight(
   CWpt& p,                     // world space sample point
   CWvec& n,                    // world space sample normal (unit length)
   CWpt& eye,                   // world space camera location
   CWvec& v,                    // world space "right" camera vector
   double r,                    // approximate sample spacing, world space
   bool do_weighted,            // if false, just use weights = 0 or 1
   const VisibilityTest& vis    // reports visibility of a point
   )
{
   // XXX - r should be expressed in world space, which means
   //       we're assuming the transform (if any) does uniform scaling

   // XXX - should allow points slightly outside the window to be used:
   if (!p.in_frustum())
      return 0;
   double nv = clamp(n*(eye - p).normalized(), 0.0, 1.0);
   double nvv = vis.visibility(p,r/4) * nv; // weighted by visibility
   if (nvv < mlib::epsAbsMath())
      return 0;
   if (do_weighted) {
      // convert r to a radius, not diameter
      r /= 2;
      // on-screen diameter of sample, treated as a sphere:
      double d = PIXEL(p + v*r).dist(PIXEL(p - v*r));
      // compute value proportional to the area of a disk, attenuated
      // by the cosine term to account for foreshortening, and multiplied
      // by visibility value (0 for totally occluded):
      return sqr(d) * nvv;
   } else {
      return (nvv > 0) ? 1.0 : 0.0;
   }
}

inline bool
extract_weights(
   const vector<DynamicSample>& w1,
   const vector<DynamicSample>& w2,
   vector<double>& weights
   )
{
   // get list of weights to work with for this frame.
   // as long as both samples have weights > 0, use
   // the weight from the first list; otherwise use 0:

   // return true if any weights are > 0

   assert(w1.size() == w2.size());
   weights.clear();
   bool ret = false;
   for (uint i=0; i<w1.size(); ++i) {
      double w = (w1[i].get_weight() > 0 && w2[i].get_weight() > 0) ?
         w1[i].get_weight() : 0.0;
      weights.push_back(w);
      ret = ret || (w > 0);
   }
   return ret;
}

inline PIXEL
weighted_sum(const vector<DynamicSample>& s,    // samples
             const vector<double>& w)           // weights
{
   // weighted sum of pixel locations

   assert(w.size() == s.size());
   PIXEL ret;
   double total = 0;
   for (uint i=0; i<w.size(); ++i) {
      total += w[i];
      ret   += s[i].get_pix()*w[i];
   }
   return (total > mlib::epsAbsMath()) ? (ret / total) : PIXEL();
}

void 
Patch::create_dynamic_samples(const VisibilityTest& vis)
{
   _old_samples.clear();

   Bface_list sample_faces;
   ARRAY<Wvec> sample_bc;
   generate_samples(
      this,
      sample_faces,
      sample_bc,
      _sample_spacing,
      _sps_height,
      _sps_min_dist,
      _sps_regularity
      );
   assert(sample_faces.num() == sample_bc.num());
   if (0 && debug_samples) {
      cerr << "Patch::create_dynamic_samples: created "
           << sample_faces.num() << " samples, spacing: "
           << _sample_spacing
           << endl; 
   }
   // record the fact that we have done this:
   _d2d_samples_valid = true;

   // XXX - temporary, for checking that we can switch:
   if (debug_samples) {
      // only report the policy when it has changed from last frame:
      static bool recent_ls_policy = false;
      if (recent_ls_policy != _use_weighted_ls) {
         recent_ls_policy = _use_weighted_ls;
         cerr << "using " << (_use_weighted_ls ? "soft" : "hard")
              << " weights" << endl;
      }
      static bool recent_vis_policy = false;
      if (recent_vis_policy != _use_visibility_test) {
         recent_vis_policy = _use_visibility_test;
         cerr << "using " << (_use_visibility_test ? "" : "no")
              << " visibility" << endl;
      }
   }

   // object to world transforms
   CWtransf& xf = xform();                // for points
   Wtransf xfn = inv_xform().transpose(); // for normals
   Wpt eye = VIEW::peek()->eye();
   Wvec right_v = VIEW::peek()->cam()->data()->right_v();
   // get approximate sample spacing in world space:
   // XXX - assumes uniform scaling:
   double r = _sample_spacing * xf.X().length(); 

   // create the sample list:
   for(int i=0; i < sample_faces.num(); ++i) {
      // sample location in world space:
      Wpt p = xf*sample_faces[i]->bc2pos(sample_bc[i]);
      // surface normal at sample, in world space:
      Wvec n = (xfn*sample_faces[i]->norm()).normalized();
      // compute the weight:
      double weight = compute_weight(
         p, n, eye, right_v, r, _use_weighted_ls, vis
         );
      // store sample:
      _old_samples.push_back(
         DynamicSample(sample_faces[i], sample_bc[i], PIXEL(p), weight)
         );
   }
}

void
Patch::update_dynamic_samples(const VisibilityTest& vis)
{
   if(!_do_dynamic_stuff)
      return;
   
   if (_dynamic_stamp == VIEW::stamp())
      return;
   _dynamic_stamp = VIEW::stamp();
  
   if (!_d2d_samples_valid || _old_samples.empty()){
      create_dynamic_samples(vis);  
   }

   // object to world transforms
   CWtransf& xf = xform();                // for points
   Wtransf xfn = inv_xform().transpose(); // for normals
   Wpt eye = VIEW::peek()->eye();         // camera location, world space
   Wvec right_v = VIEW::peek()->cam()->data()->right_v();
   // get approximate sample spacing in world space:
   // XXX - assumes uniform scaling:
   double r = _sample_spacing * xf.X().length(); 

   // create the new samples:
   vector<DynamicSample>  new_samples;
   for (uint i=0; i< _old_samples.size(); ++i) {
      // sample location in world space:
      Wpt p = xf*_old_samples[i].get_pos();
      // surface normal at sample, in world space:
      Wvec n = (xfn*_old_samples[i].get_norm()).normalized();
      // compute the weight:
      double weight = compute_weight(
         p, n, eye, right_v, r, _use_weighted_ls, vis
         );
      // store the sample info:
      new_samples.push_back(DynamicSample(_old_samples[i], PIXEL(p), weight));
   }
   assert(new_samples.size() == _old_samples.size()); // obviously true

   // Compute the transformation from old samples to new samples.
   // It is a combination of translation, rotation, and uniform scale.
   // The latter two are represented by multiplication by complex number z.

   vector<double> weights;
   if (!extract_weights(new_samples, _old_samples, weights)){
      // XXX - not sure the following is correct...

      // Few or no sample points to work with (weights are all 0).
      // Should just start over in this case...
      //cerr << "Patch::calculate_xform: too few samples: " << endl;
      _old_samples = new_samples;  
      // recursive call but it's ok, it will use current new samples
      // as old ones
      update_dynamic_samples();
      return;
   }
   assert(weights.size() == new_samples.size());

   // compute weighted average of old and new samples:
   // use the same weights for both:
   _old_center = weighted_sum(_old_samples, weights);
   _new_center = weighted_sum( new_samples, weights);
   
   // compute z via least-squares method:
   _z = VEXEL(0,0);
   double so = 0;     // sum of squares of "old" vectors
   double sn = 0;     // sum of squares of "new" vectors
   for (unsigned int k=0; k<new_samples.size(); ++k) {
      if (weights[k] > 0) {
         VEXEL  o = _old_samples[k].get_pix() - _old_center;
         VEXEL  n =  new_samples[k].get_pix() - _new_center;
         double w = weights[k];
         _z  += VEXEL(o*n, det(o,n))*w;
         so += o.length_sqrd()*w;
         sn += n.length_sqrd()*w;
      }
   }
   if (isZero(so) || isZero(sn)) {
      // impossible? old/new samples are each concentrated at a point
      _z = VEXEL(1,0);
      _old_samples = new_samples;  
      update_dynamic_samples();
      return;
   }

   bool do_symmetric = true;
   if (!do_symmetric) {
      // old policy: scaling is not symmetric if you swap "old" and "new":
      _z /= so;
   } else {
      // new policy: does symmetric scaling, prevents shrinkage:
      _z = _z.normalized() * sqrt(sn/so);
   }

   _z = (_do_rotation) ? _z : VEXEL(_z[0], 0);

   // store the new locations/weights for next time:
   _old_samples.clear();
   _old_samples = new_samples;

   // LOD Stuff
   if(_do_lod) {
      _scale *= _z.length();
      _target_scale *= _z.length();
   }
   
   if(_use_direction_vec){
      VEXEL direaction_vec = get_direction_vec().normalized();
      _o = _new_center + cmult(_o - _old_center, _z);
      _u_o = _o + (direaction_vec);
      _v_o = _o + (direaction_vec.perpend());
   } else {
      _o = _new_center + cmult(_o - _old_center, _z);
      _u_o = _new_center + cmult(_u_o - _old_center, _z);
      _v_o = _new_center + cmult(_v_o - _old_center, _z);
   }

   // LOD computations:
   double s = _scale;
   assert(s > 0);
   // ensure s is in [1,2)
   while (s >= 2) s /= 2;
   while (s < 1)  s *= 2;

   // Periodically transitions all patches to scale 1
   static bool _use_periodic_lod_sync = true;
   if (_use_periodic_lod_sync && _do_lod && !_transition_has_started) {
      static const double sync_interval = 2.0;
      static double last_sync_time = VIEW::peek()->frame_time(); 
//      static bool counted = false;

      // If the amount of time passed since the last sync is >= to the
      // time between syncs, then we start a new transition.
      if (VIEW::peek()->frame_time() - last_sync_time >= sync_interval) {

         // If we haven't already started a transition based on this particlar
         // sync event, then mark that we've seen the periodic sync even, and
         // initiate a transition.
         if (!_saw_periodic_sync) {
            _saw_periodic_sync = true;
            if (_scale > 1.1 || _scale < 0.9)
               this->start_timed_lod_transition();
         }
         else {
            // Since we've already seen this periodic sync event (saw_periodic_sync is true)
            // we should update the last sync time, and rest our boolean.
            last_sync_time = VIEW::peek()->frame_time();
            _saw_periodic_sync = false;
         }
      }
   }

   if (_use_timed_lod_transitions) {
      
      // If the transition has already started, then we need to check to see if it
      // is finished, and if it is not, computer the new lod value.
      if (_transition_has_started) {
         double time_current = VIEW::peek()->frame_time();
         double time_remaining = _transition_duration - (time_current - _transition_start_time);
         if (time_remaining <= 0) {
            // if time_remaining is <= 0 then the transition is complete.
            _transition_has_started = false;
            _lod_t = 0.0;
            _timed_lod_lo = 1.0;
            _scale = _target_scale;
         }
         else {
            // the transition is still happening, so compute the current lod value.
            _lod_t = (_transition_duration - time_remaining) / _transition_duration;
         }
      }
      else {
         // If the scale is within acceptable limits, do nothing, otherwise
         // start a timed lod transition.
         if (_scale > _down_lim && _scale < _up_lim) {
         } else {
            this->start_timed_lod_transition();
         }
      }
      _lod_u = sample_u_vec().normalized()*_scale;
      _lod_v = sample_v_vec().normalized()*_scale;
   }
   else {
      // This is for procedural LOD
      const float st0 = 1.3f; // transition start
      const float st1 = 1.7f; // transition end
      _timed_lod_hi = 2.0;
      _timed_lod_lo = 1.0;
      _lod_t = (s < st0) ? 0.0 : (s > st1) ? 1.0: (s - st0)/(st1 - st0);
      _lod_u = sample_u_vec().normalized()*s;
      _lod_v = sample_v_vec().normalized()*s;
   }
}

void
Patch::start_timed_lod_transition()
{
   _target_scale = 1.0;
   _timed_lod_hi = _scale;
   _transition_start_time = VIEW::peek()->frame_time();
   _transition_has_started = true;
}

VEXEL 
Patch::get_z_dynamic_samples(const vector<DynamicSample>& old_samples)
{
   // XXX - not the real deal... used for creating the video demo?
   //       to animate the samples going from "old" to "new"?

   // compute a 2D translation (delt) and rotation and uniform
   // scale (z) from the 2D motion of sample points on the patch
   VEXEL    z = VEXEL(1,0);
   if (old_samples.empty()){
      cerr << "Patch::get_z_dynamic_samples ERROR, no old_samples" << endl;
      return VEXEL(1,0);
   }

   // object to world transforms
   CWtransf& xf = xform();                // for points
   Wtransf xfn = inv_xform().transpose(); // for normals
   Wpt eye = VIEW::peek()->eye();         // camera location, world space
   Wvec right_v = VIEW::peek()->cam()->data()->right_v();
   // get approximate sample spacing in world space:
   // XXX - assumes uniform scaling:
   double r = _sample_spacing * xf.X().length(); 

   // create the new samples:
   vector<DynamicSample>  new_samples; 
   for (uint i=0; i< old_samples.size(); ++i) {
      // sample location in world space:
      Wpt p = xf*old_samples[i].get_pos();
      // surface normal at sample, in world space:
      Wvec n = (xfn*old_samples[i].get_norm()).normalized();
      // compute the weight:
      // XXX not doing visibility here
      double weight = compute_weight(
         p, n, eye, right_v, r, _use_weighted_ls, VisibilityTest()
         );
      // store the sample info:
      new_samples.push_back(DynamicSample(old_samples[i], PIXEL(p), weight));
   }
   assert(new_samples.size() == old_samples.size()); // obviously true

   // Compute the transformation from old samples to new samples.
   // It is a combination of translation, rotation, and uniform scale.
   // The latter two are represented by multiplication by complex number z.

   vector<double> weights;
   if (!extract_weights(new_samples, old_samples, weights)) {
      // XXX - not sure the following is correct...

      // Few or no sample points to work with (weights are all 0).
      cerr << "Patch::get_z_dynamic_samples ERROR, "
           << "no sample points to work with" << endl;
      return VEXEL(1,0);
   }
   assert(weights.size() == new_samples.size());

   // compute weighted average of old and new samples:
   // use the same weights for both:
   PIXEL old_center = weighted_sum(old_samples, weights);
   PIXEL new_center = weighted_sum(new_samples, weights);
   
   // compute z via least-squares method:
   z = VEXEL(0,0);
   double so = 0;     // sum of squares of "old" vectors
   double sn = 0;     // sum of squares of "new" vectors
   for (unsigned int k=0; k< new_samples.size(); ++k) {
      if (weights[k] > 0) {
         VEXEL  o = old_samples[k].get_pix() - old_center;
         VEXEL  n = new_samples[k].get_pix() - new_center;
         double w = weights[k];
         z  += VEXEL(o*n, det(o,n))*w;
         so += o.length_sqrd()*w;
         sn += n.length_sqrd()*w;
      }
   }
   if (isZero(so) || isZero(sn)) {
      // impossible? old/new samples are each concentrated at a point
      z = VEXEL(1,0);
 
      //create_dynamic_samples(); <- bad idea 
      return z;
   } 

   bool do_symmetric = true;
   if (!do_symmetric) {
      // old policy: scaling is not symmetric if you swap "old" and "new":
      z /= so;
   } else {
      // new policy: does symmetric scaling, prevents shrinkage:
      z = z.normalized() * sqrt(sn/so);
   }

   z = (_do_rotation) ? z : VEXEL(z[0], 0);

   return z;
}

void 
Patch::reset_dynamic_samples()
{
}

void
GTexture_list::add_slave_textures(
   GTexture_list& ret,
   const GTexture_list& list
   )
{
   // output everything in the list, plus all their slave GTextures
   // (continuing recursively):
   for (int i=0; i<list.num(); i++) {
      ret.add(list[i]);
      add_slave_textures(ret, list[i]->gtextures());
   }
}

VEXEL    
Patch::get_direction_vec() const
{
   if(_direction_stroke.empty())
      return VEXEL(0.0,0.0);

   Wvec w_vec = _direction_stroke.last() - _direction_stroke.pt(0);
   return VEXEL(_direction_stroke.average(), w_vec);

}

// end of file patch.C
