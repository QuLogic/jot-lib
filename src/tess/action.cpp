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
#include "tess/action.H"
#include "tess/panel.H"
#include "tess/tex_body.H"
#include "tess/ti.H"

static bool debug = Config::get_var_bool("DEBUG_ACTION",false);

/*****************************************************************
 * DECODER stuff
 *****************************************************************/
// make sure Actions can be looked up via DECODER mechanism
class DecoderAction {
 public:
   DecoderAction() {
      DECODER_ADD(Script);
      DECODER_ADD(BpointAction);
      DECODER_ADD(BcurveAction);
      DECODER_ADD(PanelAction);
   }
} DecoderAction_static;

/*****************************************************************
 * UTILITIES 
 *****************************************************************/
inline LMESHptr 
lookup_mesh(int mesh_num)
{
   TEXBODY* tx = TEXBODY::cur_tex_body();
   if (!tx) {
      err_adv(debug, "lookup_mesh: error: can't get TEXBODY");
      return nullptr;
   }
   if (!tx->meshes().valid_index(mesh_num)) {
      err_adv(debug, "lookup_mesh: error: invalid mesh num");
      return nullptr;
   }
   return dynamic_pointer_cast<LMESH>(tx->mesh(mesh_num));
}

inline int
get_mesh_num(BMESHptr m)
{
   TEXBODY* tx = TEXBODY::bmesh_to_texbody(m);
   return tx ? tx->meshes().get_index(m) : -1;
}

inline Action*
get_action(Bbase* b)
{
   return b ? b->get_action() : nullptr;
}

inline Action*
get_action(int i, const Action_list& list)
{
   if (0 <= i && i < (int)list.size())
      return list[i];
   return nullptr;
}

/*****************************************************************
 * Action_list
 *****************************************************************/
void
Action_list::delete_all()
{
   for (Action_list::size_type i=0; i<size(); i++) {
      delete at(i);
   }
   clear();
}

void
Action_list::prepare_writing() const 
{
   for (Action_list::size_type i=0; i<size(); i++)
      at(i)->prepare_writing(*this);
}

void
Action_list::apply_indices() const 
{
   for (Action_list::size_type i=0; i<size(); i++)
      at(i)->apply_indices(*this);
}

Action_list 
Action_list::predecessors() const
{
   Action_list ret;
   set<Action*> unique;

   for (Action_list::size_type i=0; i<size(); i++) {
      Action_list tmp = at(i)->predecessors();
      for (Action_list::size_type j=0; j<tmp.size(); j++) {
         pair<set<Action*>::iterator,bool> result;
         result = unique.insert(tmp[j]);
         if (result.second)
            ret.push_back(tmp[j]);
      }
   }

   return ret;
}

bool 
Action_list::contains(const Action_list& list) const 
{
   for (Action_list::size_type i=0; i<list.size(); i++) {
      Action_list::const_iterator it;
      it = std::find(begin(), end(), list[i]);
      if (it == end())
         return false;
   }
   return true;
}

bool 
Action_list::can_invoke() const 
{
   bool ret = true;

   for (Action_list::size_type i=0; i<size(); i++) {
      if (!at(i)->can_invoke()) {
         if (debug)
            cerr << "Action_list::can_invoke: can't invoke "
                 << at(i)->class_name()
                 << endl;
         ret = false;
         break;
      }
   }

   return ret;
}

bool 
Action_list::invoke() const 
{
   // XXX - for development/debug
   if (!can_invoke()) {
      err_adv(debug, "Action_list::invoke: error: can_invoke returns false");
      return false;
   }

   // even though we checked, we're still keeping track of
   // whether they do in fact report success:
   bool ret = true;
   for (Action_list::size_type i=0; i<size(); i++)
      ret = at(i)->invoke() && ret;
   assert(ret == true);
   return ret;
}

bool 
Action_list::was_invoked() const 
{
   for (Action_list::size_type i=0; i<size(); i++)
      if (!at(i)->was_invoked())
         return false;
   return true;
}

