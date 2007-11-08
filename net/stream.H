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
#ifndef STD_STREAM_HAS_BEEN_INCLUDED
#define STD_STREAM_HAS_BEEN_INCLUDED

#include "std/support.H"
#include "mem_push.H"

/* --------------------------    Constants     ----------------------------- */

/* --------------------------      Types       ----------------------------- */

/*
 * TITLE   :    Data Stream Class
 * DESCR   :	This class provides safe data transfer for objects and
 * 		non-object data with a syntax similar to that of the
 * 		standard C++ iostreams.  The simple non-object data types
 * 		have pack and unpack methods defined for them with the
 * 		appropriate conversions built in to avoid byte-order
 * 		problems and ensure architecture independence.  Objects
 * 		that define their own pack and unpack methods should use
 * 		these primitive routines.
 */
class STDdstream {
 public:
   STDdstream();

   // New policy: 5/2004:
   //   The STDdstream either contains an iostream, 
   //   or it contains just an istream or just an ostream.
   STDdstream(iostream* s);     // has both
   STDdstream( istream* i);     // has just istream
   STDdstream( ostream* o);     // has just ostream

   virtual ~STDdstream ()  {}

   void    write(const char * const data, int count);
   void    read (     UGAptr data, int count, int pop = 1);
   void    flush();

   iostream* iostr() const { return _iostream; }
   istream*  istr()  const { return _iostream ? (istream*)_iostream : _istream; }
   ostream*  ostr()  const { return _iostream ? (ostream*)_iostream : _ostream; }

   int     ascii() const { return istr() || ostr(); }

   char    peekahead();
   void    read_open_delim();
   void    read_close_delim();

   void    write_open_delim()         { write_delim('{');if(ascii()) _indent++;}
   void    write_close_delim()        { write_delim('}');if(ascii()) _indent--;}
   void    write_newline()            { if (ascii()) {ws("\n");
                                        for (int i=0;i<_indent;i++) ws("\t"); }}
   void    write_delim(char);
   char    read_delim();

   int     check() { char brace = peekahead(); return brace == '}' ? 0 : 1; }
   int     eof() const { return istr() ? istr()->eof(): !_in_queue.count(); }

   void    ws(char *);
   STDbool operator ! ()        const { return _fail; }
   STDbool fail       ()        const { return _fail; }
   STDbool block      ()        const { return _block; }
   STDbool block(STDbool flag)        { STDbool old=_block; _block = flag; return old; }
   // Get a string with spaces in it - the same as STDdstream >> str_ptr when
   // using a binary stream, but different when doing ascii
   str_ptr get_string_with_spaces();

 protected:

   virtual size_t send (const UGAptr data, int count) { _in_queue.put(data,count); return count; }
   virtual size_t recv (UGAptr data, int count)       { return _out_queue.get (data, count); }

   STDmem_queue _in_queue;
   STDmem_queue _out_queue;
   iostream*    _iostream;      // iostream, may be null
   istream*     _istream;       // just an in stream (used when iostream is null)
   ostream*     _ostream;       // just an out stream (used when iostream is null)
   int          _indent;

 private:
   STDbool      _fail;
   STDbool      _block;

 //So _fail can get set by ascii IO...
   friend STDdstream & operator >> (STDdstream &, char &);
   friend STDdstream & operator >> (STDdstream &, char *&);
   friend STDdstream & operator >> (STDdstream &, str_ptr&);
   friend STDdstream & operator >> (STDdstream &, string&);
   friend STDdstream & operator >> (STDdstream &, short &);
   friend STDdstream & operator >> (STDdstream &, int &);
   friend STDdstream & operator >> (STDdstream &, long &);
   friend STDdstream & operator >> (STDdstream &, unsigned char &);
   friend STDdstream & operator >> (STDdstream &, unsigned short &);
   friend STDdstream & operator >> (STDdstream &, unsigned int &);
   friend STDdstream & operator >> (STDdstream &, unsigned long &);
   friend STDdstream & operator >> (STDdstream &, float &);
   friend STDdstream & operator >> (STDdstream &, double &); 

   friend STDdstream & operator << (STDdstream &, char);
   friend STDdstream & operator << (STDdstream &, const char * const);
   friend STDdstream & operator << (STDdstream &, str_ptr);
   friend STDdstream & operator << (STDdstream &, const string&);
   friend STDdstream & operator << (STDdstream &, short);
   friend STDdstream & operator << (STDdstream &, int);
   friend STDdstream & operator << (STDdstream &, long);
   friend STDdstream & operator << (STDdstream &, unsigned char);
   friend STDdstream & operator << (STDdstream &, unsigned short);
   friend STDdstream & operator << (STDdstream &, unsigned int);
   friend STDdstream & operator << (STDdstream &, unsigned long);
   friend STDdstream & operator << (STDdstream &, float);
   friend STDdstream & operator << (STDdstream &, double); 
};

/* --------------------------      Methods     ----------------------------- */

inline STDdstream & flush (STDdstream &ds)  { ds.flush(); return (ds); }

STDdstream & operator >> (STDdstream &, char &);
STDdstream & operator >> (STDdstream &, char *&);
STDdstream & operator >> (STDdstream &, str_ptr&);
STDdstream & operator >> (STDdstream &, short &);
STDdstream & operator >> (STDdstream &, int &);
STDdstream & operator >> (STDdstream &, long &);
STDdstream & operator >> (STDdstream &, unsigned char &);
STDdstream & operator >> (STDdstream &, unsigned short &);
STDdstream & operator >> (STDdstream &, unsigned int &);
STDdstream & operator >> (STDdstream &, unsigned long &);
STDdstream & operator >> (STDdstream &, float &);
STDdstream & operator >> (STDdstream &, double &); 

STDdstream & operator << (STDdstream &, char);
STDdstream & operator << (STDdstream &, const char * const);
STDdstream & operator << (STDdstream &, str_ptr);
STDdstream & operator << (STDdstream &, short);
STDdstream & operator << (STDdstream &, int);
STDdstream & operator << (STDdstream &, long);
STDdstream & operator << (STDdstream &, unsigned char);
STDdstream & operator << (STDdstream &, unsigned short);
STDdstream & operator << (STDdstream &, unsigned int);
STDdstream & operator << (STDdstream &, unsigned long);
STDdstream & operator << (STDdstream &, float);
STDdstream & operator << (STDdstream &, double); 

/* --------------------------      Macros      ----------------------------- */


#endif  /* STD_STREAM_HAS_BEEN_INCLUDED */
