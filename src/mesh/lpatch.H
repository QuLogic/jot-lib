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
#ifndef LPATCH_H_HAS_BEEN_INCLUDED
#define LPATCH_H_HAS_BEEN_INCLUDED

#include "patch.H"
#include "lmesh.H"

/**********************************************************************
 * Lpatch:
 *
 *   Specialized kind of Patch that deals with subdivision hierarchy.
 *   Each Lpatch has a child patch at the next finer level in the
 *   hierarchy. It may also have a parent. When it has no parent, it
 *   is the "control patch." The control patch is responsible for
 *   keeping a list of GTextures (like a normal Patch). Child patches
 *   don't bother with GTextures. Instead, the GTextures of the
 *   control patch are used to render the child patches (according to
 *   the "current" subdivision level).
 *
 *   It can happen that a control patch is NOT in the control mesh.
 *   This is used to represent a piece of mesh that is added to an
 *   LMESH at level k > 0.
 *
 **********************************************************************/
class Lpatch : public Patch {
   friend class LMESH;
 public:
   //******** MANAGERS ********
   virtual ~Lpatch();

   //******** ACCESSORS ********

   // Owning mesh, conveniently upcast to LMESH:
   LMESH*  lmesh()       const { return (LMESH*)_mesh; }
   
   // Parent and child patches in the subdivision hierarchy:
   Patch* parent()       const { return (Patch*)_parent; }
   Patch* child()        const { return (Patch*)_child; }

   // More upcasts (convenience):
   Lface*  lface(int i)  const { return (Lface*)_faces[i]; }
   Lstrip* lstrip(int i) const { return (Lstrip*)_tri_strips[i]; }

   //******** SUBDIVISION HIERARCHY ********

   // Control mesh for the subdivision hierarchy:
   LMESH* control_mesh() const { return _mesh ? lmesh()->control_mesh() : 0; }

   // Level of the currently drawn mesh in the hierarchy:
   int cur_level()       const { return _mesh ? lmesh()->cur_level() : 0; }

   // Returns the child patch at the given subdivision level
   // RELATIVE to this patch. E.g.:
   //
   //     k | returned Patch
   // ---------------------
   //    -1 | _parent
   //     0 | this
   //     1 | _child
   //     2 | _child->_child
   //      ...
   //
   Lpatch* sub_patch(int k);

   void delete_child() { delete _child; _child = 0;}
   

   //******** Patch VIRTUAL METHODS ********

   // Returns the corresponding patch at the "current"
   // subdivision level:
   virtual Patch* cur_patch() {
      return sub_patch(cur_level() - subdiv_level());
   }

   // Produces (if needed) and returns a child patch
   // at the next subdivision level:
   virtual Patch* get_child();

   // enters into child relationship w/ given patch,
   // if it's legal. returns true on success
   bool set_parent(Patch* p);

   // Returns the highest-level parent Patch of this one.
   // NB. the control patch may not belong to the control mesh,
   // in the case that a patch has been added at a level > 0.
   virtual Patch* ctrl_patch() const {
      return _parent ? _parent->ctrl_patch() : (Patch*)this;
   }

   bool is_ctrl_patch() const { return _parent == NULL; }

   // The level of this Patch relative to its control Patch:
   virtual int rel_subdiv_level() {
      return subdiv_level() - ctrl_patch()->subdiv_level();
   }

   // Returns mesh elements at current subdivision level:
   virtual CBface_list& cur_faces()     const;
   virtual Bvert_list   cur_verts()     const;
   virtual Bedge_list   cur_edges()     const;

   bool faces_at_level ( int l, Bface_list& faces );

   // Returns number of faces at current subdiv level:
   virtual int num_faces() const;

   // Diagnostic:
   virtual double tris_per_strip() const;

   virtual int  draw_tri_strips(StripCB*);
   virtual int  draw_sil_strips(StripCB*);

   //******** VERSIONING/CACHING ********
   virtual void triangulation_changed();

   // used by textures to tell if they are up-to-date
   // (e.g., display lists may be out of date):
   virtual void changed() { ctrl_patch()->Patch::changed(); }
   virtual uint stamp()   { return ctrl_patch()->Patch::stamp(); }

   //******** RefImageClient METHODS ********
   virtual int draw(CVIEWptr&);

   //**************** DATA_ITEM methods ****************

   virtual CTAGlist &tags	()	const;
   virtual DATA_ITEM   *dup()  const;

   static TAGlist *_lpatch_tags;
   virtual void   put_parent_patch      (TAGformat &)  const;
   virtual void   get_parent_patch      (TAGformat &);



   //***********Protected**********************************
 protected:

   Lpatch*      _parent;        // parent patch in subdivision hierarchy
   Lpatch*      _child;         // child patch in subdivision hierarchy

   //******** INTERNAL METHODS ********

   // Normal people can't make these --
   // get yours from an LMESH:
   Lpatch(LMESH *mesh) : Patch(mesh), _parent(0), _child(0) {}

   // Clear subdivision strips down to given level:
   void clear_subdiv_strips(int level);
};

#endif  // LPATCH_H_HAS_BEEN_INCLUDED

/* end of file lpatch.H */

