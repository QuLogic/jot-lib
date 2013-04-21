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
#include "std/config.H"
#include "data_item.H"
#include "mlib/points.H"
#include "ctype.h"

using mlib::Wpt;
using mlib::Wvec;

HASH *DATA_ITEM::_hash = 0;
DATA_ITEM* (*DATA_ITEM::_decode_unknown)(STDdstream&, Cstr_ptr&, DATA_ITEM*) = 0;

const str_ptr NAME_INT    ("int");
const str_ptr NAME_DOUBLE ("double");
const str_ptr NAME_VEC3D  ("vec3d");
const str_ptr NAME_COLOR  ("color");
const str_ptr NAME_POINT3D("point3d");
const str_ptr NAME_STR_PTR("str_ptr");

static int im=DATA_ITEM::add_decoder(NAME(0),     new TDI<int>(0), 1);
static int dm=DATA_ITEM::add_decoder(NAME(0.0),   new TDI<double>(0), 1);
static int vm=DATA_ITEM::add_decoder(NAME(Wvec()),new TDI<Wvec>(Wvec(0,1,0)), 1);
static int pm=DATA_ITEM::add_decoder(NAME(Wpt()), new TDI<Wpt>(Wpt()), 1);
static int sm=DATA_ITEM::add_decoder(NAME(str_ptr()),new TDI<str_ptr>(str_ptr()), 1);

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
   Cstr_ptr   &name, 
   DATA_ITEM  *di,
   int         copy
   )
{
   if (!_hash) 
      _hash = new HASH(128);
   _hash->add(**name, (void *) di);
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
   str_ptr str; 
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
   } else if (str) {
      if (_decode_unknown)
         di = _decode_unknown(d, str, 0);

      if (!di) {
         char *x = **str; char _buf[128], *buf = _buf;
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
      COMMENT(Cstr_ptr &name = str_ptr("//")) : _name(name) {}
      virtual ~COMMENT() {}

      STDdstream &format(CDATA_ITEM *, STDdstream &d) { return d; }
      STDdstream &decode(CDATA_ITEM *, STDdstream &d) {
         _delim.set_stream(&d);
         _delim.read_id();
         if (d.ascii()) {
            const int size = 1024;
            char name[size];
            d.istr()->getline(name, 1024);
         } else {
            str_ptr str;
            d >> str;
         }
         return d;
      }
      virtual Cstr_ptr &name() const { return _name; }
   protected:
      Cstr_ptr  _name;
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
                                                     // if object only has
   if (tags().num() == 1 && tags()[0]->name() == "") // 1 unnamed tag, then
      tags()[0]->decode(this, *d);                   // just call its decoder
   else
      while (d) {                  // if stream hasn't hit an end delimiter
         str_ptr  tag_name;
         *d >> tag_name;                         // read tag name,
         int j;
         for (j = 0; j < tags().num(); j++)  // then find tag reader
            if (tags()[j]->name() == tag_name) { // corresponding to tag.
               tags()[j]->decode(this, *d);      // invoke tag reader.
               break;
            }
         if (j == tags().num()) {
            if (comment.name() == tag_name) {
               comment.decode(this, *d);
            } else { // skip over tag's data section
               int count = 0, finished = 0;
               while (!finished) {
                  str_ptr s;
                  *d >> s;
                  if (!count && s[0] != '{')  // tag is single-valued
                     break;
                  // skip over matching { }'s
                  for (int x=0; x < (int)s->len(); x++) {
                     if (s[x] == '{')   count++;
                     if (s[x] == '}') { count--; if (count == 0) finished=1; }
                  }
               }
               cerr << "DATA_ITEM::decode - unrecognized tag '" << tag_name
                    << "' while decoding class " << class_name() << endl;
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
   for (int i=0; i<tags().num(); i++)
      tags()[i]->format(this, *d);  // write name value pair
   ds.write_newline();              // add carriage return
   d.end_id();                      // write end_delimiter

   return *d;            
}
