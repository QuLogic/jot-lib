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
#ifndef __GEL_FILTER_H__
#define __GEL_FILTER_H__

#include "gel.H"

/*****************************************************************
 * Virtual base class for accepting or rejecting a GEL based
 * on some condition.
 *****************************************************************/
class GELFILT {
 public:
   //******** MANAGERS ********
   virtual ~GELFILT() {}

   //******** RUN-TIME TYPE ID ********
   // XXX -
   //   rarely handled in GELFILT derived types
   virtual STAT_STR_RET class_name () const { return static_name();}
   static  STAT_STR_RET static_name();

   //******** VIRTUAL METHOD ********
   // Returns true if the GEL is accepted
   virtual bool accept(CGELptr &gel) = 0; 
};

/*****************************************************************
 *  This rejects any GEL found in a specified list of GELs
 *****************************************************************/
class GELFILTothers : public GELFILT {
 public:
   //******** MANAGERS ********
   GELFILTothers(CGELlist &gels) : _gels(gels) {}
   virtual ~GELFILTothers() {}

   //******** VIRTUAL METHOD ********
   virtual bool accept(CGELptr &gel) { return !_gels.contains(gel); }

 protected:
   CGELlist _gels;      // GELS to be rejected
};

/*****************************************************************
 * Accepts GELs from a given list of class names.
 *
 * NOTE:
 *    The GEL is only accepted if its exact class name
 *    appears in the list. To accept a GEL that is DERIVED
 *    from one of the named classes, use GELFILTclass_desc,
 *    below.
 *****************************************************************/
class GELFILTclass_name : public GELFILT {
 public:
   //******** MANAGERS ********
   GELFILTclass_name(Cstr_ptr  &cn  ) { _cn.add(cn); }
   GELFILTclass_name(Cstr_list &list) : _cn(list) { }

   void add(Cstr_ptr &cn)            { _cn.add(cn); }

   //******** VIRTUAL METHOD ********
   virtual bool  accept(CGELptr &g) { return _cn.contains(g->class_name()); }

 protected:
   str_list _cn;        // acceptable class names
};

/*****************************************************************
 * Similar to GELFIILTclass_name, but also accepts a GEL that
 * is DERIVED from any class named in the list.
 *****************************************************************/
class GELFILTclass_desc : public GELFILTclass_name {
 public:
   //******** MANAGERS ********
   GELFILTclass_desc(Cstr_ptr& cn)    : GELFILTclass_name(cn)  {}
   GELFILTclass_desc(Cstr_list& list) : GELFILTclass_name(list){}

   //******** VIRTUAL METHOD ********
   virtual bool accept(CGELptr &g) {
      for (int i=0; i<_cn.num(); i++)
         if (g->is_of_type(_cn[i]))
            return 1;
      return 0;
   }
};

/*****************************************************************
 * Opposite of GELFILTclass:
 *   Rejects any GEL whose exact class name is in a given list.
 *****************************************************************/
class GELFILTclass_name_excl : public GELFILTclass_name {
 public:
   //******** MANAGERS ********
   GELFILTclass_name_excl(Cstr_ptr  &cn)   : GELFILTclass_name(cn)   {}
   GELFILTclass_name_excl(Cstr_list &list) : GELFILTclass_name(list) {}

   //******** VIRTUAL METHOD ********
   virtual bool accept(CGELptr &g) { return !GELFILTclass_name::accept(g); }
};

/*****************************************************************
 * Opposite of GELFILTclass_desc:
 *    Rejects any GEL that derives from a given set of classes.
 *****************************************************************/
class GELFILTclass_desc_excl : public GELFILTclass_desc {
 public:
   //******** MANAGERS ********
   GELFILTclass_desc_excl(Cstr_ptr  &cn  ) : GELFILTclass_desc(cn)  {}
   GELFILTclass_desc_excl(Cstr_list &list) : GELFILTclass_desc(list){}

   //******** VIRTUAL METHOD ********
   virtual bool accept(CGELptr &g) { return !GELFILTclass_desc::accept(g); }
};

/*****************************************************************
 *  This rejects GELS that aren't pickable
 *****************************************************************/
class GELFILTpickable : public GELFILT {
 public:
   //******** MANAGERS ********
   GELFILTpickable(bool pick_all) : _all(pick_all) {}

   //******** RUN-TIME TYPE ID ********
   virtual STAT_STR_RET class_name () const { return static_name();}
   static  STAT_STR_RET static_name()       { RET_STAT_STR("GELFILTpickable");}

   //******** VIRTUAL METHOD ********
   virtual bool accept(CGELptr &g) { return _all ? 1 : PICKABLE.get(g) != 0; }

 protected:
   bool _all;  // whether all objects should be pickable 
};

/*****************************************************************
 * This will reject any GEL that has field's value set to
 * the given value
 *****************************************************************/
template <class T>
class GELFILTtdi : public GELFILT {
 public:
   //******** MANAGERS ********
   GELFILTtdi(hashvar<T> &field, T val) : _field(field), _val(val) {}

   //******** VIRTUAL METHOD ********
   virtual bool accept(CGELptr &g) { return _field.get(g) != _val; }

 protected:
   hashvar<T> &_field;
   T           _val;
};

extern hashvar<int> WIDGET_3D;
/*****************************************************************
 * This rejects all GELs tagged as WIDGET_3Ds
 *****************************************************************/
class GELFILTwidget3d : public GELFILTtdi<int> {
 public:
   //******** MANAGERS ********
   GELFILTwidget3d() : GELFILTtdi<int>(WIDGET_3D, 1) {}
};

/*****************************************************************
 * A list of filters.  The GEL must be accepted by all filters
 *****************************************************************/
class GELFILTlist : public GELFILT, public ARRAY<GELFILT*> {
 public:
   //******** MANAGERS ********
   GELFILTlist(int num=16)           : ARRAY<GELFILT *>(num) {}
   GELFILTlist(const GELFILTlist &l) : ARRAY<GELFILT *>(l)   {}
   GELFILTlist(GELFILT *filt) { *this += filt;}

   //******** VIRTUAL METHOD ********
   virtual bool accept(CGELptr &gel) {
      for (int i=0; i<_num; i++)
         if (!_array[i]->accept(gel))
            return 0;
      return true;
   }
};

#endif // __GEL_FILTER_H__

/* end of file gel_filt.H */
