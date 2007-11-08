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
 * bsurface.H
 *****************************************************************/
#ifndef BSURFACE_H_IS_INCLUDED
#define BSURFACE_H_IS_INCLUDED

#include "mesh/lpatch.H"
#include "mesh/uv_data.H"

#include "bpoint.H"
#include "bcurve.H"

/*****************************************************************
 * Bsurface:
 * 
 *      A procedural entity that operates on a mesh to approximate
 *      some shape.
 *
 *      under construction ...
 *****************************************************************/
class Bsurface;
typedef const Bsurface CBsurface;
class Bsurface : public Bbase {
 public:
   //******** MANAGERS ********
   Bsurface(CLMESHptr& mesh=0);

   virtual ~Bsurface();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("Bsurface", Bsurface*, Bbase, CBnode*);

   //******** UTILITY METHODS ********

   static Bsurface_list selected_surfaces();
   static Bsurface*     selected_surface();

   //******** LOOKUP ********

   // Returns the Bsurface that owns the boss meme on the simplex:
   static Bsurface* find_owner(CBsimplex* s) {
      return upcast(Bbase::find_owner(s));
   }

   // Find the Bsurface owner of this simplex, or if there is none,
   // find the Bsurface owner of its subdivision parent (if any),
   // continuing up the subdivision hierarchy until we find a
   // Bsurface owner or hit the top trying.
   static Bsurface* find_controller(CBsimplex* s) {
     return upcast(Bbase::find_controller(s));
   }

   // Returns the set of Bsurfaces owning any of the given faces:
   static Bsurface_list get_surfaces(CBface_list& faces);

   // If a single Bsurface owns all the faces, return it:
   static Bsurface* get_surface(CBface_list& faces);

   //******** ACCESSORS ********
   double      target_len()     const   { return _target_len; }
   Lpatch*     patch()                  { return _patch; }

   CBface_list& bfaces()        const;

   // Additional mesh elements, passed by copying:
   Bedge_list bedges()          const { return bfaces().get_edges(); }
   Bvert_list bverts()          const { return bfaces().get_verts(); }

   // Meme lists
   const EdgeMemeList& ememes() const { return _ememes; }
   const FaceMemeList& fmemes() const { return _fmemes; }

   //******** SUBDIVISION Bsurfaces ********

   // Bsurface at top of hierarchy:
   Bsurface* ctrl_surface()     const   { return (Bsurface*) control(); }

   // The subdiv parent and child surfaces:
   Bsurface* parent_surface()   const   { return (Bsurface*) _parent; }
   Bsurface* child_surface()    const   { return (Bsurface*) _child; }

   //******** ASSOCIATED POINTS AND CURVES ********

   void absorb(Bcurve* c);
   void absorb(Bpoint* p);

   void absorb(CBcurve_list& c) {
      for (int i=0; i<c.num(); i++)
         absorb(c[i]);
   }
   void absorb(CBpoint_list& p) {
      for (int i=0; i<p.num(); i++)
         absorb(p[i]);
   }

   CBcurve_list& curves()       const   { return _bcurves; }
   
   //******** BUILDING ********

   // Copy the elements of the given mesh, adding all the new
   // faces to the Patch belonging to this Bsurface:
   void absorb_mesh_copy(CLMESHptr& m);

   // XXX - obsolete:
   // Punch out a hole in the region containing the given face:
   bool punch(Lface* f);
   
   //******** TESSELLATION ********
   void tessellate(CARRAY<Bvert_list>& contours);

   // XXX - need regular tessellation too

   //******** DRAWING ********
   void draw_creases();

   //******** UPDATING ********

   // XXX - no longer supported:

   // edge operations
   virtual bool do_swaps();
   virtual bool do_splits();
   virtual bool do_collapses();

   // all edge ops:
   virtual bool do_retri();

   //******** DIAGNOSTIC ********
   static void toggle_show_crease()     { _show_crease = !_show_crease; }

   //******** PICKING ********

   // Given screen-space point p and search radius (in pixels),
   // return the Bsurface closest to p within the search radius
   // (if any). When successful, also fills in the Wpt that was
   // hit, and the hit face (if requested).
   //
   // Note: the face may not belong directly to the surface if
   // it's from a subdivision level not controlled directly by a
   // Bsurface.
   //
   static Bsurface* hit_surface(CNDCpt& p, double rad, mlib::Wpt& hit, Bface** f=0);
   static Bsurface* hit_surface(CNDCpt& p, double rad=2, Bface** f=0) {
      Wpt hit;
      return hit_surface(p, rad, hit, f);
   }

   // Same as above, but returns the control-level Bsurface:
   static Bsurface* hit_ctrl_surface(CNDCpt& p, double rad, mlib::Wpt& hit,
                                     Bface** e=0){
      Bsurface* ret = hit_surface(p, rad, hit, e);
      return ret ? ret->ctrl_surface() : 0;
   }
   static Bsurface* hit_ctrl_surface(CNDCpt& p, double rad=2, Bface** f=0) {
      Wpt hit;
      return hit_ctrl_surface(p, rad, hit, f);
   }
   
   //******** Bbase VIRTUAL METHODS  ********

   // Set the parent, ensure patches are in agreement
   virtual void set_parent(Bbase* p);

   // Wipe out mesh elements controlled by this
   virtual void delete_elements();

   virtual void rem_edge_meme(EdgeMeme* e);
   virtual void rem_face_meme(FaceMeme* f);

   virtual void draw_debug();

   virtual void set_selected();

   //******** SHOW/HIDE ********

   // Hide means make the surface disappear;
   // show means make it reappear. Gives the effect of undo/redo:
   virtual bool can_hide() const { return bfaces().can_push_layer(); }
   virtual bool can_show() const { return bfaces().can_unpush_layer(); }

