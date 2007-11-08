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
#include "std/fstream.H"
#include "bnode.H"

static bool debug = Config::get_var_bool("DEBUG_BNODE", false);

int        Bnode::_next_num = 0;
Bnode_list Bnode::_all_bnodes(1024);
Bnode_list Bnode::_active_bnodes(1024);
bool       Bnode::_is_active_sorted = false;


void
Bnode::destructor()
{
   unhook();            // get out of dependency graph
   if (is_active()) {
      deactivate();     // get out of active list
   }

   static bool debug = Config::get_var_bool("DEBUG_BNODE_DESTRUCTOR",false);
   if (debug) {
      cerr << class_name() << "::~" << class_name()
           << " (" << identifier() << ")" << endl;
   }
   if (!outputs().empty()) {
      cerr << "Bnode::destructor: deleted Bnode outputs not empty:"
           << endl << "  ";
      outputs().print_identifiers();
   }
   if (is_active()) {
      cerr << "Bnode::destructor: deleted Bnode is still active" << endl;
   }
}

bool 
Bnode::tick() 
{
   // Callback invoked once per frame for "active" Bnodes.
   // Returns true if it wants to remain active:

   static bool debug = ::debug || Config::get_var_bool("DEBUG_BNODE_TICK", false);

   err_adv(debug, "Bnode::tick: %s returning false", **identifier());
   return false; 
}

void
Bnode::activate()
{
   if (_is_active)
      return;
   _is_active = true;
   _active_bnodes += this;
   _is_active_sorted = false;
}

void
Bnode::deactivate()
{
   static bool debug = ::debug || Config::get_var_bool("DEBUG_BNODE_TICK", false);
   if (debug) {
      cerr << identifier() << ": deactivate"
           << (_is_active ? "" : ": was not active")
           << endl;
   }
   if (!_is_active)
      return;
   _is_active = false;
   if (!(_active_bnodes -= this))
      cerr << "Bnode::deactivate: error: cannot remove "
           << identifier()
           << " from active list"
           << endl;
   _is_active_sorted = false;
}

void 
Bnode::sort_active_nodes()
{
   // Ensure active Bnodes are sorted topologically:
   if (!_is_active_sorted) {
      // the sorted list, which may include nodes not in the active list:
      Bnode_list sorted = _active_bnodes.topological_sort();
      // clear the active list, then copy back in the "active" nodes:
      _active_bnodes.clear();
      for (int i=0; i<sorted.num(); i++)
         if (sorted[i]->_is_active)
            _active_bnodes += sorted[i];
   }
}

void
Bnode::apply_frame_cb()
{
   static bool debug = ::debug || Config::get_var_bool("DEBUG_BNODE_TICK", false);

   // Run through the active list, call Bnode::tick() on each Bnode,
   // removing those that become inactive:

   // Make sure they are sorted by dependency:
   // (later ones depending only on earlier ones)
   sort_active_nodes();

   // list of Bnodes that are finished being active:
   Bnode_list done_nodes(_active_bnodes.num());

   // send the frame callback to each Bnode:
   for (int i=0; i<_active_bnodes.num(); i++) {
      if (!_active_bnodes[i]->tick()) {
         done_nodes += _active_bnodes[i];
      }
   }

   if (debug && !done_nodes.empty()) {
      cerr << "Bnode::apply_frame_cb: de-activating ";
      done_nodes.print_identifiers();
   }

   // deactivate the ones that finished:
   done_nodes.deactivate();
}

void 
Bnode::hookup() 
{
   Bnode_list ins = inputs();
   ins.add_output(this);
   if (ins.any_dirty() && !_dirty) {
      if (debug)
         cerr << identifier() << ":hookup: came in late, invalidating" << endl;
      _dirty = true;
   }
}

void 
Bnode::invalidate() 
{
   // Mark this and all downstream nodes dirty:
   
   if (_dirty)
      return;        // no-op

   // Invalidate this and downstream nodes
   _dirty = 1;

   static bool debug = ::debug ||
      Config::get_var_bool("DEBUG_BNODE_INVALIDATE", false);
   if (debug && !outputs().empty())
      cerr << identifier() << ": invalidating outputs" << endl;
   outputs().invalidate();
}

// probably there's a standard way to do this:
// write some spaces to the given output stream
inline ostream&
space(ostream& out, int indent)
{
   for (int i=0; i<indent; i++)
      out << " ";
   return out;
}

void 
Bnode::update() 
{
   // Recompute this node after recomputing all upstream nodes:

   // If nothing to do, return
   if (!_dirty)
      return;

   // for indenting err messages:
   static int indent  = 0;

   indent++;
   if (debug) {
      space(cerr, indent) << identifier() << ": updating inputs..." << endl;
   }

   // Before recomputing, update upstream nodes
   inputs().update();

   if (debug) {
      space(cerr, indent) << identifier() << ": recomputing" << endl;
   }
   indent--;            // unindent

   // Now recompute with up-to-date inputs
   recompute();

   // Mark it clean
   _dirty = 0;
}

