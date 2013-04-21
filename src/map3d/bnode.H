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
 * bnode.H
 *****************************************************************/
#ifndef BNODE_H_IS_INCLUDED
#define BNODE_H_IS_INCLUDED

#include "geom/mod.H"
#include "net/rtti.H"
#include "std/support.H"
#include "std/config.H"

#include "net/data_item.H"

class Bnode;

/*****************************************************************
 * Bnode_list:
 *
 *      ARRAY of Bnode pointers, prevents duplicates
 *      from being added to the ARRAY.
 *****************************************************************/
class Bnode_list;
typedef const Bnode_list CBnode_list;
class Bnode_list : public ARRAY<Bnode*>{
 public:

   //******** MANAGERS ********

   Bnode_list(int n=0) : ARRAY<Bnode*>(n) {
      // Prevent duplicates:
      set_unique();
   }
   Bnode_list(CARRAY<Bnode*>& l) : ARRAY<Bnode*>(l) {
      // Prevent duplicates:
      set_unique();
   }
   Bnode_list(Bnode* b1, Bnode* b2=0, Bnode* b3=0, Bnode* b4=0) :
      ARRAY<Bnode*>(4) {
      // Prevent duplicates:
      set_unique();
      if (b1) *this += b1;
      if (b2) *this += b2;
      if (b3) *this += b3;
      if (b4) *this += b4;
   }

   // Same as (*this)[i]:
   Bnode* bnode(int i) const { return (*this)[i]; }

   // Set flag of each Bnode to the given value:
   void set_flags(int f = 1) const;

   // Set flag of each Bnode to 0:
   void clear_flags() const { set_flags(0); }

   // Returns a list of Bnodes including all those present in this
   // list, plus any "downstream" nodes in the dependency graph. The
   // return list is ordered so that each Bnode shows up in the list
   // before any of its downstream nodes.
   Bnode_list topological_sort() const;

   // For testing/validation:
   // Returns true if the list is ordered to respect
   // the dependency graph, with downstream nodes
   // always coming later in the list:
   bool is_topo_sorted() const;

   //******** CONVENIENCE ********

   inline void update()                 const;
   inline void invalidate()             const;
   inline void add_output(Bnode*)       const;
   inline void rem_output(Bnode*)       const;
   inline void print_identifiers()      const;
   inline bool tick()                   const;
   inline void activate()               const;
   inline void deactivate()             const;

   bool any_dirty() const;

   // Print the full set of outputs of each Bnode in this list
   Bnode_list outputs() const;

   // Print the full set of inputs of each Bnode in this list
   Bnode_list inputs() const;
};

/*****************************************************************
 * Bnode:
 *
 *      Base class for "nodes" in a dependency graph. Each node
 *      has "inputs" (other nodes) that it depends on, and
 *      "outputs" (also nodes) that it affects. Each node stores
 *      data that affect its outputs and are affected by its
 *      inputs. For example, the endpoints of a curve affect the
 *      curve's shape, because moving an endpoint can cause the
 *      entire curve to stretch and rotate. 
 *
 *      When a node changes and needs to be recomputed, it is
 *      marked "dirty" (indicating that its data are no longer
 *      valid). When this happens, all the downstream nodes
 *      (outputs) are also marked dirty. Later, when a dirty node
 *      wants recomputes its data, it first tells its input nodes
 *      to recompute to ensure their data are valid.
 *
 *      Marking a node dirty is done via Bnode::invalidate(),
 *      which recursively calls Bnode::invalidate() on all the
 *      downstream (output) nodes. Calling Bnode::update() on a
 *      node first recursively calls Bnode::update() on all the
 *      upstream (input) nodes, then it calls Bnode::recompute().
 *      Bnode::invalidate() is a no-op if the node is already
 *      dirty. Bnode::update() is a no-op if the node is already
 *      up-to-date (not dirty).
 *
 *      Both invalidate() and update() are fully implemented in
 *      Bnode. Derived classes just need to fill in the inputs()
 *      and outputs() accessors, and the protected
 *      Bnode::recompute() method, which recomputes a node's data
 *      given that the inputs are already up-to-date.
 *****************************************************************/
