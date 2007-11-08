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
#include "std/support.H" 
#include "glew/glew.H"

#include "b_stroke_pool.H"

using namespace mlib;

/*****************************************************************
 * BStrokePool
 *****************************************************************/

static int foobag = DECODER_ADD(BStrokePool);

TAGlist*    BStrokePool::_bsp_tags   = NULL;

/////////////////////////////////////////////////////////////////
// BStrokePool Methods
/////////////////////////////////////////////////////////////////

/////////////////////////////////////
// tags()
/////////////////////////////////////

CTAGlist &
BStrokePool::tags() const
{
   if (!_bsp_tags) {
      _bsp_tags = new TAGlist;
      *_bsp_tags += new TAG_meth<BStrokePool>(
         "base_prototype",
         &BStrokePool::put_base_prototype,
         &BStrokePool::get_base_prototype,  1);
      *_bsp_tags += new TAG_meth<BStrokePool>(
         "prototype",
         &BStrokePool::put_prototypes,
         &BStrokePool::get_prototype,  1);
      *_bsp_tags += new TAG_meth<BStrokePool>(
         "stroke",
         &BStrokePool::put_strokes,
         &BStrokePool::get_stroke,  1);
   }

   return *_bsp_tags;
}

/////////////////////////////////////
// decode()
/////////////////////////////////////

STDdstream  &
BStrokePool::decode(STDdstream &ds)
{

   assert(_prototypes.num() == 1);

   clear();

   STDdstream  &ret =  DATA_ITEM::decode(ds);

   _num_strokes_used=_num;
  
   return ret;
}

/////////////////////////////////////
// get_base_prototype()
/////////////////////////////////////
void
BStrokePool::get_base_prototype(TAGformat &d)
{
//   cerr << "BStrokePool::get_base_prototype()\n";

   str_ptr str;
   *d >> str;      

   if (_prototypes[0]->class_name() != str)
   {
      cerr << "BStrokePool::get_base_prototype() - Class name " 
         << str  << " not a " << _prototypes[0]->class_name() << "!!!!\n";
      return;
   }
//
   else
   {
//      cerr << "BStrokePool::get_base_prototype() - Class name " 
//         << str  << " MATCHES " << _prototypes[0]->class_name() << "!!!!\n";
   }
//

   _prototypes[0]->decode(*d);

   assert(_edit_proto == 0);
   assert(_prototypes.num() == 1);

   set_prototype(_prototypes[0]);

}

/////////////////////////////////////
// put_base_prototype()
/////////////////////////////////////
void
BStrokePool::put_base_prototype(TAGformat &d) const
{
   d.id();
   _prototypes[0]->format(*d);
   d.end_id();
}



/////////////////////////////////////
// get_prototype()
/////////////////////////////////////
void
BStrokePool::get_prototype(TAGformat &d)
{
//   cerr << "BStrokePool::get_prototype()\n";

   str_ptr str;
   *d >> str;      

   add_prototype();

   if (_prototypes.last()->class_name() != str)
   {
      cerr << "BStrokePool::get_prototype() - Loaded class name " 
         << str  << " not a " << _prototypes.last()->class_name() << "!!!!\n";
      _prototypes.pop();
      return;
   }
//
   else
   {
//      cerr << "BStrokePool::get_prototype() - Loaded class name " 
//         << str  << " MATCHES " << _prototypes.last()->class_name() << "!!!!\n";
   }
//

   assert(_edit_proto == 0);

   _edit_proto = _prototypes.num()-1;
   
   _prototypes[_edit_proto]->decode(*d);
   
   set_prototype(_prototypes[_edit_proto]);
   
   _edit_proto = 0;

}

/////////////////////////////////////
// put_prototypes()
/////////////////////////////////////
void
BStrokePool::put_prototypes(TAGformat &d) const
{
   if (_prototypes.num() > 1)
   {
      cerr << "BStrokePool::put_prototypes() - Putting " 
            << _prototypes.num()-1 << " additional prototypes.\n";
   }

   for (int i=1; i< _prototypes.num(); i++)
   {
      d.id();
      _prototypes[i]->format(*d);
      d.end_id();
   }
}


