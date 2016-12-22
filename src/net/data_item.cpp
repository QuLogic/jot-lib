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
#include "std/config.hpp"
#include "data_item.hpp"

using mlib::Wpt;
using mlib::Wvec;

map<string,DATA_ITEM*> *DATA_ITEM::_hash = nullptr;
DATA_ITEM* (*DATA_ITEM::_decode_unknown)(STDdstream&, const string&, DATA_ITEM*) = nullptr;

const string NAME_INT    ("int");
const string NAME_DOUBLE ("double");
const string NAME_VEC3D  ("vec3d");
const string NAME_COLOR  ("color");
const string NAME_POINT3D("point3d");
const string NAME_STRING ("string");

static int im=DATA_ITEM::add_decoder(NAME(0),     new TDI<int>(0), 1);
static int dm=DATA_ITEM::add_decoder(NAME(0.0),   new TDI<double>(0), 1);
static int vm=DATA_ITEM::add_decoder(NAME(Wvec()),new TDI<Wvec>(Wvec(0,1,0)), 1);
static int pm=DATA_ITEM::add_decoder(NAME(Wpt()), new TDI<Wpt>(Wpt()), 1);
static int sm=DATA_ITEM::add_decoder(NAME(string()),new TDI<string>(string()), 1);

// for some odd reason, we can't inline this on the Suns because
// it generates link errors when we create subclasses of DATA_ITEM...
// the odd thing is it doesn't seem to effect subclasses of DATA_ITEM
// that *are* able to define static_name inline...
//
STAT_STR_RET
DATA_ITEM::static_name()
{ 
   RET_STAT_STR("DATA_ITEM"); 
}

DATA_ITEM::~DATA_ITEM() 
{
}

int
DATA_ITEM::add_decoder(
   const string &name,
   DATA_ITEM    *di,
   int           copy
   )
{
   if (!_hash) 
      _hash = new map<string,DATA_ITEM*>();
   (*_hash)[name] = di;
   if (copy != -1)
      di->_copy = copy;
   return 1;
}
 

DATA_ITEM  *
DATA_ITEM::Decode(
   STDdstream &d,
   int         DelayDecoding
   ) 
{
   // read the keyword or class name
   string str;
   d >> str;

   if (Config::get_var_bool("DEBUG_DATA_ITEM",false))
      cerr << "Decoding: " << str << endl;

   DATA_ITEM* di = lookup(str);
   if (di) {
      if (di->class_name() != str) {
         // DATA_ITEM only decodes classes
         // app may be able to decode objects
         // if app can't, then it's an error
         di = _decode_unknown(d, str, di);
         if (!di) {
            cerr << "DATA_ITEM::Decode - failure, class '"
	         << di->class_name() << "' can't read '" << str << "'\n";
         }
      } else {
         if (di->_copy)
            di = di->dup();
         if (!DelayDecoding) 
            di->decode(d);
      }
   } else if (!str.empty()) {
      if (_decode_unknown)
         di = _decode_unknown(d, str, nullptr);

      if (!di) {
         const char *x = str.c_str(); char _buf[128], *buf = _buf;
         while (*x && isupper(*x)) 
            *buf++ = *x++;  
         *buf++ = '\0';  

         cerr << "DATA_ITEM::Decode - unknown object " << str <<endl;

      }
   }

   return di;
}

class COMMENT : public TAG {
   public:
      COMMENT(const string &name = "//") : _name(name) {}
      virtual ~COMMENT() {}

      STDdstream &format(CDATA_ITEM *, STDdstream &d) { return d; }
      STDdstream &decode(CDATA_ITEM *, STDdstream &d) {
         _delim.set_stream(&d);
         _delim.read_id();
         const int size = 1024;
         char name[size];
         d.istr()->getline(name, 1024);
         return d;
      }
      virtual const string &name() const { return _name; }
   protected:
      const string _name;
      TAGformat _delim;
};

/* -----------------------------------------------------
    This decodes a DATA_ITEM record.  The format of all
  DATA_ITEMS is either:
      {
          name  value
          ...
          name  value
      }

   Or:

     {
        value
     }
   ----------------------------------------------------- */
STDdstream &
DATA_ITEM::decode(STDdstream &ds)
{
   TAGformat d(&ds, class_name(), 1);
   static COMMENT comment;
   
   d.read_id();                                 // read the start delimiter

   if (tags().size() == 1 && tags()[0]->name() == "") {
      // if object only has 1 unnamed tag, then just call its decoder
      tags()[0]->decode(this, *d);
   } else {
      while (d) {
         string tag_name;
         *d >> tag_name;
         TAGlist::size_type j;
         for (j = 0; j < tags().size(); j++) {
            if (tags()[j]->name() == tag_name) {
               tags()[j]->decode(this, *d);
               break;
            }
         }
         if (j == tags().size()) {
            if (comment.name() == tag_name) {
               comment.decode(this, *d);
            } else { // skip over tag's data section
               int count = 0, finished = 0;
               while (!finished) {
                  string s;
                  *d >> s;
                  if (!count && s[0] != '{')  // tag is single-valued
                     break;
                  // skip over matching { }'s
                  for (auto & elem : s) {
                     if (elem == '{')   count++;
                     if (elem == '}') { count--; if (count == 0) finished=1; }
                  }
               }
               cerr << "DATA_ITEM::decode - unrecognized tag '" << tag_name
                    << "' while decoding class " << class_name() << endl;
            }
         }
      }
   }

   d.read_end_id();                             // read the end delimiter

   recompute();                                 // update object from file data

   return *d;
}

STDdstream  &
DATA_ITEM::format(STDdstream &ds) const 
{
   TAGformat d(&ds, class_name(), 1);

   if (Config::get_var_bool("DEBUG_DATA_ITEM",false))
      cerr << "Formatting " << class_name() << endl;

   d.id();                          // write OBJECT_NAME start_delimiter
   for (auto & tag : tags())
      tag->format(this, *d);  // write name value pair
   ds.write_newline();              // add carriage return
   d.end_id();                      // write end_delimiter

   return *d;            
}

STDdstream &
operator >> (STDdstream &ds, map<string,DATA_ITEM*> &h)
{
   h.clear();

   int num;
   ds >> num;

   for (int i = 0; i < num; i++) {
      string key;
      ds >> key;

      DATA_ITEM *di = DATA_ITEM::Decode(ds);

      h[key] = di;
   }

   return ds;
}

STDdstream &
operator << (STDdstream &ds, const map<string,DATA_ITEM*> &h)
{
   ds << h.size();

   map<string,DATA_ITEM*>::const_iterator it;
   for (it = h.begin(); it != h.end(); ++it) {
      ds << it->first << it->second;
   }

   return ds;
}