class Bnode;
typedef const Bnode CBnode;
class Bnode : public DATA_ITEM {
   friend class Bnode_list;
 public:
   //******** MANAGERS ********
   Bnode() : _dirty(0),
             _flag(0),
             _num(_next_num++),
             _name(""),
             _debug(false),
             _is_active(false) {
      Bnode::_all_bnodes.add(this);
   }
   virtual ~Bnode() {
      _all_bnodes.rem(this);
   }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS_BASE("Bnode", CBnode*);

   //******** ACCESSORS ********

   int bnode_num() const { return _num; }

   Cstr_ptr& name()                     const   { return _name; }
   void set_name(Cstr_ptr& name)                { _name = name; }

   bool is_dirty() const { return _dirty; }

   CMOD& get_mod()      const   { return _mod; }
   void  set_mod(CMOD& m)       { _mod = m; }

   int  flag()          const   { return _flag; }
   void set_flag(int f)         { _flag = f; }

   bool is_active() const { return _is_active; }

   //******** VIRTUAL METHODS ********

   // Nodes affected by this:
   Bnode_list outputs() const { return _outputs; }

   // Register a generic output -- i.e. some Bnode that for
   // reasons of its own has decided it wants to know about this
   // Bnode:
   virtual void add_output(Bnode* o) { _outputs.add_uniquely(o); }
   virtual void rem_output(Bnode* o) { _outputs -= o; }

   // Inputs are handled differently -- there are no generic
   // inputs, as Bnodes generally know what their specific inputs
   // are and how they are used for recomputation.
   //
   // Nodes that affect this:
   virtual Bnode_list inputs()  const = 0;

   //******** ACTIVE BNODES ********

   // Callback invoked once per frame for "active" Bnodes.
   // Returns true if it wants to remain active:
   virtual bool tick();

   virtual void activate();     // get in the active list
   virtual void deactivate();   // get out of the active list

   // returns the list of active Bnodes:
   static CBnode_list& get_active_bnodes() { return _active_bnodes; }

   // Ensure active Bnodes are sorted topologically:
   static void sort_active_nodes();

   // Run through the active list, call Bnode::tick() on each Bnode,
   // removing those that become inactive:
   static void apply_frame_cb();

   //******** ALL BNODES ********

   // returns the list of all Bnodes:
   static CBnode_list& get_all_bnodes() { return _all_bnodes; }

   static void set_all_flags(int f=1)   { _all_bnodes.set_flags(f); }
   static void clear_all_flags()        { _all_bnodes.clear_flags(); }

   //******** MANAGING DEPENDENCIES ********

   // Mark this and all downstream nodes dirty:
   void invalidate();

   // Recompute this node after recomputing all upstream nodes:
   void update();

   //******** UTILITIES ********

   // Identifier used in diagnostic output. E.g. Bbase
   // types also print their level in the subdivision hierarchy.
   virtual str_ptr identifier() const {
      if (_name == NULL_STR)
         return class_name() + "_" + str_ptr(_num);
      return class_name() + "_" + _name + "_" + str_ptr(_num);
   }

   // Print the list of immediate inputs and outputs:
   void print_dependencies() const;

   // Print all outputs, including downstream ones:
   void print_all_outputs()  const;

   // Print all inputs, including upstream ones:
   void print_all_inputs() const;

   // Print the dependency graph for this Bnode in graphvis format
   // (see http://www.graphviz.org/):
   void print_input_graph() const;

   bool do_debug()              const   { return _debug; }
   void set_do_debug(bool b=true)       { _debug = b; }

   //******** data_item METHODS ********

   virtual DATA_ITEM   *dup()  const { return 0;}

   //*******************************************************
   // PROTECTED
   //*******************************************************
 protected:
   bool         _dirty;   // dirty flag
   Bnode_list   _outputs; // generic outputs
   int          _flag;    // value used in graph searches