/////////////////////////////////////
// get_stroke()
/////////////////////////////////////
void
BStrokePool::get_stroke(TAGformat &d)
{
//   cerr << "BStrokePool::get_stroke()\n";

   str_ptr str;
   *d >> str;      

   DATA_ITEM *data_item = DATA_ITEM::lookup(str);

   if (!data_item) 
   {
      cerr << "BStrokePool::get_stroke() - Class name " << str
         << " could not be found!!!!!!!" << endl;
      return;
   }
   else if (!OutlineStroke::isa(data_item)) 
   {
      cerr << "BStrokePool::get_stroke() - Class name " << str
         << " not a OutlineStroke or a derived class!!!!!!" << endl;
      return;
   }
//
   else
   {
//      cerr << "BStrokePool::get_stroke() - Loaded class name " << str  << "\n";
   }
//
   OutlineStroke *s = (OutlineStroke*)data_item->dup();   assert(s);

   s->decode(*d);

   add(s);
}

/////////////////////////////////////
// put_strokes()
/////////////////////////////////////
void
BStrokePool::put_strokes(TAGformat &d) const
{
   if (_write_strokes)
   {
      //cerr << "BStrokePool::put_strokes() - Putting " << _num_strokes_used << " strokes.\n";

      for (int i = 0; i < _num_strokes_used; i++) 
      {
         d.id();
         _array[i]->format(*d);
         d.end_id();
      }
   }
   else
   {
      //cerr << "BStrokePool::put_strokes() - NOT Putting strokes for this pool.\n" ;
   }
}


/////////////////////////////////////
// Constructor
/////////////////////////////////////

BStrokePool::BStrokePool(OutlineStroke* proto)
{   
   _prototypes += proto;
   _edit_proto = 0;
   _draw_proto = 0;
   _lock_proto = false;

   _write_strokes = true;
   _num_strokes_used = 0;

   _hide = false;
   _selected_stroke = -1;

   _cur_mesh_size = 1.0;
   _cur_period = 1.0;
}

BStrokePool::~BStrokePool()
{
   // XXX -- safe to delete proto?
   //delete _prototypes;

   int i = 0;
   while (i < _num)
      delete _array[i++];
}

//***These manage _draw_proto/_edit_proto issues

void
BStrokePool::set_lock_proto(bool l)
{
   bool apply = ( (l ? _edit_proto : _draw_proto) != get_active_proto());

   _lock_proto = l;

   if (apply) apply_active_proto();
}


void
BStrokePool::set_edit_proto(int i)
{
   assert(i<_prototypes.num());

   bool apply = ( (_lock_proto ? i : _draw_proto) != get_active_proto());

   _edit_proto = i;

   if (apply) apply_active_proto();
}

void
BStrokePool::set_draw_proto(int i)
{
   assert(i<_prototypes.num());

   bool apply = ( (_lock_proto ? _edit_proto : i) != get_active_proto());

   _draw_proto = i;

   if (apply) apply_active_proto();
}

void
BStrokePool::apply_active_proto()
{
   assert(get_active_proto() < _prototypes.num());

   OutlineStroke* s = _prototypes[get_active_proto()]; assert(s);
   for (int i=0; i<_num; i++) _array[i]->copy(*s);
   mark_dirty();
}

//***These operate using _edit_proto***

OutlineStroke*
BStrokePool::get_prototype() const
{
   return _prototypes[_edit_proto];
}

int
BStrokePool::set_prototype(OutlineStroke* p)
{
   assert(p);

   //XXX - Enforce multiprototype issue here?!
   //i.e. all protos have the period and mesh
   //size of the 1st proto. Okay, let a virtual
   //function handle this...
   

   int retval = set_prototype_internal(p);


   if(_edit_proto == get_active_proto()) apply_active_proto();

   OutlineStroke* s = _prototypes[0];
   _cur_mesh_size = s->get_original_mesh_size();   
   _cur_period = (s->get_offsets())?
                     (s->get_offsets()->get_pix_len()):
                     ((double)max(1.0f,(s->get_angle())));


   return retval;
}

int
BStrokePool::set_prototype_internal(OutlineStroke* p)
{
   assert(_prototypes.num() == 1);

   _prototypes[_edit_proto] = p;

   return 0;
}