   virtual void hide();
   virtual void show();

   //******** Bnode METHODS ********

   virtual CCOLOR& selection_color() const;
   virtual CCOLOR& regular_color()   const;

   virtual Bnode_list inputs() const;

   //******** RefImageClient METHODS
   // draw to the screen:
   virtual int draw(CVIEWptr&);

   //******** RefImageClient METHODS ********
   virtual void request_ref_imgs();
   virtual int draw_vis_ref();  // draw vis ref image (for picking)

 protected:
   // utility method used in draw methods below:
   Patch* get_p() const { return _is_shown && _patch ? _patch : 0; }
 public:
   virtual int draw_id_ref()        { return    IDRefDrawer( ).draw(get_p());}
   virtual int draw_id_ref_pre1()   { return   IDPre1Drawer( ).draw(get_p());}
   virtual int draw_id_ref_pre2()   { return   IDPre2Drawer( ).draw(get_p());}
   virtual int draw_id_ref_pre3()   { return   IDPre3Drawer( ).draw(get_p());}
   virtual int draw_id_ref_pre4()   { return   IDPre4Drawer( ).draw(get_p());}
   virtual int draw_color_ref(int i){ return ColorRefDrawer(i).draw(get_p());}
   virtual int draw_final(CVIEWptr &v) {
      return FinalDrawer(v).draw(get_p());
   }
	
 //*******************************************************
 // PROTECTED
 //*******************************************************
 protected:

   //******** INTERNAL DATA ********

   Lpatch*      _patch;         // patch that can draw the triangles
   EdgeMemeList _ememes;        // edge memes
   FaceMemeList _fmemes;        // face memes

   // XXX - Subject to redesign:
   // Used as the "controls" (drawn when the surface is selected):
   Bpoint_list  _bpoints;       // points associated w/ this surface
   Bcurve_list  _bcurves;       // curves associated w/ this surface

   // Retriangulation:
   int          _retri_count;   // incremented each retriangulation pass
   double       _target_len;    // may be used for retriangulation
  
   // For debugging:
   uint         _update_stamp;  // VIEW::stamp() of last update

   //******** STATICS ********
   static bool  _show_crease;

   //******** INTERNAL METHODS ********

   // Does Bbase::set_mesh()
   virtual void set_mesh(CLMESHptr& mesh);

   void gen_subdiv_memes() const;

   //******** ADDING ELEMENTS ********

 public:
   // XXX - the following should be protected.
   //       but fixing it would take too long right now.

   // Edges:
   virtual EdgeMeme* add_edge_meme(EdgeMeme* e);

   virtual EdgeMeme* add_edge_meme(Ledge*);
   EdgeMeme* add_edge(Bvert* u, Bvert* v) {
      return add_edge_meme((Ledge*)_mesh->add_edge(u,v));
   }

   // Faces
   virtual FaceMeme* add_face_meme(FaceMeme* f);

   virtual FaceMeme* add_face_meme(Lface*);

   // Create the face and add the face and edge memes
   FaceMeme* add_face(Bvert* u, Bvert* v, Bvert* w);

   // Create a face with the given UV-coords:
   FaceMeme* add_face(Bvert* u, Bvert* v, Bvert* w,
                      CUVpt& a, mlib::CUVpt& b, mlib::CUVpt& c);

   // Add a quad (2 faces), optionally with UV-coords:
   FaceMeme* add_quad(Bvert* u, Bvert* v, Bvert* w, Bvert* x);
   FaceMeme* add_quad(Bvert* u, Bvert* v, Bvert* w, Bvert* x,
                      CUVpt& a, mlib::CUVpt& b, mlib::CUVpt& c, mlib::CUVpt& d);

   // add face memes to these faces (should be ours)
   // also adds memes to the contained edges
   void add_face_memes(CBface_list& faces);
 protected:

   //******** Bnode PROTECTED METHODS ********
   virtual void recompute();
};

/*****************************************************************
 * Bsurface_list
 *****************************************************************/
class Bsurface_list : public _Bbase_list<Bsurface_list,Bsurface> {
 public:
   //******** MANAGERS ********
   Bsurface_list(int n=0) :
      _Bbase_list<Bsurface_list,Bsurface>(n)    {}
   Bsurface_list(const Bsurface_list& list) :
      _Bbase_list<Bsurface_list,Bsurface>(list) {}
   Bsurface_list(Bsurface* s) :
      _Bbase_list<Bsurface_list,Bsurface>(s)    {}

   Bsurface_list(CBbase_list& bbases) :
      _Bbase_list<Bsurface_list,Bsurface>(bbases.num()) {
      for (int i=0; i<bbases.num(); i++) {
         Bsurface* s = Bsurface::upcast(bbases[i]);
         if (s)
            *this += s;
      }
   }
};
typedef const Bsurface_list CBsurface_list;

/*****************************************************************
 * Bsurface inline methods
 *****************************************************************/
inline Bsurface_list
Bsurface::get_surfaces(CBface_list& faces)
{
   Bsurface_list ret;
   Bsurface* bs = 0;
   for (int i=0; i<faces.num(); i++)
      if ((bs = find_owner(faces[i])))
         ret.add_uniquely(bs);
   return ret;
}

// If a single Bsurface owns all the faces, return it:
inline Bsurface* 
Bsurface::get_surface(CBface_list& faces) 
{
   Bsurface_list surfs = get_surfaces(faces);
   if (surfs.num() != 1)
      return 0;
   Bsurface* ret = surfs.first();
   return faces.all_satisfy(BbaseOwnerFilter(ret)) ? ret : 0;
}

#endif // BSURFACE_H_IS_INCLUDED

/* end of file bsurface.H */
