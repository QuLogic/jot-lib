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
#ifndef SIL_STROKE_POOL_HEADER
#define SIL_STROKE_POOL_HEADER

#include "stroke/b_stroke_pool.H"
#include "npr/zxedge_stroke_texture.H"

#define CSilStrokePool const SilStrokePool

class SilStrokePool : public BStrokePool {
 private:
   /******** STATIC MEMBER VARIABLES ********/   
   static TAGlist*      _ssp_tags;

 protected:
   /******** MEMBER VARIABLES ********/

   bool                 _coher_global;
   bool                 _coher_sigma_one;
   int                  _coher_fit_type;
   int                  _coher_cover_type;
   float                _coher_pix;
   float                _coher_wf;
   float                _coher_ws;
   float                _coher_wb;
   float                _coher_wh;
   int                  _coher_mv;
   int                  _coher_mp;
   int                  _coher_m5;
   int                  _coher_hj;
   int                  _coher_ht;

   uint                 _path_index_stamp;     
   uint                 _group_index_stamp;    
   uint                 _selected_stroke_path_index;
   uint                 _selected_stroke_group_index;
   uint                 _selected_stroke_path_index_stamp;     
   uint                 _selected_stroke_group_index_stamp;    


 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
   SilStrokePool(OutlineStroke* proto = 0);

   /******** MEMBER METHODS *******/
   virtual void         draw_flat(CVIEWptr &);

   /******** ACCESSORS *******/

   bool                 get_coher_global()            { return _coher_global;    }
   void                 set_coher_global(bool g)      { _coher_global = g;       }
   bool                 get_coher_sigma_one()         { return _coher_sigma_one; }
   void                 set_coher_sigma_one(bool s)   { _coher_sigma_one = s;    }
   int                  get_coher_fit_type()          { return _coher_fit_type;  }
   void                 set_coher_fit_type(int f)     { _coher_fit_type = f;     }
   int                  get_coher_cover_type()        { return _coher_cover_type;}
   void                 set_coher_cover_type(int c)   { _coher_cover_type = c;   }
   float                get_coher_pix()               { return _coher_pix;       }
   void                 set_coher_pix(float p)        { _coher_pix = p;          }
   float                get_coher_wf()                { return _coher_wf;        }
   void                 set_coher_wf(float f)         { _coher_wf = f;           }
   float                get_coher_ws()                { return _coher_ws;        }
   void                 set_coher_ws(float s)         { _coher_ws = s;           }
   float                get_coher_wb()                { return _coher_wb;        }
   void                 set_coher_wb(float b)         { _coher_wb = b;           }
   float                get_coher_wh()                { return _coher_wh;        }
   void                 set_coher_wh(float h)         { _coher_wh = h;           }
   int                  get_coher_mv()                { return _coher_mv;        }
   void                 set_coher_mv(int v)           { _coher_mv = v;           }
   int                  get_coher_mp()                { return _coher_mp;        }
   void                 set_coher_mp(int p)           { _coher_mp = p;           }
   int                  get_coher_m5()                { return _coher_m5;        }
   void                 set_coher_m5(int p)           { _coher_m5 = p;           }
   int                  get_coher_hj()                { return _coher_hj;        }
   void                 set_coher_hj(int j)           { _coher_hj = j;           }
   int                  get_coher_ht()                { return _coher_ht;        }
   void                 set_coher_ht(int t)           { _coher_ht = t;           }

   void                 set_path_index_stamp(uint i)  { assert(!num_strokes()); _path_index_stamp = i;   }
   uint                 get_path_index_stamp() const  { return _path_index_stamp;                        }
   void                 set_group_index_stamp(uint i) { assert(!num_strokes()); _group_index_stamp = i;  }
   uint                 get_group_index_stamp() const { return _group_index_stamp;                       }

