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
#ifndef JOT_DATA_ITEM_H_IS_INCLUDED
#define JOT_DATA_ITEM_H_IS_INCLUDED

#include "std/hash.H"
#include "net/stream.H"
#include "net/net_types.H"

#include "rtti.H"

#define DECODER_ADD(X) DATA_ITEM::add_decoder(X::static_name(), new X(), 1)

#define DECLARE_NETWORK_TAGS(CLASS_NAME) \
TAGlist *CLASS_NAME::_##CLASS_NAME##tags = 0; \
static int CLASS_NAME##st=DECODER_ADD(CLASS_NAME);

class COLOR;
class mlib::Wvec;
class mlib::Wpt;
extern Cstr_ptr  NAME_INT;
extern Cstr_ptr  NAME_DOUBLE;
extern Cstr_ptr  NAME_COLOR;
extern Cstr_ptr  NAME_VEC3D;
extern Cstr_ptr  NAME_POINT3D;
extern Cstr_ptr  NAME_STR_PTR;
inline STAT_STR_RET NAME(const str_ptr    &)  { return NAME_STR_PTR;}
inline STAT_STR_RET NAME(const int        &)  { return NAME_INT;    }
inline STAT_STR_RET NAME(const double     &)  { return NAME_DOUBLE; }
inline STAT_STR_RET NAME(const mlib::Wvec &)  { return NAME_VEC3D;  }
inline STAT_STR_RET NAME(const mlib::Wpt  &)  { return NAME_POINT3D;}
inline STAT_STR_RET NAME(const COLOR      &)  { return NAME_COLOR;}
template <class T>
inline STAT_STR_RET NAME(const T &x)       { return x.class_name(); }

#define CDATA_ITEM const DATA_ITEM
class DATA_ITEM;

/* -----------------------------------------------------------------------
    TAGformat's are used to print out the name of a TAG and any delimiters
    necessary to distinguish the boundary of the data for the TAG.
    In general, TAG's with multi-valued fields will have their data 
    wrapped in { }'s.   However, if there is no name associated with the 
    TAG, then it is assumed that there is only one TAG in the object being
    serialized, and thus no delimiters are necessary.
   ---------------------------------------------------------------------- */
class TAGformat {
  protected :
     str_ptr     _name;
     int         _multi;
     STDdstream *_ds;
  public:
         TAGformat(STDdstream *d, Cstr_ptr &n,int m):_name(n),_multi(m),_ds(d){}
         TAGformat(               Cstr_ptr &n,int m):_name(n),_multi(m),_ds(0){}
         TAGformat() : _multi(0), _ds(0) {}
     void        set_stream(STDdstream *d) { _ds = d; }
     STDdstream &operator*()               { return *_ds; }
     STDdstream &read_id()                 { if (_name != NULL_STR && _multi)
                                                _ds->read_open_delim();
                                             return *_ds; }
     STDdstream &read_end_id()             { if (_name != NULL_STR && _multi)
                                                _ds->read_close_delim();
                                             return *_ds; }
     STDdstream &id()                      { _ds->write_newline();
                                             if (_name != NULL_STR) {
                                                *_ds << _name; _ds->ws("\t");
                                                if (_multi)
                                                   _ds->write_open_delim();
                                             }
                                             return *_ds; }
     STDdstream &end_id()                  { if (_name != NULL_STR && _multi)
                                                _ds->write_close_delim();
                                             return *_ds; }
     operator    int()                     { return !_ds->eof()&&_ds->check(); }
     Cstr_ptr   &name()      const         { return _name; }
};


/* -----------------------------------------------------------------------
   TAGs are the base unit for formatting a piece of data.  A TAG has a 
   name and a method for formatting and decoding its data.
   ---------------------------------------------------------------------- */
class TAG {
   public:
     TAG() { }
     virtual ~TAG() {}
     virtual STDdstream &format(CDATA_ITEM *me, STDdstream &d) = 0;
     virtual STDdstream &decode(CDATA_ITEM *me, STDdstream &d) = 0;
     virtual Cstr_ptr   &name() const = 0;
};
typedef ARRAY<TAG *> TAGlist;
#define CTAGlist const TAGlist

