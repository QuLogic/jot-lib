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
 * action.H
 **********************************************************************/
#ifndef ACTION_H_IS_INCLUDED
#define ACTION_H_IS_INCLUDED

#include "net/data_item.H"
#include "tess/tess_cmd.H"
#include "tess/bsurface.H"

class Action;
class Script;
class TEXBODY;
class Panel;

/*****************************************************************
 * Action_list:
 *
 *  List of actions, with convenience methods.
 *
 *****************************************************************/
class Action_list : public ARRAY<Action*> {
 public:

   //******** MANAGERS ********

   Action_list(int n=0)                 : ARRAY<Action*>(n) {}
   Action_list(const ARRAY<Action*>& l) : ARRAY<Action*>(l) {}

   // delete the actions and reset list to empty:
   void delete_all();

   //******** BUILDING ********

   void prepare_writing() const;
   void apply_indices() const;

   //******** DEPENDENCIES ********

   // predecessors of all actions in this list:
   Action_list predecessors() const;

   //******** QUERIES ********

   // returns true if all actions in given list are in this script:
   // XXX - should probably be added to ARRAY subclass:
   bool contains(const Action_list& list) const;

   // returns true if every predecessor of an action in the list
   // is also in the list:
   bool self_contained() const { return contains(predecessors()); }

   // return true if all actions can be invoked (or list is empty):
   bool can_invoke() const;

   // invoke all actions:
   bool invoke() const;

   // were all actions already invoked?
   bool was_invoked() const;

   // are all the actions viable?
   bool are_viable() const { return was_invoked() || can_invoke(); }

   //******** I/O ********

   // write the list of actions to the output stream:
   friend STDdstream& operator<<(STDdstream& ds, const Action_list& al);

   // read a list of actions from an input stream:
   friend STDdstream& operator>>(STDdstream& ds, Action_list& al);
};
typedef const Action_list CAction_list;

/*****************************************************************
 * Action:
 *
 *   Basic element of a script used to procedurally model
 *   shapes in free-form sketch. An action records data needed
 *   to generate part of a shape, usually by generating a new
 *   primitive.
 *
 *****************************************************************/
class Action : public DATA_ITEM {
 public:

   //******** MANAGERS ********

   // no public constructor for this abstract base class

   virtual ~Action() {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("Action", Action*, DATA_ITEM, CDATA_ITEM*);

   //******** ACCESSORS ********

   const MULTI_CMDptr& cmd() const { return _cmd; }

   //******** VIRTUAL METHODS ********

   // the list of actions that this action depends on.
   // e.g., a "create curve" action depends on two 
   // "create point" actions to provide its endpoints.
   virtual Action_list predecessors() const { return Action_list(); }

   // the following are used to write or read a list of actions.
   // (also used in copy/paste). each action may have predecessors
   // (see above) which are recorded with pointers.  to write the
   // actions, we convert the pointers to integers representing
   // indices in a list, and update member variables (prepare_writing).
   // when we read the actions
   // from file, we need to map the indices back to pointers
   // (apply_indices).
   virtual void prepare_writing(const Action_list&) {}
   virtual void apply_indices(const Action_list&) {}

   // is the action valid, i.e. ready to be invoked?
   virtual bool can_invoke() const = 0;

   // invoke this action:
   virtual bool invoke() = 0; 

   // was this action already invoked?
   virtual bool was_invoked() const = 0;

   // was it invoked, or can it be?
   bool is_viable() const { return was_invoked() || can_invoke(); }

 protected:

   //******** MEMBER DATA ********

   // List of commands that together do the work of this action.
   // Any undoable steps carried out by this Action should be
   // handled via COMMANDS that are added to this list:
   MULTI_CMDptr _cmd;

   //******** PROTECTED METHODS ********

   Action() : _cmd(new MULTI_CMD()) {}
};
typedef const Action CAction;

/*****************************************************************
 * Script:
 *
 *  Trying out the idea that a Script is a kind of Action, 
 *  in addition to containing a list of actions.
 *
 *  Still not sure if we need this class...
 *****************************************************************/
class Script : public Action {
 public:

   //******** MANAGERS ********
   Script() {}

   // destructor deletes all the actions in the script:
   virtual ~Script() { _actions.delete_all(); }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("Script", Script*, Action, CDATA_ITEM*);

   //******** ACCESSORS ********

   // returns true if the action is in this script:
   bool contains(Action* a) const {
      return _actions.ARRAY<Action*>::contains(a);
   }

   // returns true if all actions in given list are in this script:
   bool contains(const Action_list& list) const {
      return _actions.contains(list);
   }