inline STDdstream& 
operator<<(STDdstream& ds, const Action_list& al) 
{
   // write the list of actions to the output stream:
   al.prepare_writing();
   ds << "{ ";
   for (Action_list::size_type i=0; i<al.size(); i++)
      al[i]->format(ds);
   ds << " }";
   return ds;
}

inline STDdstream& 
operator>>(STDdstream& ds, Action_list& al) 
{
   // read a list of actions from an input stream:
   char brace; ds >> brace;
   al.clear();
   while (ds.check_end_delim()) {
      Action* a = dynamic_cast<Action*>(DATA_ITEM::Decode(ds, false));
      if (a) {
         err_adv(debug, "Action_list::decode:  read %s", a->class_name().c_str());
         al.push_back(a);
      } else {
         err_adv(debug, "Action_list::decode: error reading action");
      }
   }
   ds >> brace;
   al.apply_indices();
   return ds;
}

/*****************************************************************
 * Script
 *****************************************************************/
TAGlist* Script::_tags = nullptr;

bool
Script::add(Action* a)
{
   // add the action to the script.

   // first be sure it's not null and isn't already in a script.
   if (!a) {
      err_adv(debug, "Script::add: null action, rejecting...");
      return false;
   }
   if (contains(a)) {
      err_adv(debug, "Script::add: action previously added, rejecting");
      return false;
   }

   // add the action to the list:
   _actions.push_back(a);
   return true;
}

CTAGlist &
Script::tags() const
{
   if (!_tags) {
      _tags = new TAGlist(Action::tags());
      _tags->push_back(new TAG_meth<Script>(
         "actions",
         &Script::put_actions,
         &Script::get_actions, 1));
   }

   return *_tags;
}

void
Script::put_actions(TAGformat &d) const
{
   if (debug) {
      cerr << "Script::put_actions: writing "
           << _actions.size() << " actions" << endl;
   }
   d.id();
   *d << _actions << "\n";
   d.end_id();
}

void
Script::get_actions(TAGformat &d)
{
   if (!_actions.empty()) {
      err_adv(debug,"Script::get_actions: warning: clearing non-empty script");
      _actions.clear();
   }
   *d >> _actions;
   if (debug) {
      cerr << "Script::get_actions: read "
           << _actions.size() << " actions: " << endl;
      //print_names<Action_list>(cerr, _actions);
   }

}

/*****************************************************************
 * BpointAction
 *****************************************************************/
TAGlist* BpointAction::_tags = nullptr;

BpointAction::BpointAction() :
   _res_level(0),
   _mesh_num(-1),
   _p(nullptr)
{
}

BpointAction::BpointAction(
   LMESHptr m,
   CWpt&    o,
   CWvec&   n,
   CWvec&   t,
   int      r) :
   _o(o),
   _n(n),
   _t(t),
   _res_level(r),
   _mesh_num(-1),
   _mesh(m),
   _p(nullptr)
{
}

BpointAction::~BpointAction()
{
}

Bpoint*
BpointAction::create(
   LMESHptr      m,
   CWpt&         o,
   CWvec&        n,
   CWvec&        t,
   int           r,
   MULTI_CMDptr  cmd)
{
   if (!m) {
      cerr << "BpointAction::create: error: null mesh" << endl;
      return nullptr;
   }
   BpointAction* ret = new BpointAction(m, o, n, t, r);
   assert(ret && ret->can_invoke());
   ret->invoke();
   
   TEXBODY* tx = TEXBODY::bmesh_to_texbody(m);
   if (tx) {
      err_adv(debug, "BpointAction::create: added action to script");
      tx->add_action(ret);
   } else {
      err_adv(debug, "BpointAction::create: no TEXBODY found");
   }
   if (cmd)
      cmd->add(ret->cmd());
   return ret->bpoint();
}

void   
BpointAction::prepare_writing(const Action_list&)
{
   if (!_p) {
      err_msg("BpointAction::prepare_writing: point was not created yet");
   } else {
      _o = _p->o();
      _n = _p->n();
      _t = _p->t();
      _res_level = _p->res_level();
      _mesh_num = get_mesh_num(_p->mesh());
   }
}

void 
BpointAction::apply_indices(const Action_list&)
{
   _mesh = lookup_mesh(_mesh_num);
}