void
BStrokePool::add_prototype()
{
   //Subclass this is you don't like it... Generally,
   //its assumed we have OutlineStrokes... If more
   //subclassed stroke issues are req'd, a subclass pool 
   //can handle it.  By the way, this outght to be called
   //an OStrokePool...

   assert(_prototypes[_edit_proto]);
   assert(_prototypes[_edit_proto]->class_name() == OutlineStroke::static_name());   

   OutlineStroke *s = (OutlineStroke *)_prototypes[_edit_proto]->dup(); assert(s);

   s->set_propagate_offsets(true);
   s->set_propagate_patch(true);

   assert(_prototypes[_edit_proto]->get_patch() == s->get_patch());

   _prototypes += s;
}

void
BStrokePool::del_prototype()
{
//   assert( _edit_proto > 0 );
   assert(_prototypes.num()>1);

   bool apply = _edit_proto == get_active_proto();

   if (_edit_proto == _prototypes.num()-1)
   {
      delete _prototypes.pop();
      _edit_proto--;

      if (_draw_proto == _prototypes.num())
         _draw_proto--;
   }
   else
   {
      delete _prototypes[_edit_proto];
      _prototypes[_edit_proto] = _prototypes.pop();

      if (_draw_proto == _prototypes.num())
         _draw_proto = _edit_proto;
   }

   if (apply) apply_active_proto();

}

//***These operate using _active_proto***

OutlineStroke*
BStrokePool::get_active_prototype() const
{
   return _prototypes[get_active_proto()];
}

OutlineStroke* 
BStrokePool::stroke_at(int i) 
{
   // sanity check
   assert((i >= -1) && (i < _num_strokes_used));

   if (i == -1)   return NULL;
   else           return _array[i];
}

OutlineStroke* 
BStrokePool::internal_stroke_at(int i) 
{
   // sanity check
   assert(i >= -1);

   if (i == -1) return NULL;

   // If i is an index to a slot that doesn'e exist,
   // add slots and fill them with a copy of the prototype
   // until we make a slot with index i.
   while (_num < i+1) 
   {
      add((OutlineStroke*)(get_active_prototype()->copy()));
      _num_strokes_used = _num;
   }

   return _array[i];
}

void
BStrokePool::draw_flat(CVIEWptr& v) 
{
   OutlineStroke *proto = get_active_prototype();

   int i;

   if (_hide) return;
   if (!proto) return;

   // XXX - This pool assumes all stroke can be drawn with the
   // monolithic prototype.  Unlike crease and decal pools,
   // these is no mix of 'effective' prototypes...

   proto->draw_start();

   for (i = 0; i < _num_strokes_used; i++) _array[i]->draw(v);
  
   proto->draw_end();
}

void
BStrokePool::mark_dirty()
{
   for (int i = 0; i < _num_strokes_used; i++) {
      _array[i]->set_dirty();
   }   
}


// Don't actually remove the stroke: exchange it with the last stroke
// in _array that is currently in use, and decrement the count of
// strokes used.  This places the "removed" stroke in the slot at the
// beginning of the unused portion of the memory pool.
int 
BStrokePool::remove_stroke(OutlineStroke* s)
{
   int i = get_index(s);

   assert(i != BAD_IND);
   assert(i < _num_strokes_used);

   OutlineStroke *ss = get_selected_stroke();

   if (s == ss)
   {
      ss = NULL;
   }
   internal_deselect();

   OutlineStroke* tmp = _array[i];   assert(tmp);   tmp->clear();
   _array[i] = _array[_num_strokes_used-1];
   _array[_num_strokes_used-1] = tmp;
   _num_strokes_used--;

   if (ss)
   {
      i = get_index(ss);

      assert(i != BAD_IND);
      assert(i < _num_strokes_used);

      internal_select(i);
   }

   assert (_num_strokes_used <= _num);

   return 1;
}