   // returns true if every predecessor of an action in the script
   // is also in the script:
   bool self_contained() const { return _actions.self_contained(); }

   bool empty() const { return _actions.empty(); }
   int  num()   const { return _actions.num(); }

   //******** BUILDING ********

   // clear the script:
   void clear() { _actions.clear(); }

   // unlike a raw Action_list, a Script prevents you from adding
   // a null pointer or duplicate action to the list:
   bool add(Action* a);

   //******** Action VIRTUAL METHODS ********

   // a script has no predecessors (XXX - correct?)

   virtual bool can_invoke () const { return _actions.can_invoke(); }
   virtual bool invoke     ()       { return _actions.invoke(); }
   virtual bool was_invoked() const { return _actions.was_invoked(); }

   //******** DATA_ITEM METHODS ********
   
   virtual CTAGlist&  tags() const;
   virtual DATA_ITEM* dup()  const { return new Script(); }

   //******** I/O ********

   // used by TAG_meth for I/O:
   virtual void put_actions(TAGformat&) const;
   virtual void get_actions(TAGformat&);

 protected:
   //******** MEMBER DATA ********
   Action_list          _actions; // list of actions, in order
   static TAGlist*      _tags;
};

/*****************************************************************
 * BpointAction
 *****************************************************************/
 //! \brief Action that creates a Bpoint
class BpointAction : public Action {
 public:

   //******** MANAGERS ********

   // need this for the DECODER table:
   BpointAction();

   static Bpoint* create(
      LMESHptr      m,  // mesh to use
      CWpt&         o,  // point origin
      CWvec&        n,  // normal direction
      CWvec&        t,  // tangent direction
      int           r,  // res level
      MULTI_CMDptr cmd  // undoable command that this action is part of
      );
   virtual ~BpointAction();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("BpointAction", BpointAction*, Action, CDATA_ITEM*);

   //******** ACCESSORS ********

   Bpoint* bpoint() const { return _p; }

   //******** Action METHODS ********

   virtual void   prepare_writing(const Action_list&);
   virtual void apply_indices(const Action_list&);

   virtual bool can_invoke () const;
   virtual bool invoke     ();
   virtual bool was_invoked() const { return _p != 0; }

   //******** DATA_ITEM METHODS ********
   
   virtual CTAGlist&  tags() const;
   virtual DATA_ITEM* dup()  const { return new BpointAction(); }

   //******** I/O ********

   // used by TAG_val for I/O:
   int&  mesh_num()     { return _mesh_num; }
   Wpt&  origin()       { return _o; }
   Wvec& n_vec()        { return _n; }
   Wvec& t_vec()        { return _t; }
   int&  r_lev()        { return _res_level; }

 protected:
   //******** MEMBER DATA ********

   // coordinate frame data:
   Wpt          _o;        // "origin" or location of point
   Wvec         _n;        // "normal"
   Wvec         _t;        // "tangent"
   int          _res_level;// Bbase "res level"

   // "indices" used in apply_indices():
   int          _mesh_num; // which mesh to use in the known TEXBODY?

   // pointers that result after apply_indices():
   LMESHptr     _mesh;     // mesh to use

   // after being invoked:
   Bpoint*      _p;

   static TAGlist* _tags;

   //******** MANAGERS ********

   BpointAction(LMESHptr m,  // mesh to use
                CWpt&    o,  // point location
                CWvec&   n,  // "normal"
                CWvec&   t,  // "tangent"
                int      r); // res level
};

/*****************************************************************
 * BcurveAction
 *****************************************************************/
 //! \brief Action that creates a Bcurve
class BcurveAction : public Action {
 public:

   //******** MANAGERS ********

   // need this for the DECODER table:
   BcurveAction();
   virtual ~BcurveAction();

   static Bcurve* create(
      LMESHptr      m,  // mesh to use
      CWpt_list& wpts,  // path of curve
      CWvec&        n,  // plane normal associated w/ curve
      int   num_edges,  // number of edges to use in Bcurve
      int           r,  // res level
      Bpoint*      b1,  // first end point
      Bpoint*      b2,  // 2nd end point
      MULTI_CMDptr cmd  // undoable command that this action is part of
      );
   static Bcurve* create_circle(
      CWplane&     P,   // plane
      CWpt&        c,   // center
      double       r,   // radius
      int          n,   // number of edges in control mesh
      LMESHptr     mesh,// mesh to use
      MULTI_CMDptr cmd  // undoable command that this action is part of
      );

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("BcurveAction", BcurveAction*, Action, CDATA_ITEM*);

   //******** ACCESSORS ********

   Bcurve* bcurve() const { return _c; }

   //******** UTILITIES ********

