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

#ifndef B_STROKE_POOL_HEADER
#define B_STROKE_POOL_HEADER
#include "outline_stroke.H"

   // Dynamically allocated strokes are retained in the pool for
   // possible reuse from frame to frame. Thus, _num_strokes_used may be
   // less than _num, the total number of strokes being stored in the pool.

#define BSTROKEPOOL_SET_PROTOTYPE__NEEDS_FLUSH              1
#define BSTROKEPOOL_SET_PROTOTYPE__NEEDED_ADJUSTMENT        2

#define CBStrokePool const BStrokePool

class BStrokePool : protected ARRAY<OutlineStroke*>, public DATA_ITEM {
 private:
   /******** STATIC MEMBER VARIABLES ********/
   static TAGlist          *_bsp_tags;

 protected:
   /******** MEMBER VARIABLES ********/
   ARRAY<OutlineStroke*>  _prototypes;

   int         _edit_proto;
   int         _draw_proto;
   bool        _lock_proto;

   int         _num_strokes_used;  // Number of strokes actually in use.
   bool        _hide;
   int         _selected_stroke;
   bool        _write_strokes;

   double      _cur_mesh_size;
   double      _cur_period;
 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
   BStrokePool(OutlineStroke* proto = 0);
   virtual ~BStrokePool();

   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS_BASE("BStrokePool", CBStrokePool*);
   //DEFINE_RTTI_METHODS2("BStrokePool", BStrokePool, CBStrokePool*);
   virtual DATA_ITEM       *dup() const { return new BStrokePool; }
   virtual CTAGlist        &tags() const;
   //Subclass this and have it set _complete
   virtual STDdstream      &decode(STDdstream &ds);

   /******** I/O functions ********/

   virtual void            get_base_prototype (TAGformat &d);
   virtual void            put_base_prototype (TAGformat &d) const;

   virtual void            get_prototype (TAGformat &d);
   virtual void            put_prototypes (TAGformat &d) const;

   virtual void            get_stroke (TAGformat &d);
   virtual void            put_strokes (TAGformat &d) const;

   /******** MEMBER METHODS ********/
   //***These manage _draw_proto/_edit_proto issues

   virtual void            set_edit_proto(int i);
   int                     get_edit_proto() const     { return _edit_proto; }

   virtual void            set_draw_proto(int i);
   int                     get_draw_proto() const     { return _draw_proto; }

   virtual void            set_lock_proto(bool e);
   bool                    get_lock_proto() const     { return _lock_proto; }

   int                     get_num_protos() const     { return _prototypes.num(); }

 protected:
   inline int              get_active_proto() const   { return (_lock_proto)?(_edit_proto):(_draw_proto); }

   virtual void            apply_active_proto();

 public:
   //***These operate using _edit_proto***

   // call this to set the default stroke for the pool to dup from
   virtual int             set_prototype(OutlineStroke* p);

 protected:
   //used by set_prototype() to enforce that _prototype[0]
   //governs the offset period, and orig mesh size, etc.
    //subclass pools may implement this... (i.e. sils!)
   virtual int             set_prototype_internal(OutlineStroke* p);
 public:
   // returns the prototype. If you change it, and want the pool
   // to know this, call set_prototype with the updated stroke.
   OutlineStroke*          get_prototype() const;

   virtual void            add_prototype();
   virtual void            del_prototype();

   //***These operate using _active_proto***

   OutlineStroke*          get_active_prototype() const;

   // this will draw all of the strokes modified since the last reset
   // draws with z-buffering off
   virtual void            draw_flat(CVIEWptr &);

   //***These operate without direct reference to _draw_proto/_edit_proto

   ostream &write_stream(ostream &os) const;
   istream &read_stream(istream &is);        

   int                     num_strokes () const { return _num_strokes_used;}


   void                    mark_dirty();  // Flags the current group of strokes as 'dirty'. 
                                          // Will cause the strokes to be updated before drawing.

   void                    hide()            { _hide = true; }
   void                    show()            { _hide = false; }

   OutlineStroke*          pick_stroke(mlib::CNDCpt& pt, double& dist, double thresh = 5.0);    

   // this will *always* return a OutlineStroke*, resizing the
   // array if necessary. 
 protected:
   virtual OutlineStroke*  internal_stroke_at(int i);
   // this one wont
 public:   
   virtual OutlineStroke*  stroke_at(int i);

   int                     remove_stroke(OutlineStroke* s);
   
   // this will get a new, cleared stroke, ready to add points to
   virtual OutlineStroke*  get_stroke() 
   { 
      OutlineStroke* s = internal_stroke_at(_num_strokes_used++); 
      s->clear(); 
      return s; 
   }

   // call this when you're rebuilding strokes and
   // don't wanna trash the selection information
   // (e.g. for sils)
   void blank()
   {
      _num_strokes_used = 0;
   }

   // call this when you're done with the current group of strokes
   void reset()  
   { 
      internal_deselect();
      blank();
   }

   // deletes all the strokes in the memory pool
   void drain() 
   {
      reset();
      for (int i=0; i<_num; i++) delete _array[i];
      clear();
   }

 protected:   
   virtual void internal_deselect()
   {
      if (_selected_stroke > -1)
      {
         assert(_array[_selected_stroke]->get_is_highlighted());
         assert(_array[_selected_stroke]->get_overdraw());
         _array[_selected_stroke]->highlight(false);
         _array[_selected_stroke]->set_overdraw(false);
         //blank path/group indices and stamp
      }
      _selected_stroke = -1;
   }
   virtual void internal_select(int i)
   {
      _selected_stroke = i;
      if (_selected_stroke > -1)
      {
         //set path/group indices and stamp
         _array[_selected_stroke]->highlight(true);
         _array[_selected_stroke]->set_overdraw(true);
      }
   }

 public:
   //int set_selected_stroke(OutlineStroke* s)
   void set_selected_stroke(OutlineStroke* s)
   {
      int i;

      if (s)
      {
         i = get_index(s);
         assert((i != BAD_IND) && (i < _num_strokes_used));
      }
      else
      {
         i = -1;
      }

      internal_deselect();
      internal_select(i);
   }

   //OutlineStroke* select_stroke(int i) 
   void set_selected_stroke_index(int i) 
   {
      assert((i >= -1) && (i < _num_strokes_used));

      internal_deselect();
      internal_select(i);
   }

   //int selected_stroke_index() 
   int get_selected_stroke_index() 
   {
      assert((_selected_stroke >= -1) && (_selected_stroke < _num_strokes_used));
      return _selected_stroke;
   }

   //OutlineStroke* selected_stroke() 
   OutlineStroke* get_selected_stroke() 
   {
      assert((_selected_stroke >= -1) && (_selected_stroke < _num_strokes_used));
      return (stroke_at(_selected_stroke));
   }

   OutlineStroke* cycle_selected_stroke() 
   {
      assert((_selected_stroke >= -1) && (_selected_stroke < _num_strokes_used));

      if (_num_strokes_used == 0) return NULL;
      
      int i = (_selected_stroke+1) % _num_strokes_used;

      internal_deselect();
      internal_select(i);
               
      return get_selected_stroke();
   }


};

#endif