bool
BpointAction::can_invoke() const
{
   // it's ready but hasn't yet been invoked

   return (
      !was_invoked()     &&
      _mesh != nullptr   &&
      !_n.is_null()      &&
      !_t.is_null()      &&
      _res_level >= 0    &&
      _cmd               &&
      _cmd->is_empty()
      );
}

bool
BpointAction::invoke()
{
   // Check validity:
   if (!can_invoke()) {
      err_adv(debug, "BpointAction::invoke: error: can't invoke");
      return false;
   }

   assert(_mesh != nullptr);
   assert(!_p);

   err_adv(debug, "BpointAction::invoke: creating Bpoint...");

   // XXX - initial version just handles 1 kind of Bpoint:
   _p = new Bpoint(_mesh, new WptMap(_o, _n, _t), _res_level);
   _mesh->update_subdivision(_res_level);

   // record this action as the one responsible for creating p:
   assert(_p && !_p->get_action());
   _p->set_action(this);

   assert(_cmd && _cmd->is_empty());
   _cmd->add(make_shared<SHOW_BBASE_CMD>(_p));
   return true;
}

CTAGlist &
BpointAction::tags() const
{
   if (!_tags) {
      _tags = new TAGlist(Action::tags());
      _tags->push_back(new TAG_val<BpointAction,int>(
         "mesh",
         &BpointAction::mesh_num
         ));
      _tags->push_back(new TAG_val<BpointAction,Wpt>(
         "o",
         &BpointAction::origin
         ));
      _tags->push_back(new TAG_val<BpointAction,Wvec>(
         "n",
         &BpointAction::n_vec
         ));
      _tags->push_back(new TAG_val<BpointAction,Wvec>(
         "t",
         &BpointAction::t_vec
         ));
      _tags->push_back(new TAG_val<BpointAction,int>(
         "r",
         &BpointAction::r_lev
         ));
   }

   return *_tags;
}

/*****************************************************************
 * BcurveAction
 *****************************************************************/
TAGlist* BcurveAction::_tags = nullptr;

const int NUM_EDGES = 4;

BcurveAction::BcurveAction() :
   _num_edges(NUM_EDGES),
   _res_level(0),
   _mesh_num(-1),
   _i1(-1),
   _i2(-1),
   _b1(nullptr),
   _b2(nullptr),
   _c(nullptr)
{
}

BcurveAction::BcurveAction(
   LMESHptr   m,
   CWpt_list& pts,
   CWvec&     n,
   int        num_edges,
   int        r,
   Bpoint*    b1,
   Bpoint*    b2) :
   _pts(pts),
   _n(n),
   _num_edges(num_edges),
   _res_level(r),
   _mesh_num(-1),
   _i1(-1),
   _i2(-1),
   _b1(dynamic_cast<BpointAction*>(get_action(b1))),
   _b2(dynamic_cast<BpointAction*>(get_action(b2))),
   _mesh(m),
   _c(nullptr)
{
}

BcurveAction::~BcurveAction()
{
}

Bcurve*
BcurveAction::create(
   LMESHptr     m,
   CWpt_list&   pts,
   CWvec&       n,
   int          num_edges,
   int          r,
   Bpoint*      b1,
   Bpoint*      b2,
   MULTI_CMDptr cmd)
{
   if (!m) {
      cerr << "BcurveAction::create: error: null mesh" << endl;
      return nullptr;
   }
   BcurveAction* ret = new BcurveAction(m, pts, n, num_edges, r, b1, b2);
   assert(ret && ret->can_invoke());
   ret->invoke();
   
   TEXBODY* tx = TEXBODY::bmesh_to_texbody(m);
   if (tx) {
      err_adv(debug, "BcurveAction::create: added action to script");
      tx->add_action(ret);
   } else {
      err_adv(debug, "BcurveAction::create: no TEXBODY found");
   }
   if (cmd)
      cmd->add(ret->cmd());
   return ret->bcurve();
}

