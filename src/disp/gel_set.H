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
/***************************************************************************
    gel_set.H
    
    GELset
    * A GEL container for rapid display/undisplay mechanisms 

    -------------------
    Pascal Barla
    Fall 2004
 ***************************************************************************/
#ifndef _GEL_SET_H_
#define _GEL_SET_H_

#include "disp/gel.H"

MAKE_PTR_SUBC(GELset,GEL);
typedef const GELset CGELset;
typedef const GELsetptr CGELsetptr;
class GELset : public GEL {
 public:
  // constructor/destructor
  GELset(){}
  ~GELset() {}

  // accessors
  int           num () const                  { return _gel_list.num(); } 
  void          operator += (const GELptr el) { _gel_list += el;}
  const GELptr  operator [](int i) const      { return _gel_list[i]; }
  void          pop()                         { if (!_gel_list.empty()) _gel_list.pop(); }
  void          clear()                       { _gel_list.clear(); }
  
  // display
  virtual RAYhit &intersect  (RAYhit  &r,mlib::CWtransf&m=mlib::Identity,
			      int uv=0)const {return _gel_list.intersect(r, m);}
  virtual bool cull (const VIEW *v) const {return _gel_list.cull(v);}

  virtual int  draw (CVIEWptr &v)       {return _gel_list.draw(v);}
  virtual int  draw_final(CVIEWptr & v) {return _gel_list.draw_final(v);} 
  virtual int  draw_id_ref()            {return _gel_list.draw_id_ref();}
  virtual int  draw_id_ref_pre1()       {return _gel_list.draw_id_ref_pre1();}
  virtual int  draw_id_ref_pre2()       {return _gel_list.draw_id_ref_pre2();}
  virtual int  draw_id_ref_pre3()       {return _gel_list.draw_id_ref_pre3();}
  virtual int  draw_id_ref_pre4()       {return _gel_list.draw_id_ref_pre4();}
  virtual int  draw_color_ref(int i)   {return _gel_list.draw_color_ref(i);}

  virtual int  draw_vis_ref()           {return _gel_list.draw_vis_ref();}
  virtual BBOX bbox(int i=0) const;

  virtual DATA_ITEM *dup() const { return 0; }
private:
  GELlist _gel_list;
};

#endif // _GEL_SET_H_
