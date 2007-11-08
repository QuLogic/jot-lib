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
 * rsurf.H
 *
 *  Replica surface (Rsurface):
 *     it takes on the shape of an existing set of faces.
 *
 *****************************************************************/
#ifndef RSURF_H_IS_INCLUDED
#define RSURF_H_IS_INCLUDED

#include "bsurface.H"
#include "vert_pair.H"

/*****************************************************************
 * Rmeme:
 *
 *   Vert meme class used by replica surface (Rsurface).
 *   It sticks to a given vertex.
 *****************************************************************/
class RmemeVertPair;
class Rmeme : public VertMeme {
 public:
   //******** MANAGERS ********

   // constructor for Rmeme of the control surface:
   Rmeme(Bbase* b, Lvert* v, uint pair_lookup_key,
         Lvert* ref, double h=0, double s=0, bool scale_h=true);

   // constructor for Rmeme at subdiv level > 0:
   Rmeme(Bbase* b, Lvert* v, uint pair_lookup_key);

   virtual ~Rmeme();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("Rmeme", Rmeme*, VertMeme, CSimplexData*);

   //******** ACCESSORS ********

   // reference vert:
   Lvert*    ref()      const { return _ref; }

   // displacement from the reference vert (along its normal):
   double    h()     const { return _h; }

   // scale factor used at creases and corners:
   double    s()     const { return _s; }

   //******** NOTIFICATION ********

   void ref_vert_changed();

   void ref_vert_deleted();
   
   //******** VertMeme VIRTUAL METHODS ********

   // Rmemes don't need to get any info from their parent
   // (any more)
   virtual void update_from_parent() {}

   virtual void copy_attribs_v(VertMeme*);
   virtual void copy_attribs_e(VertMeme*, VertMeme*);
   virtual void copy_attribs_q(VertMeme*, VertMeme*, VertMeme*, VertMeme*);

   // replicas don't do relaxing:
   virtual bool compute_delt() { return false; }
   virtual bool apply_delt()   { return false; }

   // Compute 3D vertex location 
   virtual CWpt& compute_update();

 protected:
   Lvert*         _ref;      // reference vertex this one sticks to
   double         _h;        // displacement due to inflation/extrude
   double         _s;        // additional scale factor for _h
   uint           _pair_key; // key for finding VertPair

   //******** VertMeme VIRTUAL METHODS ********

   // Methods for generating vert memes in the child Bbase.
   // (See meme.H for more info):
   virtual VertMeme* _gen_child(Lvert*)                                  const;
   virtual VertMeme* _gen_child(Lvert*, VertMeme*)                       const;
   virtual VertMeme* _gen_child(Lvert*, VertMeme*, VertMeme*, VertMeme*) const;

   //******** HELPERS ********

   void set_ref(Lvert* r);

   void gen_pair();
   void delete_pair();
};


/*****************************************************************
 * Rsurface
 *****************************************************************/
class Rsurface_list;
class Rsurface: public Bsurface {
 public:
   //******** MANAGERS ********

   // Create an empty Rsurface
   Rsurface(CLMESHptr& mesh=0);

   // Create a replica of the given face set.
   // 'mesh' is expected to be the mesh of the face set
   // (the faces are expected to come from a single mesh).
   // offset distance 'h' is relative to local edge length.
   // 'level' tells how many levels down to fit the face set
   // (i.e. it is the "res level")
   Rsurface(
      CLMESHptr&        mesh,            // build on this mesh
      CBface_list&      faces,           // side A faces to replicate
      double            h,               // offset distance
      int               level,           // match side A to this subdiv level
      bool              flip_norms=false,// orient surface opposite to side A
      bool              scale_h=true     // Use local or global scale for h
      );

   // Create a child Rsurface of the given parent:
   Rsurface(Rsurface* parent);

   virtual ~Rsurface();
   
   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("Rsurface", Rsurface*, Bsurface, CBnode*);

   //******** ACCESSORS ********

   // "side A" faces:
   CBface_list&  ref_faces() const { return _ref_faces; }

   Rmeme* rmeme(int i) const;

   uint pair_lookup_key() const { return (uint)this; }

   //******** A --> B MAPPING ********

   // Given side A element, return corresponding side B element:
   Lvert* map_to_b(CBvert* a) const {
      return (Lvert*)VertPair::map_vert(pair_lookup_key(), a);
   }
   Ledge* map_to_b(CBedge* a) const {
      if (!a) return 0;
      return (Ledge*)lookup_edge(map_to_b(a->v1()), map_to_b(a->v2()));
   }
   Lface* map_to_b(CBface* a) const {
      if (!a) return 0;
      return (Lface*)lookup_face(
         map_to_b(a->v1()),
         map_to_b(a->v2()),
         map_to_b(a->v3())
         );
   }

   // Map the entire A-side region to corresponding B-side faces,
   // but if any face can't be mapped, return the empty list:
   bool map_to_b(CBface_list& A, Bface_list& B) const;
   Bface_list map_to_b(CBface_list& A) const {
      Bface_list B;
      map_to_b(A, B);
      return B;
   }

   // Map an A-side edge strip to the corresponding strip on side B,
   // but if any element can't be mapped, return the empty strip:
   bool map_to_b(CEdgeStrip& A, EdgeStrip& B) const;
   EdgeStrip map_to_b(CEdgeStrip& A) const {
      EdgeStrip B;
      map_to_b(A, B);
      return B;
   }

