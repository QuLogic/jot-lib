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

/* ------------------- Hash table variables ----------------------------- */

class   HASHobs;
class   hashdist;
typedef ARRAYptrs<HASHobs *> HASHobs_list;
class HASHobs {
   static HASHobs_list  *_hash_obs_list;
   HASHobs_list *hash_list()     { if (!_hash_obs_list) 
                                      _hash_obs_list = new HASHobs_list;
                                   return _hash_obs_list; }
   public:
   virtual ~HASHobs() {}
   static  void notify_hash_obs(CGELptr &o, hashdist *h)
                                 { if (!_hash_obs_list) return;
                                   for (int i=0;i<_hash_obs_list->num();i++) 
                                     (*_hash_obs_list)[i]->notify_hash(o,h); }
   virtual void notify_hash(CGELptr &old, hashdist *h) = 0;
   
           void hash_obs()       { hash_list()->add_uniquely(this); }
           void unobs_hash()     { hash_list()->rem         (this); }
};

class hashdist : public DATA_ITEM, public GELdistobs,
                 public EXISTobs,   public DUPobs {
   protected:
      str_ptr   _str;  // name of the hash variable
      const int _dist; // whether this hash variable should be distributed
      HASH      _dhash;// hash table of objects that have non-default value
      const int _dup;  // ... should be copied on object duplication
      TAGlist   _tags;
              void *find(CGELptr &g)  const   { return _dhash.find((long) &*g);}
              void  add (CGELptr &g, void *v) { _dhash.add((long) &*g, v); }
      virtual void  del_item(void *) = 0;
              void  del_hash_items()          { ARRAY<long  > keys;
                                                ARRAY<void *> items;
                                                _dhash.get_items(keys, items);
                                                for (int i=0; i<keys.num();i++){
                                                   _dhash.del(keys[i]);
                                                   del_item(items[i]);
                                                }
                                              }
   public:
      virtual        ~hashdist()   { }
                      hashdist(Cstr_ptr &str, int dist, int dup=1):_str(str),
                                       _dist(dist), _dhash(16), _dup(dup) {
                         if (_dist)   // if hash is distributed, we watch
                            distrib_obs(); // when any GEL is distributed
                         DATA_ITEM::add_decoder(_str, this);
                         exist_obs(); // deleted objects must be uhashed
                         if (_dup)    // If 1-to-1 mapping, we don't want to
                            dup_obs();// copy information when object is dup'ed
                         _tags += new TAG_meth<hashdist>("",
                               &hashdist::put_dummy, &hashdist::get_var, 1);
                       }
             void      del(CGELptr &g)              { void *ret = 0;
                                                      if ((ret=find(g))) {
                                                         _dhash.del((long) &*g);
                                                         del_item(ret); }
                                                    }
              int       is_default(CGELptr &g)const { return find(g)==0; }
              CHASH    &hash()                const { return _dhash; }
      virtual CTAGlist &tags()                const { return _tags; }
      virtual DATA_ITEM *dup()                const { return  0; }
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
                 hashvar(Cstr_ptr &var, T val, int dist=0, int dup=1):
                      hashdist(var,dist,dup), _val(val) {}
   virtual      ~hashvar()         { del_hash_items();} // Can't do in ~hashdist

           T     get(CGELptr &g) const { return find(g) ? 
                                            ((TDI<T> *)find(g))->get() : _val; }
           void  set(GELptr g, T v)     { del(g);
                                          add(g, (void *)new TDI<T>(v));
                                          if (_dist)
                                          HASHobs::notify_hash_obs(g,this); }
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
                hashptr(Cstr_ptr &var, const T v, int dist=0,int dup=1):
                      hashdist(var,dist,dup), _val(v) { if (v) v->Own(); }
               ~hashptr()          { del_hash_items();} // Can't do in ~hashdist
           T    get(CGELptr &g)    { return find(g) ? (T)find(g) : _val;}
           void set(GELptr g, T v) { if (v) v->Own(); 
                                            del(g); add(g, (void *)v);
                                            if (_dist)
                                            HASHobs::notify_hash_obs(g,this); }

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
                hashenum(Cstr_ptr &var, T val, int dist=0, int dup=0):
                         hashdist(var,dist,dup), _val(val) {}
               ~hashenum()         { del_hash_items();} // Can't do in ~hashdist
           T    get(CGELptr &g) const { return (T)(find(g) ? 
                                          ((TDI<int> *)find(g))->get() : _val);}
           void set(GELptr g, T v)    { del(g);
                                        add(g, (void *) new TDI<int>((int)v));
                                        if (_dist)
                                            HASHobs::notify_hash_obs(g,this); }
   
    // 2005 VS generates compiler warnings for this funcion 		   
   //virtual str_ptr enum_to_str(T e) const { char b[20];sprintf(b,"%d",(int)e); 
   //                                         return b; }
   virtual T    str_to_enum(Cstr_ptr &s) const { return (T)atoi(**s); }
   virtual void put_var(TAGformat &d, CGELptr &gel) const {
                                        if (class_name() && find(gel)) {
                                           d.id() << gel->name() << " " << get(gel);
                                           d.end_id(); } 
                                       }
   virtual void get_var(TAGformat &d)  {  GELptr g; str_ptr val;
                                         *d >> g >> val; 
                                         if (g) set(g, str_to_enum(val)); }

   virtual void notify_dup(CGELptr &old, CGELptr &newg)
                                       { if (find(old)) set(newg, get(old)); }
};


class GrabVar : public hashvar<int> {
  public :
   GrabVar() : hashvar<int>("GRABBED", 0, 0) { }
           void grab      (CGELptr &g) { set(g, get(g)+1); GRABobs::notify_grab_obs(g,1); }
           void release   (CGELptr &g) { set(g, get(g)-1); GRABobs::notify_grab_obs(g,0); }
   virtual void notify_dup(CGELptr &, CGELptr &) { }
};


#define MAKE_NET_HASHVAR(NAME, TYPE, VAL) hashvar<TYPE>  NAME(#NAME, VAL, 1)
#define MAKE_NET_HASHENUM(NAME,TYPE, VAL) hashenum<TYPE> NAME(#NAME, VAL, 1)
#endif