Bcurve*
BcurveAction::create_circle(
   CWplane&     P,   // plane
   CWpt&        c,   // center
   double       r,   // radius
   int          n,   // number of edges in control mesh
   LMESHptr     mesh,// mesh to use
   MULTI_CMDptr cmd  // undoable command that this action is part of
   )
{
   if (!P.is_valid() || mesh == nullptr || n < 3 || r <= 0)
      return nullptr;

   // Get a coordinate system
   Wvec Z = P.normal().normalized();
   Wvec X = Z.perpend(); // is unit length
   Wvec Y = cross(Z,X);  // must be unit length
   Wtransf xf(c, X, Y, Z);

   // Make the hi-res circle for the curve's map1d3d:
   const int ORIG_RES = 128;
   Wpt_list pts(ORIG_RES + 1);
   double dt = (2*M_PI)/ORIG_RES;
   for (int i=0; i<ORIG_RES; i++) {
      double t = dt*i;
      pts.push_back(xf*Wpt(r*cos(t), r*sin(t), 0));
   }
   pts.push_back(pts[0]);       // make it closed

   int rlev = 2;
   return create(mesh, pts, Z, n, rlev, nullptr, nullptr, cmd);
}                                          

Action_list 
BcurveAction::c2a(CBcurve_list& c)
{
   Action_list ret(c.num());
   for (int i=0; i<c.num(); i++) {
      Action* a = get_action(c[i]);
      if (!a) return Action_list(); // if any fail, all fail
      else    ret[i] = a;
   }
   return ret;
}

Bcurve_list 
BcurveAction::a2c(CAction_list& a)
{
   Bcurve_list ret(a.size());
   for (Action_list::size_type i=0; i<a.size(); i++) {
      Bcurve* c = get_curve(a[i]);
      if (!c) return Bcurve_list(); // if any fail, all fail
      else    ret += c;
   }
   return ret;
}

void   
BcurveAction::prepare_writing(const Action_list& list)
{
   // map pointers to integers,
   // before writing to file or paste operation
   if (!_c) {
      err_msg("BcurveAction::prepare_writing: curve was not created yet");
   } else {
      Action_list::const_iterator it;
      _mesh_num = get_mesh_num(_c->mesh());
      it = std::find(list.begin(), list.end(), get_action(_c->b1()));
      _i1 = (it != list.end()) ? (it - list.begin()) : -1;
      it = std::find(list.begin(), list.end(), get_action(_c->b2()));
      _i2 = (it != list.end()) ? (it - list.begin()) : -1;

      _pts = _c->get_wpts();
      _n = _c->normal();
      _num_edges = _c->num_edges();
      _res_level = _c->res_level();
   }

   err_adv(debug, "BcurveAction::prepare_writing: i1: %d, i2: %d", _i1, _i2);
}

void 
BcurveAction::apply_indices(const Action_list& list)
{
   // map integers to pointers,
   // after read from file or copy operation

   _mesh = lookup_mesh(_mesh_num);
   _b1 = dynamic_cast<BpointAction*>(get_action(_i1, list));
   _b2 = dynamic_cast<BpointAction*>(get_action(_i2, list));

   if (debug) {
      cerr << "BcurveAction::apply_indices: "
           << "mesh: " << static_pointer_cast<LMESH>(_mesh)
           << ", b1: " << _b1
           << ", b2: " << _b2
           << endl;
   }
}

Action_list 
BcurveAction::predecessors() const
{
   Action_list ret;
   if (_b1) ret.push_back(_b1);
   if (_b2) ret.push_back(_b2);
   return ret;
}

bool
BcurveAction::can_invoke() const
{
   // it's ready but hasn't yet been invoked

   if (was_invoked()) {
      err_adv(debug, "BcurveAction::can_invoke: curve exists");
      return false;
   }
   if (!_mesh) {
      err_adv(debug, "BcurveAction::can_invoke: null mesh");
      return false;
   }
   if (_pts.size() < 2 || _n.is_null() || _num_edges < 1 || _res_level < 0) {
      err_adv(debug, "BcurveAction::can_invoke: bad params");
      return false;
   }
   if (!(_cmd && _cmd->is_empty())) {
      err_adv(debug, "BcurveAction::can_invoke: bad command");
      return false;
   }
   if (_b1 && _b2) {
      return _b1->is_viable() && _b2->is_viable();
   }
   if (_b1 || _b2) {
      err_adv(debug, "BcurveAction::can_invoke: error: 1 but not 2 endpoints");
      return false;
   }
   // 0 endpoints, curve must be closed
   if (!_pts.is_closed()) {
      err_adv(debug,
              "BcurveAction::can_invoke: error: no points, path not closed");
      return false;
   }
   return true;
}