   bool                 update_selection(const LuboPathList&  pl)
   {
      if (_selected_stroke == -1)
      {
         return false;
      }
      else
      {
         assert(_selected_stroke < _num);
         assert(_array[_selected_stroke]->get_is_highlighted());
         assert(_array[_selected_stroke]->get_overdraw());

         assert(_selected_stroke_group_index_stamp == _selected_stroke_path_index_stamp);

         assert(_path_index_stamp == _group_index_stamp);

         if (_group_index_stamp == _selected_stroke_group_index_stamp)
         {
            assert(_selected_stroke < _num_strokes_used);
            assert(_selected_stroke_path_index == _array[_selected_stroke]->get_path_index());
            assert(_selected_stroke_group_index == _array[_selected_stroke]->get_group_index());
            
            return false;
         }
         else
         {
            int pi,si;
            if (pl.strokepath_id_to_indices(_selected_stroke_group_index, &pi, &si))
            {
               assert(_num_strokes_used > 0);

               uint id = pl[pi]->groups()[si].id();
               
               int ind = -1;
               for (int i=0; i<_num_strokes_used; i++)
               {
                  if (_array[i]->get_group_index() == id)
                  {
                     assert((_array[i]->get_path_index() == (uint)pi) && (ind == -1));
                     ind = i;
                  }
               }
               assert(ind != -1);

               internal_deselect();
               internal_select(ind);
            }
            else
            {
               internal_deselect();
            }
            return true;
         }
      }

   }

   // this will get a new, cleared stroke, ready to add points to
   virtual OutlineStroke*  get_stroke() 
   { 
      OutlineStroke* s = internal_stroke_at(_num_strokes_used++); 
      s->clear(); 
      return s; 
   }

 protected:   
   virtual void         internal_deselect()
   {
      if (_selected_stroke > -1)
      {
         assert(_array[_selected_stroke]->get_is_highlighted());
         assert(_array[_selected_stroke]->get_overdraw());
         _array[_selected_stroke]->highlight(false);
         _array[_selected_stroke]->set_overdraw(false);
      }
      _selected_stroke_path_index = 0;
      _selected_stroke_group_index = 0;
      _selected_stroke_path_index_stamp = 0;     
      _selected_stroke_group_index_stamp = 0;    
      _selected_stroke = -1;
   }
   virtual void         internal_select(int i)
   {
      _selected_stroke = i;
      if (_selected_stroke > -1)
      {
         assert(_path_index_stamp == _group_index_stamp);
         _selected_stroke_path_index = _array[_selected_stroke]->get_path_index();
         _selected_stroke_group_index = _array[_selected_stroke]->get_group_index();
         _selected_stroke_path_index_stamp = _path_index_stamp;     
         _selected_stroke_group_index_stamp = _group_index_stamp;    
         _array[_selected_stroke]->highlight(true);
         _array[_selected_stroke]->set_overdraw(true);
      }
      else
      {
         _selected_stroke_path_index = 0;
         _selected_stroke_group_index = 0;
         _selected_stroke_path_index_stamp = 0;     
         _selected_stroke_group_index_stamp = 0;    
      }
   }
 public:
   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS2("SilStrokePool", BStrokePool, CDATA_ITEM *);
   virtual DATA_ITEM*   dup() const  { return new SilStrokePool(); }
   virtual CTAGlist&    tags() const;

   /******** INTERNAL MEMBER METHODS ********/
 protected:
   //used by set_prototype() to enforce that _prototype[0]
   //governs the offset period, and orig mesh size, etc.
    //subclass pools may implement this... (i.e. sils!)
   virtual int          set_prototype_internal(OutlineStroke* p);

   /******** I/O Methods ********/
   //virtual void         get_texture_file (TAGformat &d);
   //virtual void         put_texture_file (TAGformat &d) const;

   /******** I/O Access Methods ********/
   bool&                coher_global_()      { return _coher_global;    }
   bool&                coher_sigma_one_()   { return _coher_sigma_one; }
   int&                 coher_fit_type_()    { return _coher_fit_type;  }
   int&                 coher_cover_type_()  { return _coher_cover_type;}
   float&               coher_pix_()         { return _coher_pix;       }
   float&               coher_wf_()          { return _coher_wf;        }
   float&               coher_ws_()          { return _coher_ws;        }
   float&               coher_wb_()          { return _coher_wb;        }      
   float&               coher_wh_()          { return _coher_wh;        }
   int&                 coher_mv_()          { return _coher_mv;        }
   int&                 coher_mp_()          { return _coher_mp;        }
   int&                 coher_m5_()          { return _coher_m5;        }
   int&                 coher_hj_()          { return _coher_hj;        }
   int&                 coher_ht_()          { return _coher_ht;        }
};


#endif // SIL_STROKE_POOL_HEADER

/* end of file sil_stroke_pool.H */
