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
#include "mlib/vec3.H" // XXX - Probably doesn't need to be included here.
#include "geom/geom.H"
#include "geom/world.H"
#include "geom/body.H"
#include "std/config.H"


using mlib::Wtransf;

//
// DEFINER's can be used to define a GEOM's original geometry, CSG geometry, or
// xform.  For example, if GEOM A is grouped to (on top of) GEOM B, A's
// xf_definer (xform definer) has B as an input, and B has A as an xform output.
// When B's xform changes, GEOM::_tsort is used to find the GEOM's that need
// updating and to make sure that they are done in correct order.
//

static int dm=DECODER_ADD(DEFINER);
static int cm=DECODER_ADD(CSG_DEF);

TAGlist *DEFINER::_def_tags;

/* ---- DEFINER functions that access GEOM methods ----- */

DEFINER::DEFINER(): _out_mask(XFORM) 
{
}

CTAGlist &
DEFINER::tags() const
{
   if (!_def_tags) {
      _def_tags  = new TAGlist;
      *_def_tags += new TAG_meth<DEFINER>("out_mask",
            &DEFINER::put_outmask,&DEFINER::get_outmask, 0);
      *_def_tags += new TAG_meth<DEFINER>("inputs",
            &DEFINER::put_inputs, &DEFINER::get_inputs, 1);
   }
   return *_def_tags;
}

//
// For all of our inputs, remove the other end of each connection (the outputs)
//
DEFINER::~DEFINER()
{
   for (int i = _inputs.num()-1; i>=0; i--) {
      _inputs[i]->rem_xf_output (this);
      _inputs[i]->rem_csg_output(this);
   }
}

//
// Add a GEOM as input to our DEFINER, but also make sure that the GEOM's output
// is updated as well
//
void 
DEFINER::add_input(
   GEOM *o
   )
{
   _inputs += o; 
   o->data_xform()     += this; 
   o->data_orig_body() += this; 
}

//
// Remove a GEOM as input to our DEFINER, but also make sure that the GEOM's
// output is removed as well
//
void 
DEFINER::rem_input(
   GEOM *o
   )
{
   _inputs -= o; 
   o->data_xform()     -= this; 
   o->data_orig_body() -= this; 
}

void 
DEFINER::visit(
   MOD &mod
   ) 
{ 
   int i;
   if (_ob_delt.id().current())
      for (i = 0; i < _outputs.num(); i++)
         geom(i)->write_orig_body(geom(i)->orig_body(), mod);
   if (_xf_delt.id().current())
      for (i = 0; i < _outputs.num(); i++) {
         Wtransf xf_delt(xf_delta());
         geom(i)->write_xform(xf_delt * geom(i)->xform(), xf_delt, mod);
      }
}

// format() - from DATA_ITEM, formats GEOM to a STDdstream
// (usually to a file or to a network connection)
void
DEFINER::put_inputs(TAGformat &d) const
{
   d.id(); 
   for (int i = 0; i < _inputs.num(); i++) 
      *d << _inputs[i]->name();
   d.end_id();
}

// decode() - from DATA_ITEM, decodes GEOM from a STDdstream
// (usually from a file or from a network connection)
void
DEFINER::get_inputs(TAGformat &d)
{
   for (int k = _inputs.num() - 1; k>= 0; k--)
      rem_input(_inputs[k]);

   str_ptr str;
   GEOMptr o;
   for (int i = 0; i < 1000; i++) {
      if (!d)
         break;
      *d >> str;
      if (o = WORLD::lookup(str))
         add_input(&*o);
      else
         cerr << "DEFINER::decode- unknown object '" << str 
              << "' ref'd by " << geom()->name() << endl;
   }
}

/* ---- DEFINER sub-classes  ----- */
void   
COMPOSITE_DEF::visit(
   MOD &mod
   )
{
   if (_inputs.num() > 0) {
      int all_equal    = 1;
      int any_body_mod = 0;
      MOD cmp(_inputs[0]->data_xform().id());
   
      int i;
      for (i = 1; i < _inputs.num() && all_equal; i++)
         all_equal = (cmp == _inputs[i]->data_xform().id());
      for (i = 0; i < _inputs.num() && !any_body_mod; i++)
         any_body_mod = _inputs[i]->data_orig_body().id().current();
      if (all_equal && !_inputs[0]->data_xform().id().current())
         all_equal = 0;
      inputs_changed(all_equal, any_body_mod, mod);
   }
}

// format() - from DATA_ITEM, formats GEOM to a STDdstream
// (usually to a file or to a network connection)
STDdstream &
CSG_DEF::format(STDdstream &ds) const
{
   ds << class_name();
   // Don't need to send _data_mask - always CSG_BODY
   ds << _cutters.num();
   for (int i = 0; i < _cutters.num(); i++) {
      ds << _cutters[i]._geom->name() << (int) _cutters[i]._op;
   }
   return ds;
}

// decode() - from DATA_ITEM, decodes GEOM from a STDdstream
// (usually from a file or from a network connection)
STDdstream &
CSG_DEF::decode(STDdstream &ds)
{
   int i;
   // Now we remove all inputs and then add the new inputs
   // XXX - should only make minimal number of removes/adds
   // XXX - should not have to recode this here & in DEFINER
   for (i = _cutters.num() - 1; i>= 0; i--) {
      rem_input(_cutters[i]._geom);
   }

   // Class name has already been read off
   static int print_errs = (Config::get_var_bool("PRINT_ERRS",false,true) != 0);
   
   int num;
   ds >> num;

   if (print_errs)
      cerr << "CSG_DEF::decode: (" << num << "):";

   str_ptr str;
   GEOMptr o;
   int   int_csgop;
   for (i = 0; i < num; i++) {
      ds >> str >> int_csgop;
      o = WORLD::lookup(str);
      if (print_errs) cerr << " " << str << " " << int_csgop;
      if (o) {
         add_input(&*o, (CSGop) int_csgop);
      } else {
         cerr << "CSG_DEF::decode- could not find " 
            << str << " to csg" << endl;
      }
   }
   if (print_errs) cerr << endl;
   return ds;
}

void
CSG_DEF::compute_csg(
   MOD &mod,
   int
   )
{
   if (_cutters.num()) {
      BODYptr orig_body(geom()->orig_body());
      if (!orig_body) return; // No original body upon which to do CSG
      BODYptr b(orig_body->body_copy());
      b->transform(geom()->xform(), mod);
      for (int i = 0; i < _cutters.num(); i++) {
         GEOMptr cutby(_cutters[i]._geom);
         if (cutby) {
            GEOM *cutter=(GEOM *)&*cutby;
            BODYptr p(((BODY *) &*cutter->csg_body())->body_copy());
            p->transform(cutter->xform(), mod);
            switch (_cutters[i]._op) {
               case CSG_NONE:
             brcase CSG_SUB : b = b->subtract(p);
             brcase CSG_INT : b = b->intersect(p);
             brcase CSG_ADD : b = b->combine(p);
            }
            // If CSG error, exit
            if (!b) i = _cutters.num();
         }
      }

      // Update only if we have a valid CSG
      if (b != (BODYptr) 0) {
         b->transform(geom()->inv_xform(), mod);
         geom()->write_csg_body(b,mod);
      } else return;
   } else {
      geom()->reset_csg_body(mod);
   }
   CSGobs::notify_csg_obs(geom());
}


void  
CSG_DEF::inputs_changed(
   int   all_equal, 
   int   any_body_mod, 
   MOD  &mod
   )
{
   if (!all_equal || any_body_mod)
      compute_csg(mod);
}