bool
BcurveAction::invoke()
{
   // Check validity:
   if (!can_invoke()) {
      err_adv(debug, "BcurveAction::invoke: error: can't invoke");
      return false;
   }
   assert(!_c && _mesh);

   err_adv(debug, "BcurveAction::invoke: creating Bcurve...");

   // XXX - initial version just handles 1 kind of Bcurve:
   _c = new Bcurve(_mesh, _pts, _n, _num_edges, _res_level, bp1(), bp2());
   _mesh->update_subdivision(_res_level);

   err_adv(debug, "...done");

   // record this action as the one responsible for creating the curve
   assert(_c && !_c->get_action());
   _c->set_action(this);

   assert(_cmd && _cmd->is_empty());
   _cmd->add(make_shared<SHOW_BBASE_CMD>(_c));
   return true;
}

CTAGlist &
BcurveAction::tags() const
{
   if (!_tags) {
      _tags = new TAGlist(Action::tags());
      _tags->push_back(new TAG_val<BcurveAction,int>(
         "mesh",
         &BcurveAction::mesh_num
         ));
      _tags->push_back(new TAG_val<BcurveAction,int>(
         "b1",
         &BcurveAction::i1
         ));
      _tags->push_back(new TAG_val<BcurveAction,int>(
         "b2",
         &BcurveAction::i2
         ));
      _tags->push_back(new TAG_val<BcurveAction,Wpt_list>(
         "pts",
         &BcurveAction::pts
         ));
      _tags->push_back(new TAG_val<BcurveAction,Wvec>(
         "n",
         &BcurveAction::n_vec
         ));
      _tags->push_back(new TAG_val<BcurveAction,int>(
         "num_e",
         &BcurveAction::n_edge
         ));
      _tags->push_back(new TAG_val<BcurveAction,int>(
         "r",
         &BcurveAction::r_lev
         ));
   }

   return *_tags;
}

/*****************************************************************
 * PanelAction
 *****************************************************************/
TAGlist* PanelAction::_tags = nullptr;

PanelAction::PanelAction() :
   _creating(true),
   _p(nullptr)
{
}

PanelAction::PanelAction(CBcurve_list& contour) :
   _creating(true),
   _actions(BcurveAction::c2a(contour)),
   _p(nullptr)
{
   if ((int)_actions.size() != contour.num()) {
      err_msg("PanelAction::PanelAction: error converting curves to actions");
   }
}

PanelAction::~PanelAction()
{
}

Panel*
PanelAction::create(CBcurve_list& contour, MULTI_CMDptr cmd)
{
   LMESHptr m = contour.mesh();
   if (!m) {
      cerr << "PanelAction::create: error: null mesh from contour" << endl;
      cerr << contour << endl;
      return nullptr;
   }

   // XXX - should do other checks here...

   PanelAction* ret = new PanelAction(contour);
   assert(ret);
   if (!ret->can_invoke()) {
      cerr << "PanelAction::create: error: can't invoke action" << endl;
      // delete ret;
      return nullptr;
   }
   ret->invoke();

   TEXBODY* tx = TEXBODY::bmesh_to_texbody(m);
   assert(tx && ret->panel() && ret->panel()->texbody() == tx);
   if (tx) {
      tx->add_action(ret);
      err_adv(debug, "PanelAction::create: added action to script");
   } else {
      err_adv(debug, "PanelAction::create: no TEXBODY found");
   }
   if (cmd)
      cmd->add(ret->cmd());
   return ret->panel();
}

