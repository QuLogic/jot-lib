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

#ifndef HASH_TYPES_H_INCLUDED
#define HASH_TYPES_H_INCLUDED

#include <map>
#include <set>

/* ------------------- Hash table variables ----------------------------- */

class hashdist : public DATA_ITEM, public GELdistobs,
                 public EXISTobs,   public DUPobs {
   protected:
      string    _str;  // name of the hash variable
      const int _dist; // whether this hash variable should be distributed
      map<CGELptr,void *> _dhash;// hash table of objects that have non-default value
      const int _dup;  // ... should be copied on object duplication
      TAGlist   _tags;
              void *find(CGELptr &g)  const   { map<CGELptr,void *>::const_iterator it;
                                                it = _dhash.find(g);
                                                if (it != _dhash.end())
                                                   return it->second;
                                                else
                                                   return nullptr;
                                              }
              void  add (CGELptr &g, void *v) { _dhash[g] = v; }
      virtual void  del_item(void *) = 0;
              void  del_hash_items()          { map<CGELptr,void *>::iterator it;
                                                for (it = _dhash.begin(); it != _dhash.end(); ++it)
                                                   del_item(it->second);
                                                _dhash.clear();
                                              }
   public:
      virtual        ~hashdist()   { }
                      hashdist(const string &str, int dist, int dup=1):_str(str),
                                       _dist(dist), _dup(dup) {
                         if (_dist)   // if hash is distributed, we watch
                            distrib_obs(); // when any GEL is distributed
                         DATA_ITEM::add_decoder(_str, this);
                         exist_obs(); // deleted objects must be uhashed
                         if (_dup)    // If 1-to-1 mapping, we don't want to
                            dup_obs();// copy information when object is dup'ed
                         _tags.push_back(new TAG_meth<hashdist>("",
                               &hashdist::put_dummy, &hashdist::get_var, 1));
                       }
             void      del(CGELptr &g)              { map<CGELptr,void *>::iterator it;
                                                      it = _dhash.find(g);
                                                      if (it != _dhash.end()) {
                                                         del_item(it->second);
                                                         _dhash.erase(it);
                                                      }
                                                    }
              int       is_default(CGELptr &g)const { return find(g)==nullptr; }
      const map<CGELptr,void*> &hash()        const { return _dhash; }
      virtual CTAGlist &tags()                const { return _tags; }
      virtual DATA_ITEM *dup()                const { return  nullptr; }
      virtual STAT_STR_RET class_name ()      const { return _str; }
      virtual void     put_dummy(TAGformat &) const { }  // unused
      virtual void     get_var  (TAGformat &)     = 0;
      virtual void     notify_exist(CGELptr &g, int f) { if (!f) del(g); }
      virtual void     notify_dup  (CGELptr &old,CGELptr &newg) = 0;
      virtual void     put_var(TAGformat &d,  CGELptr &g) const=0;
      virtual void     notify_distrib(STDdstream &ds,CGELptr&g) {
                               TAGformat d(&ds,class_name(),1); put_var(d,g);}
};

template <class T>
class hashvar : public hashdist {
   T       _val;                 // default value of hash variable
   virtual void del_item(void * item) { delete (TDI<T> *) item;}
  public :
                 hashvar(const string &var, T val, int dist=0, int dup=1):
                      hashdist(var,dist,dup), _val(val) {}
   virtual      ~hashvar()         { del_hash_items();} // Can't do in ~hashdist

           T     get(CGELptr &g) const { return find(g) ? 
                                            ((TDI<T> *)find(g))->get() : _val; }
           void  set(GELptr g, T v)     { del(g);
                                          add(g, (void *)new TDI<T>(v)); }
   virtual void notify_dup(CGELptr &old, CGELptr &newg)
                                          { if (find(old)) set(newg, get(old));}
   virtual void put_var(TAGformat &d, CGELptr &g) const {
                                            if (_dist && find(g)) {
                                              d.id() << g->name() << get(g); 
                                              d.end_id();} }
   virtual void get_var(TAGformat &d)     { GELptr  g;
                                            T       inval;
                                            *d >> g >> inval; 
                                            if (g) set(g, inval); }
};

// (AIX compiler seems to create a macro called Free(p), so this is necessary)
#ifdef Free
#undef Free
#endif
template <class T>
class hashptr : public hashdist {
   T       _val;                 // default value of hash variable
   virtual void del_item(void *item) { ((T) item)->Free(); }
  public :
                hashptr(const string &var, const T v, int dist=0,int dup=1):
                      hashdist(var,dist,dup), _val(v) { if (v) v->Own(); }
               ~hashptr()          { del_hash_items();} // Can't do in ~hashdist
           T    get(CGELptr &g)    { return find(g) ? (T)find(g) : _val;}
           void set(GELptr g, T v) { if (v) v->Own(); 
                                            del(g); add(g, (void *)v); }

                         // Don't know about serializing of pointers
   virtual void put_var     (TAGformat &, CGELptr&) const { }
   virtual void get_var     (TAGformat &)                 { }
   virtual void notify_exist(CGELptr &g, int f)           { if (!f&&g) del(g);}
   virtual void notify_dup  (CGELptr &old, CGELptr &newg) { if (find(old)) 
                                                           set(newg, get(old));}
};

template <class T>
class hashenum : public hashdist {
   T       _val;                 // default value of hash variable
   virtual void del_item(void *item) { delete ((TDI<int> *) item); }
  public :
                hashenum(const string &var, T val, int dist=0, int dup=0):
                         hashdist(var,dist,dup), _val(val) {}
               ~hashenum()         { del_hash_items();} // Can't do in ~hashdist
           T    get(CGELptr &g) const { return (T)(find(g) ? 
                                          ((TDI<int> *)find(g))->get() : _val);}
           void set(GELptr g, T v)    { del(g);
                                        add(g, (void *) new TDI<int>((int)v)); }
   
    // 2005 VS generates compiler warnings for this funcion 		   
   //virtual string enum_to_str(T e) const { char b[20];sprintf(b,"%d",(int)e);
   //                                         return b; }
   virtual T    str_to_enum(const string &s) const { return (T)atoi(s.c_str()); }
   virtual void put_var(TAGformat &d, CGELptr &gel) const {
                                        if (!class_name().empty() && find(gel)) {
                                           d.id() << gel->name() << " " << get(gel);
                                           d.end_id(); } 
                                       }
   virtual void get_var(TAGformat &d)  {  GELptr g; string val;
                                         *d >> g >> val; 
                                         if (g) set(g, str_to_enum(val)); }

   virtual void notify_dup(CGELptr &old, CGELptr &newg)
                                       { if (find(old)) set(newg, get(old)); }
};


class GrabVar : public hashvar<int> {
  public :
   GrabVar() : hashvar<int>("GRABBED", 0, 0) { }
           void grab      (CGELptr &g) { set(g, get(g)+1); }
           void release   (CGELptr &g) { set(g, get(g)-1); }
   virtual void notify_dup(CGELptr &, CGELptr &) { }
};


#define MAKE_NET_HASHVAR(NAME, TYPE, VAL) hashvar<TYPE>  NAME(#NAME, VAL, 1)
#define MAKE_NET_HASHENUM(NAME,TYPE, VAL) hashenum<TYPE> NAME(#NAME, VAL, 1)
#endif
