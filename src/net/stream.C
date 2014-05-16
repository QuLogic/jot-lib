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
/* Copyright 1992, Brown Computer Graphics Group.  All Rights Reserved. */

/* -------------------------------------------------------------------------
 *
 *                <     File description here    >
 *
 * ------------------------------------------------------------------------- */

#include "std/support.H"
#include "net.H"
#include "stream.H"

/* -----------------------  Private Methods  ------------------------------- */



/* -----------------------  Public Methods   ------------------------------- */
STDdstream::STDdstream():
   _iostream(nullptr),
   _istream(nullptr),
   _ostream(nullptr),
   _indent(0),
   _fail(false),
   _block(true)
{
}

STDdstream::STDdstream(iostream* s):
   _iostream(s),
   _istream(nullptr),
   _ostream(nullptr),
   _indent(0),
   _fail(false),
   _block(true)
{
}

STDdstream::STDdstream(istream* s):
   _iostream(nullptr),
   _istream(s),
   _ostream(nullptr),
   _indent(0),
   _fail(false),
   _block(true)
{
}

STDdstream::STDdstream(ostream* s):
   _iostream(nullptr),
   _istream(nullptr),
   _ostream(s),
   _indent(0),
   _fail(false),
   _block(true)
{
}

/* -------------------------------------------------------------------------
 * DESCR   :	Checks if the next input character is the end delimiter.
 * ------------------------------------------------------------------------- */
bool
STDdstream::check_end_delim()
{
   int brace;
   std::istream::sentry s(*istr(), false);
   if (s)
      brace = istr()->rdbuf()->sgetc();
   else
      brace = EOF;
   return brace != '}';
}

/* --------------------  Public Multi-Methods   ---------------------------- */

void
STDdstream::read_close_delim()
{
   char brace;
   (*this) >> brace;
}

void
STDdstream::read_open_delim()
{
   char brace;
   (*this) >> brace;
}

void
STDdstream::write_delim(char c)
{
   (*this) << c;
}

string
STDdstream::get_string_with_spaces()
{
   const int bufsize = 1024;
   char buf[bufsize];
   int  i = 0;
   char ch = ' ';
   bool done = 0;
   while (!done)
   {
      istr()->get(ch);
      // Done when we hit a curly or newline
      done = (ch == '}') || (ch == '{') || (ch == '\n');
      if (done)
      {
         // shift character back onto stream
         istr()->putback(ch);
         // Remove spaces at end
         int j;
         for (j = i-1; j >= 0 && (buf[j] == ' ' || buf[j]=='\t'); j--)
         {
            istr()->putback(buf[j]);
         }
         i = j + 1;
      }
      else
      {
         done = (i > bufsize-1) || !istr()->good();
         if (!done) buf[i++] = ch;
      }
   }
   buf[i] = '\0';
   // Find first character that isn't a space
   int start = 0;
   while (buf[start] == ' ' || buf[start] == '\t')
   {
      start++;
   }
   return string(buf + start);
}


/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for char *
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, char * &data)
{  
   *ds.istr() >> data;
   ds._fail = ds.istr()->fail();
   return ds;
}

STDdstream &
operator << (STDdstream &ds, const char * const data)
{
   *ds.ostr() << data;
   ds._fail = ds.ostr()->fail();
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for string
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, string &data)
{  
   const int buflen = 4096;
   char      buff[buflen];
   char     *usebuff = buff;

   *ds.istr() >> usebuff;
   ds._fail = ds.istr()->fail();
   data = string(usebuff);

   return ds;
}

STDdstream &
operator << (STDdstream &ds, const string &data)
{
   *ds.ostr() << data.c_str() << " ";
   ds._fail = ds.ostr()->fail();
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for short
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, short &data)
{  
   *ds.istr() >> data;
   ds._fail = ds.istr()->fail();
   return ds;
}
STDdstream &
operator << (STDdstream &ds, short data)
{
   *ds.ostr() << data << " ";
   ds._fail = ds.ostr()->fail();
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for int
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, int &data)
{  
   *ds.istr() >> data;
   ds._fail = ds.istr()->fail();
   return ds;
}
STDdstream &
operator << (STDdstream &ds, int data)
{
   *ds.ostr() << data << " ";
   ds._fail = ds.ostr()->fail();
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for long
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, long &data)
{  
   *ds.istr() >> data;
   ds._fail = ds.istr()->fail();
   return ds;
}
STDdstream &
operator << (STDdstream &ds, long data)
{
   *ds.ostr() << data << " ";
   ds._fail = ds.ostr()->fail();
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for unsigned short
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, unsigned short &data)
{  
   *ds.istr() >> data;
   ds._fail = ds.istr()->fail();
   return ds;
}
STDdstream &
operator << (STDdstream &ds, unsigned short data)
{
   *ds.ostr() << data << " ";
   ds._fail = ds.ostr()->fail();
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for unsigned int
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, unsigned int &data)
{  
   *ds.istr() >> data;
   ds._fail = ds.istr()->fail();
   return ds;
}
STDdstream &
operator << (STDdstream &ds, unsigned int data)
{
   *ds.ostr() << data << " ";
   ds._fail = ds.ostr()->fail();
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for unsigned long
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, unsigned long &data)
{  
   *ds.istr() >> data;
   ds._fail = ds.istr()->fail();
   return ds;
}
STDdstream &
operator << (STDdstream &ds, unsigned long data)
{
   *ds.ostr() << data << " ";
   ds._fail = ds.ostr()->fail();
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for float
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, float &temp)
{  
   *ds.istr() >> temp;
   ds._fail = ds.istr()->fail();
   return ds;
}
STDdstream &
operator << (STDdstream &ds, float data)
{
   *ds.ostr() << data << " ";
   ds._fail = ds.ostr()->fail();
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for double
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, double &data)
{  
   *ds.istr() >> data;
   ds._fail = ds.istr()->fail();
   return ds;
}
STDdstream &
operator << (STDdstream &ds, double data)
{
   *ds.ostr() << data << " ";
   ds._fail = ds.ostr()->fail();
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for char
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, char &data)
{  
   *ds.istr() >> data;
   ds._fail = ds.istr()->fail();
   return ds;
}
STDdstream &
operator << (STDdstream &ds, char data)
{
   *ds.ostr() << data << " ";
   ds._fail = ds.ostr()->fail();
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for unsigned char
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, unsigned char &data)
{  
   *ds.istr() >> data;
   ds._fail = ds.istr()->fail();
   return ds;
}
STDdstream &
operator << (STDdstream &ds, unsigned char data)
{
   *ds.ostr() << data << " ";
   ds._fail = ds.ostr()->fail();
   return ds;
}


void
STDdstream::ws(const char *x)
{
   (*this) << x;
}

// end of file stream.C