   // Unique number assigned to each
   // Bnode in order of creation:
   int          _num;     

   // a name that can be assigned,
   // e.g. for debugging:
   str_ptr      _name;  

   // The following is used when applying a transform to a
   // network of Bnodes. We want to avoid applying the
   // transform more than once to any given Bnode. Rather
   // than enforce this by visiting each Bnode exactly once,
   // we allow multiple visits, but ensure that the Bnode is
   // modified only on the first visit.  That is, if its
   // sequence number is out of date, we apply the transform
   // and update the sequence number.

   MOD   _mod;   // sequence number identifying last update

   bool  _debug; // if true, extra diagnostic messages are printed in some cases

   bool  _is_active; // if true, this Bnode gets the tick() callback each frame

   //******** STATIC DATA ********

   static int           _next_num;      // number used in assigning unique names
   static Bnode_list    _all_bnodes;    // list of all existing Bnodes

   // list of "active" Bnodes that get a callback each frame:
   static Bnode_list    _active_bnodes;

   // true if _active_bnodes is sorted topologically:
   static bool          _is_active_sorted; 

   //******** UTILITY METHODS ********

   // utility method that should be called by derived classes in their
   // destructors. calling it in the Bnode destructor is not sufficient,
   // since it depends on virtual methods (e.g. Bnode::inputs()) that do
   // not return correct values in the Bnode destructor.
   virtual void destructor();

   // Convenience -- useful when first getting added to the
   // dependency network and virtual method inputs() is
   // available:
   void hookup();

   // Convenience -- useful when first getting removed from the
   // dependency network:
   void unhook() { inputs().rem_output(this); }

   // Does depth-first search to recursively append all child nodes to
   // the list first, followed by adding this Bnode to the list. Called
   // by Bnode_list::topological_sort(), which clears Bnode flags first.
   void append_nodes_dfs(Bnode_list& ret);

   //******** PURE VIRTUAL INTERNAL METHODS ********

   // Recompute data for this node, given that the inputs have
   // been brought up-to-date. It's protected because it should
   // only be called from within update() after upstream nodes
   // are updated.
   virtual void recompute() = 0;
};

/*****************************************************************
 * Bnode_list inline methods
 *****************************************************************/
inline void 
Bnode_list::update() const 
{
   for (int i=0; i<num(); i++)
      bnode(i)->update();
}

inline void 
Bnode_list::invalidate() const 
{
   for (int i=0; i<num(); i++)
      bnode(i)->invalidate();
}

inline void 
Bnode_list::add_output(Bnode* o) const 
{
   for (int i=0; i<num(); i++)
      bnode(i)->add_output(o);
}

inline void 
Bnode_list::rem_output(Bnode* o) const 
{
   for (int i=0; i<num(); i++)
      bnode(i)->rem_output(o);
}

inline void 
Bnode_list::print_identifiers() const 
{
   if (empty()) {
      cerr << "empty";
   } else {
      for (int i=0; i<num(); i++) {
         cerr << bnode(i)->identifier();
         if (i < num() - 1)
            cerr << ", ";
      }
   }
   cerr << endl;
}

inline bool
Bnode_list::tick() const 
{
   bool ret = false;
   for (int i=0; i<num(); i++)
      ret = bnode(i)->tick() || ret;
   return ret;
}

inline void 
Bnode_list::activate() const 
{
   for (int i=0; i<num(); i++)
      bnode(i)->activate();
}

inline void 
Bnode_list::deactivate() const 
{
   for (int i=0; i<num(); i++)
      bnode(i)->deactivate();
}

inline bool 
Bnode_list::any_dirty() const 
{
   for (int i=0; i<num(); i++)
      if (bnode(i)->is_dirty())
         return true;
   return false;
}

inline void 
Bnode_list::set_flags(int f) const 
{
   // Set flag of each Bnode to the given value:
   for (int i=0; i<num(); i++)
      bnode(i)->set_flag(f);
}

#endif // BNODE_H_IS_INCLUDED

// end of file bnode.H