/* -----------------------------------------------------------------------
   TAGmeths are TAGs that serialize the tag data by calling a pair of 
   methods on a DATA_ITEM derived object.    The formatting method 
   should call the id() function before writing any data in order to
   write out the TAG id and any delimiters.  Then after writing out all
   its data, the formatting method should call the end_id() function.
   The reason these functions are called explicitly by the formatting 
   function is to allow the formatting function not to write out *any*
   data.   The decoding function just needs to read its data, as the
   TAG name and any delimiters will be handled by TAGmeth.
   ---------------------------------------------------------------------- */
template <class T>
class TAG_meth : public TAG {
     typedef void (T::*infunc )(TAGformat &d);
     typedef void (T::*outfunc)(TAGformat &d) const;
     TAGformat _delim;
     outfunc   _format;
     infunc    _decode;
  public:
    TAG_meth() { }
    TAG_meth(Cstr_ptr &field_name, outfunc format, infunc decode, int multi=0): 
                                        _delim(field_name, multi),
                                        _format(format), _decode(decode) { }
    virtual ~TAG_meth() {}

    STDdstream &format(CDATA_ITEM *me, STDdstream &d) { _delim.set_stream(&d);
                                       (((T *)me)->*_format)(_delim); return d;}
    STDdstream &decode(CDATA_ITEM *me, STDdstream &d) 
                                      { _delim.set_stream(&d);
                                       _delim.read_id();
                                       (((T *)me)->*_decode)(_delim);
                                       _delim.read_end_id();
                                       return d; }
    Cstr_ptr   &name()     const      { return _delim.name(); }
};

template <class T, class V>
class TAG_val : public TAG {
   typedef V &(T::*value)();
   typedef bool (T::*testval)() const;
   TAGformat _delim;
   value     _value;
   testval   _test;
 public:
   TAG_val() { }
   TAG_val(Cstr_ptr &field_name, value val, testval test=0)
      : _delim(field_name, 0), _value(val), _test(test) { }
   virtual ~TAG_val() {}
   STDdstream &format(CDATA_ITEM *me, STDdstream &d) {
      bool output;
      // Need to use != 0 in order to work around VC++ problem
      if (_test == 0) output = true;
      else {
         output = (((T *)me)->*_test)();
      }
      if (output) {
         _delim.set_stream(&d);
         _delim.id() << (((T *)me)->*_value)();
      }
      return d;
   }
   STDdstream &decode(CDATA_ITEM *me, STDdstream &d)
      { _delim.set_stream(&d);
      _delim.read_id()>>(((T *)me)->*_value)();
      return d; }
   Cstr_ptr   &name()     const      { return _delim.name(); }
};

#if WIN32_VCPLUSPLUS_DIDNT_HAVE_PROBLEMS
// If Visual C++ didn't have problems with the following, we could use them to
// avoid lots of extra typing

template <class T, class V>
TAG *TAGval(char *str, V &(T::*value)()) { return new TAG_val<T,V>(str, value);}

// multi should default to 0, but that isn't allowed in AIX CC
template <class T>
TAG *TAGmeth(char *str, 
             void (T::*ofunc)(TAGformat &) const,
             void (T::*ifunc)(TAGformat &), 
             int multi) 
     { return new TAG_meth<T>(str, ofunc, ifunc, multi); }
#endif


/* ----------------------------------------------------------- * 
 *   DATA_ITEM - (marshalls objects onto and off of STDdstreams)
 *      virtual base class for all data stored in a hash table's
 *   that need to be written to and read from a STDdstream.  
 *   DATA_ITEM subclasses must fill in methods for :
 *     1) decoding themselves from a STDdstream
 *     2) formatting themselves onto a STDdstream as :
 *                OBJECT_NAME { 
 *                    field_name   value 
 *                    ... 
 *                    field_name   value
 *                }
 *     3) providing a unique string for a class name
 *     4) duplicating themselves
 * ----------------------------------------------------------- */
class DATA_ITEM {
   private:
    static HASH *_hash;
    static DATA_ITEM *(*_decode_unknown)(STDdstream &, Cstr_ptr &, DATA_ITEM *);
   protected:
      int          _copy;
      TAGlist      _DEFINERtags;

