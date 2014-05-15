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
#include "pack.H"
#include "stream.H"
static const int DSTREAM_READ_AHEAD_FACTOR = 4;

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
   if (istr()) {
      std::istream::sentry s(*istr(), false);
      if (s)
         brace = istr()->rdbuf()->sgetc();
      else
         brace = EOF;
   } else {
      char c;
      read(&c, sizeof(char), 0);
      brace = c;
   }
   return brace != '}';
}

/* -------------------------------------------------------------------------
 * DESCR   :	Writes byte data to the output buffer.  If the stream is
 * 		set for non-blocking i/o, flushes are performed
 * 		periodically, but not necessarily after each @write@. 
 * ------------------------------------------------------------------------- */
void
STDdstream::write (
   const char *const data,
   int               count
   )
{
   _out_queue.sputn(data, count);

   // Don't flush here - message size won't be prepended. - lsh
   if (_block)
      flush();
      
   /* Since flushes aren't guaranteed anyway, we don't care what happened */
   if (!_block)
      _fail = false;
}


/* -------------------------------------------------------------------------
 * DESCR   :	Reads data from the input buffer.  When the buffer is
 * 		depleted, the stream makes a receive request for the
 * 		appropriate number of bytes.  If there is a short count on
 * 		non-blocking i/o, the short bytes are left in the buffer
 * 		and the fail flag is set.
 * ------------------------------------------------------------------------- */
void
STDdstream::read (
   char  *data,
   int    count,
   int    pop    // flag for removing the read characters from input buffer
   )
{
   char *buffer = nullptr;
   size_t recv_request, recv_result;

   while ((int)_in_queue.in_avail() < count) {
      if (!buffer) {
         recv_request = count * DSTREAM_READ_AHEAD_FACTOR;
         buffer = new char [recv_request];
      }

      recv_result = recv(buffer, recv_request);
      
      if (recv_result)
         _in_queue.sputn(buffer, recv_result);

      if (!_block)
         break;
   }
   if (buffer)
      delete[] buffer;

   if ((int)_in_queue.in_avail() >= count) {
      if (pop)
           _in_queue.sgetn(data, count);
      else memcpy(data, _in_queue.str().data(), count);

      _fail = false;
   }
   else {
      cerr << "Failed to read message" << endl;
      _fail = true;
   }
}


/* -------------------------------------------------------------------------
 * DESCR   :	Attempts to flush data from the output buffer using the
 * 		@send@ method.  If there is a short count on the @send@
 * 		operation and the non-blocking flag is set, the remaining
 * 	        bytes are left in the buffer and the fail flag is set.
 * ------------------------------------------------------------------------- */
void
STDdstream::flush (void)
{
   char *buffer = nullptr;
   size_t send_request, send_result;

   while (_out_queue.in_avail() > 0) {
      if (!buffer) {
         send_request = _out_queue.in_avail();
         buffer = new char [send_request];
      }

      memcpy(buffer, _out_queue.str().data(), send_request);
      send_result = send(buffer, send_request);
      
      if (send_result)
         _out_queue.sgetn(buffer, send_result);

      if (!_block)
         break;
   }
   if (buffer)
      delete[] buffer;

   _fail = (_out_queue.in_avail() > 0);
}


/* --------------------  Public Multi-Methods   ---------------------------- */

void
STDdstream::read_close_delim()
{
   if (istr()) {
       char brace;
       (*this) >> brace;
   } else { 
       read_delim();
   }
}

void
STDdstream::read_open_delim()
{
   if (istr()) {
       char brace;
       (*this) >> brace;
   } else { 
       read_delim();
   }
}

void
STDdstream::write_delim(char c)
{
   if (ostr()) 
      (*this) << c;
   else
      write(&c, sizeof(char));
}

char
STDdstream::read_delim()
{
   char delim;
   read(&delim, sizeof(char));
   return delim;
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


static char  packbuf_space[sizeof(double)];
static char *packbuf;
static int   packcount;  /* dummy */

#define INIT_PACK         (packcount = 0, packbuf = packbuf_space)
#define DONE_PACK         ds.write_delim(' '); ds.write(packbuf_space, packcount);

#define INIT_UNPACK       packbuf = packbuf_space;      \
                          ds.read_delim(); ds.read(packbuf, sizeof(data))

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
   int len;
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