static void
read_stroke(istream &is, OutlineStroke*& stroke)
{
   STDdstream d(&is);

   //Grab the class name... should be OutlineStroke
   str_ptr name;
   d >> name;      

//    cerr << "BStrokePool<Global>::read_stroke() - Got class name: "
//         << name << endl;

   DATA_ITEM *data_item = DATA_ITEM::lookup(name);

   if (!data_item) 
   {
      cerr << "BStrokePool<Global>::read_stroke() - Class name " << name 
         << " could not be found!!!!!!!" << endl;
      return;
   }
   if (!OutlineStroke::isa(data_item)) 
   {
      cerr << "BStrokePool<Global>::read_stroke() - Class name " << name 
         << " not a OutlineStroke or a derived class!!!!!!" << endl;
      return;
   }

   //Okay, if a stroke came in, we're hoping its a pre-set
   //prototype (not just some stroke in the pool.) As such,
   //it might have non-copying flags (like propogate patch)
   //set, so we'll just use the given stroke to decode.
   //Just need to check that the class is correct (sanity!!)
   if (stroke) 
   {
      if (stroke->class_name() == data_item->class_name())
      {
//          cerr << "BStrokePool<Global>::read_stroke() - Existing " <<
//             name << " is decoding...\n";
      }
      else
      {
         cerr << "BStrokePool<Global>::read_stroke() - Stroke for filling NOT null!!" <<
            " AND it's WRONG class, " << name << ", so bailing!!!!!!!!!!!!!!!!!!!!!!!\n";
         return;
      }
   }
   else
   {
      stroke = (OutlineStroke*)data_item->dup();   assert(stroke);
   }

   stroke->decode(d);
}

istream&
BStrokePool::read_stream(istream &is)        
{
//    cerr << "BStrokePool::read_stream()" << endl;

   assert(_prototypes.num()==1);
   
   // Must read in a prototype
   read_stroke(is, _prototypes[0]);

   int foo;
   int num_protos;
   int num_strokes; 
   is >> foo; 
   
   if (foo == -1)
   {
      is >> num_protos;

      for (int i=1; i<num_protos; i++)
      {
         add_prototype();
         read_stroke(is, _prototypes[i]);
      }
      assert(_prototypes.num()==num_protos);

      is >> num_strokes; 
   }
   else
   {
      num_strokes = foo;
   }

   clear(); 
   realloc(num_strokes);

   // Must read in the specified number of strokes in the pool
   for (int i = 0; i < num_strokes; i++) 
   {
      OutlineStroke* x = 0; 
      read_stroke(is, x);
      if (x) add(x);
   }
   _num_strokes_used=_num;
   return is;
}

ostream&
BStrokePool::write_stream(ostream &os) const 
{
//   cerr << "BStrokePool::write_stream()" << endl;

   STDdstream d(&os);

   assert(_prototypes.num()>0);
   assert(_prototypes[0]);
   (_prototypes[0])->format(d);

   os << -1 ;
   os << endl;
   os << _prototypes.num(); 

   if (_prototypes.num() > 1)
   {
      for (int i = 1; i < _prototypes.num(); i++) 
      {
         _prototypes[i]->format(d);
      }
   }
   else
   {
      os << endl;
   }
   
   if (_write_strokes)
   {
      os << _num_strokes_used << endl; 

      for (int i = 0; i < _num_strokes_used; i++) 
      {
         _array[i]->format(d);
      }
   }
   else
   {
      os << 0 << endl; 
   }
   
   return os;
}

// Returns closest stroke in pool (if any) within
// pixel distance 'thresh' of pixel location 'p'

OutlineStroke* 
BStrokePool::pick_stroke(
   CNDCpt& p, 
   double& dist, 
   double thresh) 
{
   double ndc_thresh = VIEW::pix_to_ndc_scale() * thresh;

   double         min_dist    = DBL_MAX;
   OutlineStroke* min_stroke  = 0;

   for (int i=0; i<_num_strokes_used; i++) 
   {
      NDCpt nearest;
      double nearest_dist;

      _array[i]->closest(p, nearest, nearest_dist);

      if ((nearest_dist < min_dist) && (nearest_dist < ndc_thresh)) 
      {
         min_dist = nearest_dist;
         min_stroke = _array[i];
      }
   }

   if (min_stroke) 
   {
      dist = min_dist;
      return min_stroke;
   }
   else
   {
      return 0;
   }
}