   // Given a region of mesh, find an Rsurface of the same
   // mesh whose side A faces includes the region. Return
   // both the Rsurface and the corresponding B-side faces.
   static Rsurface* find_b(CBface_list& region, Bface_list& B);

   //******** B --> A MAPPING ********

   // Mapping from side B elements to corresponding side A elements:
   static Bvert* side_a_vert(CBvert* v) {
      Rmeme* rm = Rmeme::upcast(find_boss_meme(v));
      return rm ? rm->ref() : 0;
   }
   static Bedge* side_a_edge(CBedge* e) {
      return e ? lookup_edge(side_a_vert(e->v1()), side_a_vert(e->v2())) : 0;
   }
   static Bface* side_a_face(CBface* f) {
      return f ? lookup_face(side_a_vert(f->v1()),
                             side_a_vert(f->v2()),
                             side_a_vert(f->v3())) : 0;
   }

   // Map the entire B-side region to corresponding A-side faces,
   // but if any face can't be mapped, return the empty list:
   static bool map_to_a(CBface_list& B, Bface_list& A);
   static Bface_list map_to_a(CBface_list& B) {
      Bface_list A;
      map_to_a(B, A);
      return A;
   }
   static bool map_to_a(CBedge_list& B, Bedge_list& A);
   static Bedge_list map_to_a(CBedge_list& B) {
      Bedge_list A;
      map_to_a(B, A);
      return A;
   }

   //******** SIDE A / SIDE B FACES ********

   Bface_list side_a_faces() const { return _ref_faces; }
   Bface_list side_b_faces() const { return map_to_b(_ref_faces); }

   //******** Rsurface LOOKUP ********

   // Returns the Rsurface that owns the boss meme on the simplex:
   static Rsurface* find_owner(CBsimplex* s) {
      return upcast(Bbase::find_owner(s));
   }

   // Find the Rsurface owner of this simplex, or if there is none,
   // find the Rsurface owner of its subdivision parent (if any),
   // continuing up the subdivision hierarchy until we find a
   // Rsurface owner or hit the top trying.
   static Rsurface* find_controller(CBsimplex* s) {
     return upcast(Bbase::find_controller(s));
   }

   //******** BUILDING ********

   // Sew the "ribbons" joining sides A and B
   static bool sew_ribbons(CBface_list& side_b, MULTI_CMDptr cmd);

   // In case the given region is part of a side A or side B inflate,
   // then get the other side and sew up ribbons connecting the new
   // holes. It may be necessary to punch out the other side too.
   // Returns true on success.
   static bool handle_punch(CBface_list& region, MULTI_CMDptr cmd);

   //******** SUBDIVISION Rsurfaces ********

   // The subdiv parent and child Rsurfaces:
   Rsurface* parent_rsurf()   const   { return (Rsurface*) _parent; }
   Rsurface* child_rsurf()    const   { return (Rsurface*) _child; }

   void adopt(Rsurface* child);

   // Returns the side A parent simplex
   // of the given "side B" vertex:
   Bsimplex* parent_simplex_A(Rmeme* child_vert_B) const;

   // given side-B child vert, returns parent vertex or edge,
   // respectively (or null if it doesn't exist):
   Lvert* parent_vert(Rmeme* child_vert_B) const;
   Ledge* parent_edge(Rmeme* child_vert_B) const;

   //******** NOTIFICATION ********
   void ref_face_deleted(Bface* a);

   //******** Bbase VIRTUAL METHODS ********

   virtual void produce_child();

   virtual void draw_debug();

   //******** Bnode METHODS ********

   virtual Bnode_list inputs() const {
      return Bsurface::inputs() + _inputs;
   }

 protected:
   //******** MEMBER DATA ********
   Bface_list   _ref_faces; // reference faces to be replicated
   Bnode_list   _inputs;    // controllers of the reference surface

   //******** STATIC DATA ********
   static Rsurface_list _rsurfs;        // existing Rsurfaces

   //******** BUILDING ********

   void gen_verts(double h, bool scale_h=true );
   void gen_faces(bool flip_norms=false);
   void set_face_pair(Bface* a);

   static void correct_ribbon_boundary(
      CEdgeStrip& boundary,
      CBface_list& B,
      MULTI_CMDptr cmd
      );

   //******** Bnode PROTECTED METHODS ********
   virtual void recompute();
};
typedef const Rsurface CRsurface;

/*****************************************************************
 * Rsurface_list
 *****************************************************************/
class Rsurface_list : public _Bbase_list<Rsurface_list,Rsurface> {
 public:
   //******** MANAGERS ********
   Rsurface_list(int n=0) :
      _Bbase_list<Rsurface_list,Rsurface>(n)    {}
   Rsurface_list(const Rsurface_list& list) :
      _Bbase_list<Rsurface_list,Rsurface>(list) {}
   Rsurface_list(Rsurface* s) :
      _Bbase_list<Rsurface_list,Rsurface>(s)    {}
};
typedef const Rsurface_list CRsurface_list;


inline Rmeme*
Rsurface::rmeme(int i) const
{
   return (Rmeme*) _vmemes[i];
}

#endif // RSURF_H_IS_INCLUDED

// end of file rsurf.H
