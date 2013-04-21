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
#ifndef JOT_VARS_IN_DA_HOUSE
#define JOT_VARS_IN_DA_HOUSE

#include "std/support.H"
#include "net/data_item.H"

/* ---------------------- JOTvar classes ------------------------------ */
//
// this is a generic class for observing all JOTvars.  
// Observers are notified with the DATA_ITEM corresponding to the
// JOTvar that changed.  Observers are not expected to look at the
// values in the JOTvar, but instead are only expected to call 
// DATA_ITEM methods such as format or decode.
//
class JOTvar_obs;
typedef ARRAYptrs<JOTvar_obs *> JOTvar_obs_list;
class JOTvar_obs {
   static JOTvar_obs_list  *_jot_var_obs_list;
   JOTvar_obs_list *jot_var_obs_list(){ if (!_jot_var_obs_list) 
                                          _jot_var_obs_list=new JOTvar_obs_list;
                                         return _jot_var_obs_list; }
 public:
   virtual ~JOTvar_obs() {}
   static  void notify_jot_var_obs(DATA_ITEM *item)
                                 { if (!_jot_var_obs_list) return;
                                   for (int i=0;i<_jot_var_obs_list->num();i++) 
                                       (*_jot_var_obs_list)[i]->
                                             notify_jot_var(item); }
   virtual void notify_jot_var(DATA_ITEM *item) = 0;
   
           void jot_var_obs()     { jot_var_obs_list()->add_uniquely(this); }
           void unobs_jot_var()   { jot_var_obs_list()->rem         (this); }
};


//
//  Special TAG for JOTvars.  This TAG is different from a normal TAG_val
// because the TAG stores the object where the value is stored and ignores
// the object passed to format/decode.  This is because the value in a JOTvar
// is logically associated with the containing class, but is actually stored
// in the "invisible" JOTvar class.  Therefore, the object that is passed to
// format/decode is the containing class and isn't useful in helping find the
// actual variable data.  
//
template <class T, class V>
class TAG_var : public TAG {
     typedef V    (T::*value)() const;
     typedef void (T::*set_meth)(V t);

     set_meth  _smeth;
     value     _value;
     TAGformat _delim;
     T        *_var;
  public:
                TAG_var( ) : _var(0),_value(0) { }
                TAG_var(T *var, Cstr_ptr &field_name, value val, set_meth m): 
                    _var(var), _delim(field_name, 0), _value(val), _smeth(m) { }
    STDdstream &format(CDATA_ITEM *, STDdstream &d) 
                                      { _delim.set_stream(&d);
                                        _delim.id() << (_var->*_value)();
                                        return d; }
    STDdstream &decode(CDATA_ITEM *, STDdstream &d)
                                      {  V val;
                                        _delim.set_stream(&d); 
                                        _delim.read_id() >> val;
                                        (_var->*_smeth)(val);
                                        return d; }
    Cstr_ptr   &name()     const      { return _delim.name(); }
};

//
// Generic JOTvar that enables a class to easily add a member variable
// that will automatically be networked when it is set and that will 
// provide TAGs for reading and writing the variable to a file.
//
template <class OBJ, class TYPE>
class JOTvar : public DATA_ITEM {
      typedef void (OBJ::*set_meth)(TYPE t);
   protected:
      TAGlist   *_var_tags;   // tags for reading/writing the variables
      str_ptr    _vname;      // name of the variable
      str_ptr    _inst_name;  // variable instance name:  object::variable

      set_meth   _smeth;      // containing class method used to set variable
      OBJ       *_obj;        // containing instance

      TYPE       _val;        // variable value

      void       set_val(TYPE val)        { (_obj->*_smeth)(val); }
      str_ptr    inst_name()              { return _obj->name() + "::"+_vname; }
      virtual void      check_inst_name() { if (inst_name() != _inst_name) {
                                              // New instantiation name
                                              _inst_name = inst_name();
                                              DATA_ITEM::add_decoder(_inst_name,
                                                                     this);
                                            }
                                          }

   public:
                     // constructor: each JOTvar instance is a unique decoder!
                     JOTvar(TYPE v, OBJ *obj, Cstr_ptr &vname, set_meth m):
                          _vname(vname), _val(v), _obj(obj), _smeth(m), 
                          _var_tags(0)
                                          { _inst_name = inst_name(); 
                                       DATA_ITEM::add_decoder(_inst_name,this);}

         void         set(TYPE v)         { _val = v;// set val & notify net,etc
                                            check_inst_name();
                                          JOTvar_obs::notify_jot_var_obs(this);}
         TYPE         get()         const { return _val; }

 virtual Cstr_ptr    &obj_name()    const { return _obj->name();} 
 virtual Cstr_ptr    &var_name()    const { return _vname;} 
 virtual STAT_STR_RET class_name()  const { return _inst_name;}// instance name!
 virtual DATA_ITEM   *dup()         const { return  0; }
 virtual CTAGlist    &tags()        const {
                           if (!_var_tags) {
                              ((JOTvar<OBJ,TYPE> *)this)->_var_tags = new TAGlist;
                              *((JOTvar<OBJ,TYPE> *)this)->_var_tags += 
                                 new TAG_var<JOTvar<OBJ,TYPE>,TYPE>((JOTvar<OBJ,TYPE> *)this,_vname,
                                                           get,set_val);
                           }
                           return *_var_tags;
                        }
};


/*

MAKE_PTR_SUBC(Rake,GEOM);
class Rake : public GEOM {
    JOTvar<Rake,int>  _num_streams;
    JOTvar<Rake,int>  _num_rakes;
    TAGlist          *_tg_tags; // can't be static unless JOTvar tags are redone

    CTAGlist  &tags()   const   {
                            if (!_tg_tags) {
                               ((Rake  *)this)->_tg_tags  = new TAGlist;
                               *((Rake *)this)->_tg_tags += GEOM::tags();
                               *((Rake *)this)->_tg_tags += _num_rakes.tags();
                               *((Rake *)this)->_tg_tags += _num_streams.tags();
                            }
                            return *_tg_tags; 
                         }

   public:

    Rake():_tg_tags(0), _num_rakes  (0, this, "NumRakes",   set_num_rakes),
                        _num_streams(0, this, "NumStreams", set_num_streams) { }

    Rake(Cstr_ptr &n):GEOM(n),_tg_tags(0),
                        _num_rakes  (0, this, "NumRakes",   set_num_rakes),
                        _num_streams(0, this, "NumStreams", set_num_streams) { }

    int      num_rakes()            { return _num_rakes.get(); }
    void     set_num_rakes(int n)   { _num_rakes.set(n); }

    int      num_streams()          { return _num_streams.get(); }
    void     set_num_streams(int n) { _num_streams.set(n); }

    DEFINE_RTTI_METHODS2("Rake", GEOM, CDATA_ITEM *);

   virtual DATA_ITEM *dup()            const { return new Rake(_name); }
   virtual GEOMptr    dup(Cstr_ptr &n) const { cerr <<"Whaaa\n"; return 0;}
};
*/

#endif