Panel*
PanelAction::create(
   CWplane&     P,      // plane
   CWpt&        c,      // center
   double       r,      // radius
   LMESHptr     mesh,   // mesh to use
   int          samples,// number of control verticies
   MULTI_CMDptr cmd     // undoable command that this action is part of
   )
{
   // Create a circular Panel in the given plane:
   Bcurve* border = BcurveAction::create_circle(P, c, r, samples, mesh, cmd);
   if (!border) {
      err_adv(debug, "PanelAction::create: can't create border");
      return nullptr;
   }
   return create(Bcurve_list(border), cmd);
}

void   
PanelAction::prepare_writing(const Action_list& list)
{
   // map pointers to integers,
   // before writing to file or paste operation

   if (!was_invoked()) {
      // error? seems ok...
      err_adv(debug, "PanelAction::prepare_writing: panel was not created yet");
   }

   if (debug)
      cerr << "PanelAction::prepare_writing: ";

   _indices.clear();
   for (Action_list::size_type i=0; i<_actions.size(); i++) {
      Action_list::const_iterator it;
      it = std::find(list.begin(), list.end(), _actions[i]);
      _indices.push_back((it != list.end()) ? (it - list.begin()) : -1);
      if (debug)
         cerr << _indices.back() << " ";
   }

   if (debug)
      cerr << endl;
}

void 
PanelAction::apply_indices(const Action_list& list)
{
   // map integers to pointers,
   // after read from file or copy operation

   if (!_actions.empty()) {
      err_adv(debug, "PanelAction::apply_indices: warning: clearing list");
      _actions.clear();
   }
   bool err=false;
   for (vector<int>::size_type i=0; i<_indices.size(); i++) {
      Action* a = get_action(_indices[i], list);
      if (a) {
         _actions.push_back(a);
      } else {
         err = true;
         cerr << "PanelAction::apply_indices: error looking up action "
              << i << endl;
      }
   }
   _creating = false;
   if (debug) {
      cerr << "PanelAction::apply_indices: "
           << (err ? "failed" : "succeeded") << endl;
   }
}

bool
PanelAction::can_invoke() const
{
   // it's ready but hasn't yet been invoked

   if (was_invoked()) {
      err_adv(debug, "PanelAction::can_invoke: panel exists");
      return false;
   }
   if (_actions.empty()) {
      err_adv(debug, "PanelAction::can_invoke: contour is empty");
      return false;
   }
   if (!_actions.are_viable()) {
      err_adv(debug, "PanelAction::can_invoke: contour actions not viable");
      return false;
   }
   if (!(_cmd && _cmd->is_empty())) {
      err_adv(debug, "PanelAction::can_invoke: bad command");
      return false;
   }

   // XXX - should check further conditions...

   return true;
}

bool
PanelAction::invoke()
{
   // Check validity:
   if (!can_invoke()) {
      err_adv(debug, "PanelAction::invoke: error: can't invoke");
      return false;
   }
   assert(!_p && _contour.empty());

   // at this point, we hope the predecessors have all been invoked.
   // or we could explicitly invoke them. (we need the curves).
   // for now we'll rely on the actions being listed in order,
   // so the panel will be invoked after all the boundary curves...
   if (!_actions.was_invoked()) {
      err_msg("PanelAction::invoke: error: predecessors not already invoked");
      return false;
   }

   // if they've been invoked, we can get the curves:
   _contour = BcurveAction::a2c(_actions);
   assert(_contour.num() == (int)_actions.size());
   assert(_contour.mesh());

   err_adv(debug, "PanelAction::invoke: creating panel...");
   vector<int> ns;
   for (int i = 0; i < _contour.num(); i++)
      ns.push_back(_contour[i]->num_edges());
   _p = _creating ? Panel::create(_contour) : Panel::create(_contour, ns);
   err_adv(debug, "...done");

   // record this action as the one responsible for creating the panel
   assert(_p && !_p->get_action());
   _p->set_action(this);

   assert(_cmd && _cmd->is_empty());
   _cmd->add(make_shared<SHOW_BBASE_CMD>(_p));
   return true;
}

CTAGlist &
PanelAction::tags() const
{
   if (!_tags) {
      _tags = new TAGlist(Action::tags());
      _tags->push_back(new TAG_val<PanelAction,vector<int> >(
         "indices",
         &PanelAction::indices
         ));
   }

   return *_tags;
}

// end of file action.H
