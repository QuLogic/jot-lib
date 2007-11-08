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
   _iostream(0),
   _istream(0),
   _ostream(0),
   _indent(0),
   _fail(STD_FALSE),
   _block(STD_TRUE) 
{
}

STDdstream::STDdstream(iostream* s):
   _iostream(s),
   _istream(0),
   _ostream(0),
   _indent(0),
   _fail(STD_FALSE),
   _block(STD_TRUE) 
{
}

STDdstream::STDdstream(istream* s):
   _iostream(0),
   _istream(s),
   _ostream(0),
   _indent(0),
   _fail(STD_FALSE),
   _block(STD_TRUE) 
{
}

STDdstream::STDdstream(ostream* s):
   _iostream(0),
   _istream(0),
   _ostream(s),
   _indent(0),
   _fail(STD_FALSE),
   _block(STD_TRUE) 
{
}

/* -------------------------------------------------------------------------
 * DESCR   :	Peeks at the next input character without actually
 * 		removing it from the input queue.
 * ------------------------------------------------------------------------- */
char
STDdstream::peekahead()         
{ 
   char c;
   if (istr()) 
   {
      //This is sometimes used to entice an EOF
      //to detect the end of input streams.  In this
      //case, the ensuing _fail=STD_TRUE is undesirable...
      
      bool was_good = !fail();
      
      (*this) >> c;
      
      if (fail() && eof() && was_good)
      {
         _fail = STD_FALSE;
      }

      istr()->putback(c);
   } 
   else  
   {
      read(&c, sizeof(char), 0);
   }
   return c;
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
   _out_queue.put (data, count);

   // Don't flush here - message size won't be prepended. - lsh
   if (_block)
      flush();
      
   /* Since flushes aren't guaranteed anyway, we don't care what happened */
   if (!_block)
      _fail = STD_FALSE;
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
   UGAptr data,
   int    count,
   int    pop    // flag for removing the read characters from input buffer
   )
{
   UGAptr buffer = NULL;
   size_t recv_request, recv_result;

   while ((int)_in_queue.count () < count)
   {
      if (!buffer)
      {
         recv_request = count * DSTREAM_READ_AHEAD_FACTOR;
         buffer = (UGAptr)malloc(recv_request);
      }

      recv_result = recv (buffer, recv_request);
      
      if (recv_result)
         _in_queue.put (buffer, recv_result);

      if (!_block)
         break;
   }
   if (buffer)
      free(buffer); 

   if ((int)_in_queue.count () >= count)
   {
      if (pop)
           _in_queue.get (data, count);
      else _in_queue.peek(data, count);

      _fail = STD_FALSE;
   }
   else {
      cerr << "Failed to read message" << endl;
      _fail = STD_TRUE;
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
   UGAptr buffer = NULL;
   size_t send_request, send_result;

   while (_out_queue.count ())
   {
      if (!buffer)
      {
         send_request = _out_queue.count ();
         buffer = (UGAptr)malloc(send_request);
      }

      _out_queue.peek (buffer, send_request);
      send_result = send (buffer, send_request);
      
      if (send_result)
         _out_queue.get (buffer, send_result);

      if (!_block)
         break;
   }
   if (buffer)
      free(buffer);

   _fail = (_out_queue.count () > 0) ? STD_TRUE : STD_FALSE;
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

str_ptr
STDdstream::get_string_with_spaces()
{
   if (ascii()) 
   {
      const int bufsize = 1024;
      char buf[bufsize];
      int  i = 0;
      char ch = ' ';
      int done = 0; // bool is in mlib, so I won't use it
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
      return str_ptr(buf + start);
   }
   str_ptr the_string;
   (*this) >> the_string;
   return the_string;
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
   if (ds.ascii())
   {
      *ds.istr() >> data;
      ds._fail = ((ds.istr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      ds.read_delim();

      int len;
      ds >> len;
      ds.read(data, len * sizeof(char));
      data[len] = '\0';
   }
   return ds;
}

STDdstream &
operator << (STDdstream &ds, const char * const data)
{
   if (ds.ascii()) 
   {
      *ds.ostr() << data;
      ds._fail = ((ds.ostr()->fail())?(STD_TRUE):(STD_FALSE));
   } else {
      ds.write_delim(' ');   // write out the delimiter

      ds << strlen(data);
      ds.write(data, strlen(data) * sizeof(char));
   }
   return ds;
}
/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for str_ptr
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, str_ptr &data)
{  
   int len;
   const int buflen = 4096;
   char      buff[buflen];
   char     *usebuff = buff;

   if (ds.ascii()) 
   {
      *ds.istr() >> usebuff;
      ds._fail = ((ds.istr()->fail())?(STD_TRUE):(STD_FALSE));
      data = str_ptr(usebuff);
   } else {
      ds.read_delim();

      ds >> len;
      if (len + 1 > buflen) {
         usebuff = new char[len + 1];
      }
      ds.read(usebuff, len * sizeof(char));
      usebuff[len] = '\0';

      data = str_ptr(usebuff);
 
      if (len + 1 > buflen)
         delete [] usebuff;
   }

   return ds;
}

STDdstream &
operator << (STDdstream &ds, str_ptr data)
{
   if (ds.ascii())
   {
      *ds.ostr() << **data << " ";
      ds._fail = ((ds.ostr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      ds.write_delim(' '); // write out the delimiter 

      ds << strlen(**data);
      ds.write(**data, strlen(**data) * sizeof(char));
   }
   return ds;
}

STDdstream &
operator >> (STDdstream &ds, string& str)
{  
   // use str_ptr functionality for now
   str_ptr tmp;
   ds >> tmp;
   str = **tmp;
   return ds;
}

STDdstream &
operator << (STDdstream &ds, const string& str)
{
   ds << str.c_str();
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for short
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, short &data)
{  
   if (ds.ascii()) 
   {
       *ds.istr() >> data;
       ds._fail = ((ds.istr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      assert(0);
//      INIT_UNPACK;
//      UGA_UNPACK_WORD (data, packbuf, packcount, short);
   }
   return ds;
}
STDdstream &
operator << (STDdstream &ds, short data)
{
   if (ds.ascii()) 
   {
       *ds.ostr() << data << " ";
       ds._fail = ((ds.ostr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      assert(0);
//      INIT_PACK;
//      UGA_PACK_WORD (data, packbuf, packcount);
//      DONE_PACK;
   }
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for int
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, int &data)
{  
   if (ds.ascii()) 
   {
      *ds.istr() >> data;
      ds._fail = ((ds.istr()->fail())?(STD_TRUE):(STD_FALSE));
   } 
   else {
      INIT_UNPACK;
      UGA_UNPACK_WORD (data, packbuf, packcount, int);
   }
   return ds;
}
STDdstream &
operator << (STDdstream &ds, int data)
{
   if (ds.ascii()) 
   {
      *ds.ostr() << data << " ";
      ds._fail = ((ds.ostr()->fail())?(STD_TRUE):(STD_FALSE));
   } 
   else {
      assert(0);
//      INIT_PACK;
//      UGA_PACK_WORD (data, packbuf, packcount);
//      DONE_PACK;
   }
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for long
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, long &data)
{  
   if (ds.ascii()) 
   {
      *ds.istr() >> data;
      ds._fail = ((ds.istr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      INIT_UNPACK;
      UGA_UNPACK_WORD (data, packbuf, packcount, long);
   }
   return ds;
}
STDdstream &
operator << (STDdstream &ds, long data)
{
   if (ds.ascii()) 
   {
       *ds.ostr() << data << " ";
       ds._fail = ((ds.ostr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      INIT_PACK;
      UGA_PACK_WORD (data, packbuf, packcount);
      DONE_PACK;
   }
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for unsigned short
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, unsigned short &data)
{  
   if (ds.ascii()) 
   {
       *ds.istr() >> data;
       ds._fail = ((ds.istr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      INIT_UNPACK;
      UGA_UNPACK_WORD (data, packbuf, packcount, unsigned short);
   }
   return ds;
}
STDdstream &
operator << (STDdstream &ds, unsigned short data)
{
   if (ds.ascii()) 
   {
       *ds.ostr() << data << " ";
       ds._fail = ((ds.ostr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      INIT_PACK;
      UGA_PACK_WORD (data, packbuf, packcount);
      DONE_PACK;
   }
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for unsigned int
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, unsigned int &data)
{  
   if (ds.ascii()) 
   {
      *ds.istr() >> data;
      ds._fail = ((ds.istr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      INIT_UNPACK;
      UGA_UNPACK_WORD (data, packbuf, packcount, unsigned int);
   }
   return ds;
}
STDdstream &
operator << (STDdstream &ds, unsigned int data)
{
   if (ds.ascii()) 
   {
       *ds.ostr() << data << " ";
       ds._fail = ((ds.ostr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      INIT_PACK;
      UGA_PACK_WORD (data, packbuf, packcount);
      DONE_PACK;
   }
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for unsigned long
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, unsigned long &data)
{  
   if (ds.ascii()) 
   {
       *ds.istr() >> data;
       ds._fail = ((ds.istr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      INIT_UNPACK;
      UGA_UNPACK_WORD (data, packbuf, packcount, unsigned long);
   }
   return ds;
}
STDdstream &
operator << (STDdstream &ds, unsigned long data)
{
   if (ds.ascii()) 
   {
       *ds.ostr() << data << " ";
       ds._fail = ((ds.ostr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      INIT_PACK;
      UGA_PACK_WORD (data, packbuf, packcount);
      DONE_PACK;
   }
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for float
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, float &temp)
{  
   if (ds.ascii()) 
   {
       *ds.istr() >> temp;
       ds._fail = ((ds.istr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      // This is called data because INIT_UNPACK uses the size of "data"
      double data;
      INIT_UNPACK;
      UGA_UNPACK_DOUBLE (data, packbuf, packcount);
      temp = (float)data;
   }
   return ds;
}
STDdstream &
operator << (STDdstream &ds, float data)
{
   if (ds.ascii()) 
   {
       *ds.ostr() << data << " ";
       ds._fail = ((ds.ostr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      double temp = data;
      INIT_PACK;
      UGA_PACK_DOUBLE (temp, packbuf, packcount);
      DONE_PACK;
   }
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for double
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, double &data)
{  
   if (ds.ascii()) 
   {
      *ds.istr() >> data;
      ds._fail = ((ds.istr()->fail())?(STD_TRUE):(STD_FALSE));
   } 
   else {
      INIT_UNPACK;
      UGA_UNPACK_DOUBLE (data, packbuf, packcount);
   }
   return ds;
}
STDdstream &
operator << (STDdstream &ds, double data)
{
   if (ds.ascii()) 
   {
       *ds.ostr() << data << " ";
       ds._fail = ((ds.ostr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      INIT_PACK;
      UGA_PACK_DOUBLE (data, packbuf, packcount);
      DONE_PACK;
   }
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for char
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, char &data)
{  
   if (ds.ascii()) 
   {
       *ds.istr() >> data;
       ds._fail = ((ds.istr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else  {
      ds.read_delim();

      ds.read (&data, sizeof(char));
   }
   return ds;
}
STDdstream &
operator << (STDdstream &ds, char data)
{
   if (ds.ascii()) 
   {
       *ds.ostr() << data << " ";
       ds._fail = ((ds.ostr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      ds.write_delim(' ');

      ds.write (&data, sizeof(char));
   }
   return ds;
}

/* -------------------------------------------------------------------------
 * DESCR   :	Stream pack/unpack for unsigned char
 * ------------------------------------------------------------------------- */
STDdstream &
operator >> (STDdstream &ds, unsigned char &data)
{  
   if (ds.ascii()) 
   {
       *ds.istr() >> data;
       ds._fail = ((ds.istr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      ds.read_delim();

      ds.read ((UGAptr)&data, sizeof(unsigned char));
   }
   return ds;
}
STDdstream &
operator << (STDdstream &ds, unsigned char data)
{
   if (ds.ascii()) 
   {
       *ds.ostr() << data << " ";
       ds._fail = ((ds.ostr()->fail())?(STD_TRUE):(STD_FALSE));
   }
   else {
      ds.write_delim(' ');   // write out the delimiter 

      ds.write ((UGAptr)&data, sizeof(unsigned char));
   }
   return ds;
}


void
STDdstream::ws(char *x)
{
   if (ascii())
      (*this) << x;
}

// end of file stream.C