   public:
          DATA_ITEM(int copy = 0) :_copy(copy) {}
 virtual ~DATA_ITEM();

 virtual CTAGlist    &tags()                 const { return _DEFINERtags; }
 virtual void         add_tags()             const { }
 virtual STDdstream  &format(STDdstream &d)  const;
 virtual STDdstream  &decode(STDdstream &d);
 virtual void         recompute()            { }

     /* -------- prototype functions overloaded by subclasses -------- */
 virtual STAT_STR_RET class_name ()          const = 0;
 virtual DATA_ITEM   *dup        ()          const = 0; 
 static  STAT_STR_RET static_name();
 virtual int          is_of_type(Cstr_ptr &t)const { return IS(t); }

     /* -------- Class functions -------- */
 static  int         add_decoder(Cstr_ptr   &d, DATA_ITEM *di, int copy= -1);
 static  void        set_default_decoder(DATA_ITEM *(*d)(STDdstream&, Cstr_ptr&,
                                                         DATA_ITEM *))
                                                 { _decode_unknown = d; }
 static  DATA_ITEM  *Decode     (STDdstream &d, int DelayDecoding = 0);
 static  DATA_ITEM  *lookup     (Cstr_ptr   &d) { return !_hash ? 0 : 
                                                (DATA_ITEM *) _hash->find(**d);}
     /* -------- Static debugging functions -------- */
 static  HASH       *di_hash    ()              { return _hash; }
};

inline STDdstream &operator<<(STDdstream &s, CDATA_ITEM &d)
         { return d.format(s); }
inline STDdstream &operator>>(STDdstream &s, DATA_ITEM *&d)
         { d = d->Decode(s); return s; }

/* ----------------------------------------------------------- * 
 *  TDI -  (wraps conventional objects in a DATA_ITEM format)
 *     is a templated DATA_ITEM that provides a DATA_ITEM wrapper
 *   around objects that support :
 *    1) the NAME() operation
 *    2) the << and >> operators for STDdstreams.
 *    3) a copy constructor
 * ----------------------------------------------------------- */
template <class T>
class TDI : public DATA_ITEM {
  protected:
        T   _x;
  TAGlist   _tags;
         void        put_val(TAGformat &d) const { d.id() << _x; d.end_id(); }
         void        get_val(TAGformat &d)       { *d >> _x; }
  public:
                     TDI(const T &x): _x(x)
     { _tags+=new TAG_meth<TDI<T> >(NULL_STR, &TDI<T>::put_val, &TDI<T>::get_val, 1);}
                 T   &get()                  { return _x; }
         const   T   &get()            const { return _x; }
 virtual DATA_ITEM   *dup()            const { return new TDI<T>(_x); }
 virtual CTAGlist    &tags()           const { return _tags; }
 virtual STAT_STR_RET class_name()     const { return NAME(_x); }
};

/* ----------------------------------------------------------- * 
 *  FUNC_ITEM - (optimizes dup() and class_name() for function objects)
 *     is a base class for function objects.  There should only
 *  be one instance per "function class" so the dup() operation
 *  is optimized.  Alternatively, since only one instance will 
 *  be made for any class, we can store the class name in an
 *  instance variable instead of making a static method.
 * ----------------------------------------------------------- */
class FUNC_ITEM : public DATA_ITEM {
 public:
   FUNC_ITEM(Cstr_ptr &n): _name(n) { 
      if (!lookup(n)) 
         add_decoder(n,this);
      _tags += new TAG_meth<FUNC_ITEM>(
         str_ptr(), &FUNC_ITEM::put, &FUNC_ITEM::get, 1
         );
   }

   CTAGlist&            tags()            const { return _tags; }
   virtual void         put(TAGformat &)  const = 0;
   virtual void         get(TAGformat &)        = 0;
   virtual DATA_ITEM*   dup()             const { return (DATA_ITEM *) this;}
   virtual STAT_STR_RET class_name()      const { return _name; }

 protected:
   str_ptr _name; // class name
   TAGlist _tags;
};

#endif // JOT_DATA_ITEM_H_IS_INCLUDED

// end of file data_item.H