void 
Bnode::print_dependencies() const 
{
   cerr << identifier() << ":" << endl
        << " inputs:  ";
   inputs() .print_identifiers();
   cerr << " outputs: ";
   outputs().print_identifiers();
}

void
Bnode::print_all_outputs() const
{
   cerr << identifier() << ":" << " all outputs:" << endl;
   for (Bnode_list out=outputs(); !out.empty(); out = out.outputs()) {
      cerr << "  ";
      out.print_identifiers();
   }
}

void
Bnode::print_all_inputs() const
{
   Bnode_list all;
   cerr << identifier() << ":" << " all inputs:" << endl;
   for (Bnode_list in=inputs(); !in.empty(); in = in.inputs()) {
      all += in;
   }
   for (int i=0; i<all.num(); i++)
      space(cerr, 2) << all[i]->identifier() << endl;
}

static void 
print_input_edges(Bnode* b, ostream& out, CMOD& cur)
{
   if (!b || b->get_mod().current())
      return;

   b->set_mod(cur);
   Bnode_list nodes = b->inputs();
   for (int i=0; i< nodes.num(); i++) {
      print_input_edges(nodes[i], out, cur);
      out << "   " << nodes[i]->identifier()
          << " -> " << b->identifier() << ";"
          << endl;
   }
}

void
Bnode::print_input_graph() const
{
   // Print the dependency graph for this Bnode in graphvis format
   // (see http://www.graphviz.org/):

   ofstream fout;
   str_ptr filename = identifier() + "_input_graph.dot";
   fout.open(**filename, ios::trunc);
   if (!fout) {
      cerr << "Bnode::print_input_graph: could not open output file: "
           << filename << endl;
      return;
   }
   fout << "digraph G {" << endl;
   MOD::tick();
   print_input_edges((Bnode*)this, fout, MOD());
   fout << "}" << endl;
}

void
Bnode::append_nodes_dfs(Bnode_list& ret)
{
   if (flag() != 0)
      return;
   set_flag(1);
   Bnode_list o = outputs();
   for (int i=0; i<o.num(); i++)
      o[i]->append_nodes_dfs(ret);
   ret += this;
}

Bnode_list
Bnode_list::topological_sort() const
{
   Bnode::clear_all_flags();
   Bnode_list ret;
   for (int i=0; i<num(); i++)
      bnode(i)->append_nodes_dfs(ret);
   ret.reverse();

   bool debug = ::debug || Config::get_var_bool("DEBUG_BNODE_TOPO_SORT",false);
   if (debug && !ret.empty()) {
      cerr << "Bnode_list::topological_sort: result:" << endl << "  ";
      ret.print_identifiers();

      cerr << "graph is " << (ret.is_topo_sorted() ? "" : "not ") << "sorted"
           << endl;

      Bnode* b = ret.first(); assert(b);
      cerr << endl << b->identifier() << " inputs:" << endl;
      b->print_all_inputs();

      cerr << endl << b->identifier() << " outputs:" << endl;
      b->print_all_outputs();

      b->print_input_graph();
   }
   return ret;
}

void
append_upstream_nodes(Bnode* b, CMOD& cur, Bnode_list& ret)
{
   if (!b || b->get_mod().current())
      return;
   b->set_mod(cur);
   Bnode_list nodes = b->inputs();
   for (int i=0; i< nodes.num(); i++) {
      append_upstream_nodes(nodes[i], cur, ret);
      ret += b;
   }
}

Bnode_list
upstream_nodes(Bnode* b)
{
   Bnode_list ret;
   MOD::tick();
   append_upstream_nodes(b, MOD(), ret);
   return ret;
}

bool 
Bnode_list::is_topo_sorted() const
{
   // For testing/validation:
   // Returns true if the list is ordered to respect
   // the dependency graph, with downstream nodes
   // always coming later in the list:

   bool debug = ::debug || Config::get_var_bool("DEBUG_BNODE_TOPO_SORT",false);
   bool ret = true;
   for (int i=0; i<num(); i++) {
      Bnode_list up = upstream_nodes(bnode(i));
      for (int j=0; j<up.num(); j++) {
         if (get_index(up[j]) > i) {
            if (debug) {
               // don't bail out early; print info about mis-matches
               ret = false;
               cerr << "  mis-match: input " << up[j]->identifier()
                    << " is after " << bnode(i)->identifier() << endl;
            } else {
               return false;
            }
         }
      }
   }
   return ret;
}

Bnode_list
Bnode_list::outputs() const
{
   Bnode_list ret;
   for (int i=0; i<_num; i++)
      ret += _array[i]->outputs();
   return ret;
}

Bnode_list
Bnode_list::inputs() const
{
   Bnode_list ret;
   for (int i=0; i<_num; i++)
      ret += _array[i]->inputs();
   return ret;
}

// end of file bnode.C