   // if given action is a BcurveAction, return the Bcurve:
   static Bcurve* get_curve(Action* a) {
      BcurveAction* c = dynamic_cast<BcurveAction*>(a);
      return c ? c->bcurve() : 0;
   }
   // convert action list to bcurve list:
   static Bcurve_list a2c(CAction_list& a);

   // convert bcurve list to action list:
   static Action_list c2a(CBcurve_list& c);

   //******** Action METHODS ********

   virtual void   prepare_writing(const Action_list&);
   virtual void apply_indices(const Action_list&);

   virtual Action_list predecessors() const;

   virtual bool can_invoke() const;
   virtual bool invoke    ();
   virtual bool was_invoked() const { return _c != 0; }

   //******** DATA_ITEM METHODS ********
   
   virtual CTAGlist&  tags() const;
   virtual DATA_ITEM* dup()  const { return new BcurveAction(); }

   //******** I/O ********

   // used by TAG_val for I/O:
   Wpt_list& pts()      { return _pts; }
   Wvec&     n_vec()    { return _n; }
   int&      n_edge()   { return _num_edges; }
   int&      r_lev()    { return _res_level; }

   int&      mesh_num() { return _mesh_num; }
   int&      i1()       { return _i1; }
   int&      i2()       { return _i2; }

 protected:
   //******** MEMBER DATA ********

   // before being invoked:

   // curve shape info:
   Wpt_list     _pts;      // path of curve
   Wvec         _n;        // plane normal associated with curve
   int          _num_edges;// number of edges to use in curve
   int          _res_level;// Bbase "res level"

   // "indices" used in apply_indices():
   int          _mesh_num; // which mesh to use in the known TEXBODY?
   int          _i1;       // index of b1
   int          _i2;       // index of b2

   // pointers that result after apply_indices():
   BpointAction* _b1;       // 1st endpoint
   BpointAction* _b2;       // 2nd endpoint
   LMESHptr      _mesh;     // mesh to use

   // after being invoked:
   Bcurve*      _c;

   static TAGlist* _tags;

   //******** MANAGERS ********

   BcurveAction(
      LMESHptr     m,
      CWpt_list&   pts,
      CWvec&       n,
      int          num_edges,
      int          r,
      Bpoint*      b1,
      Bpoint*      b2
      );

   //******** UTILITIES ********
   static Bpoint* bp(BpointAction* a) { return a ? a->bpoint() : 0; }
   Bpoint* bp1() const { return bp(_b1); }
   Bpoint* bp2() const { return bp(_b2); }
};

/*****************************************************************
 * PanelAction
 *****************************************************************/
 //! \brief Action that creates a Panel from a set of boundary curves
class PanelAction : public Action {
 public:

   //******** MANAGERS ********

   // create a general panel (tri, quad, disk, or multi)
   static Panel* create(CBcurve_list& contour, MULTI_CMDptr cmd);

   // create a circular disk:
   static Panel* create(
      CWplane&     P,      // plane
      CWpt&        c,      // center
      double       r,      // radius
      LMESH*       mesh,   // mesh to use
      int          samples,// number of control verticies
      MULTI_CMDptr cmd     // undoable command that this action is part of
      );

   // need this for the DECODER table:
   PanelAction();

   virtual ~PanelAction();

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("PanelAction", PanelAction*, Action, CDATA_ITEM*);

   //******** ACCESSORS ********

   Panel* panel() const { return _p; }
   
   //******** Action METHODS ********

   virtual void   prepare_writing(const Action_list&);
   virtual void apply_indices(const Action_list&);

   virtual Action_list predecessors() const { return _actions; }

   virtual bool can_invoke() const;
   virtual bool invoke    ();
   virtual bool was_invoked() const { return _p != 0; }

   //******** DATA_ITEM METHODS ********
   
   virtual CTAGlist&  tags() const;
   virtual DATA_ITEM* dup()  const { return new PanelAction(); }

   //******** I/O ********

   // used by TAG_val for I/O:
   ARRAY<int>&  indices() { return _indices; }

 protected:
   //******** MEMBER DATA ********

   bool _creating;       // creating or loading

   // before being invoked:

   // "indices" used in apply_indices():
   ARRAY<int>   _indices; // curve indices

   // pointers that result after apply_indices():
   Action_list  _actions; // actions that define boundary curve

   // after being invoked:
   Bcurve_list  _contour; // same curves corresponding to _actions
   Panel*       _p;       // resulting Panel

   static TAGlist* _tags;

   //******** MANAGERS ********

   PanelAction(CBcurve_list& contour);

   //******** UTILITIES ********
};

#endif // ACTION_H_IS_INCLUDED

// end of file action.H
