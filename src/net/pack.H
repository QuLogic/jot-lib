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
#ifndef PACK_H
#define PACK_H

/*
 * These routines safely pack and unpack double, words and byte
 * strings. bcopy is not used for efficiency reasons, even though
 * code size slightly increases. Have to be careful with alignment,
 * that is why byte by byte copy is used. 
 * Assumes: 8-byte doubles
 *          4-byte words
 * Note that if across multiple architectures of different word size
 * then must agree on largest word size and change the macros
 * appropriately, else will not be able to communicate between 
 * architectures.
 */

#if defined(i386) || defined(WIN32)
#define UGA_LITTLE_ENDIAN
#endif

/* UGA packing routines for words, doubles and bytes */

#ifndef UGA_LITTLE_ENDIAN
#define UGA_PACK_DOUBLE(QUAN, BUF, COUNT)			\
        do { char *_buf_ = BUF, *_ptr_ = (char *)&(QUAN);       \
        *((_buf_)++) = *((_ptr_)++);				\
        *((_buf_)++) = *((_ptr_)++);				\
        *((_buf_)++) = *((_ptr_)++);				\
        *((_buf_)++) = *((_ptr_)++);				\
        *((_buf_)++) = *((_ptr_)++);				\
        *((_buf_)++) = *((_ptr_)++);				\
        *((_buf_)++) = *((_ptr_)++);				\
        *((_buf_)++) = *((_ptr_)++);				\
        (BUF) = _buf_;						\
        (COUNT) += 8; } while (0)
#else
#define UGA_PACK_DOUBLE(QUAN, BUF, COUNT)			\
        do { char *_buf_ = BUF, *_ptr_ = (char *)&(QUAN)+7;     \
        *((_buf_)++) = *((_ptr_)--);				\
        *((_buf_)++) = *((_ptr_)--);				\
        *((_buf_)++) = *((_ptr_)--);				\
        *((_buf_)++) = *((_ptr_)--);				\
        *((_buf_)++) = *((_ptr_)--);				\
        *((_buf_)++) = *((_ptr_)--);				\
        *((_buf_)++) = *((_ptr_)--);				\
        *((_buf_)++) = *((_ptr_)--);				\
        (BUF) = _buf_;						\
        (COUNT) += 8; } while (0)
#endif

#define UGA_PACK_NDOUBLE(QUAN, BUF, NUM, COUNT)		        \
        do { int _i;						\
          for (_i=0; _i<(int)NUM; _i++)				\
               UGA_PACK_DOUBLE(QUAN[_i], BUF, COUNT); } while (0)

#define UGA_PACK_WORD(QUAN, BUF, COUNT)				\
        do { u_long _long_ = htonl((u_long)QUAN);		\
        char *_buf_ = BUF, *_ptr_;				\
        _ptr_ = (char *)&_long_;				\
        *((_buf_)++) = *((_ptr_)++);				\
        *((_buf_)++) = *((_ptr_)++);				\
        *((_buf_)++) = *((_ptr_)++);				\
        *((_buf_)++) = *((_ptr_)++);				\
        (BUF) = _buf_;						\
        (COUNT) += 4; } while (0)

#define UGA_PACK_NWORD(QUAN, BUF, NUM, COUNT)			\
        do { int _i;						\
          for (_i=0; _i<(int)NUM; _i++)				\
                UGA_PACK_WORD(QUAN[_i], BUF, COUNT); } while (0)

#define UGA_PACK_BYTES(PTR, BUF, SIZE, COUNT)			\
        ((void)bcopy((char *)(PTR),(char *)(BUF),(int)(SIZE)),  \
         BUF += (SIZE),(COUNT) += (SIZE))

/* UGA unpacking routines for words, doubles and bytes */

#ifndef UGA_LITTLE_ENDIAN
#define UGA_UNPACK_DOUBLE(QUAN, BUF, COUNT)			\
        do { char *_buf_ = BUF, *_ptr_ = (char *)&(QUAN);	\
        *((_ptr_)++) = *((_buf_)++);				\
        *((_ptr_)++) = *((_buf_)++);				\
        *((_ptr_)++) = *((_buf_)++);				\
        *((_ptr_)++) = *((_buf_)++);				\
        *((_ptr_)++) = *((_buf_)++);				\
        *((_ptr_)++) = *((_buf_)++);				\
        *((_ptr_)++) = *((_buf_)++);				\
        *((_ptr_)++) = *((_buf_)++);				\
        (BUF) = _buf_;						\
        (COUNT) += 8; } while (0)
#else
#define UGA_UNPACK_DOUBLE(QUAN, BUF, COUNT)			\
        do { char *_buf_ = BUF, *_ptr_ = (char *)&(QUAN)+7;	\
        *((_ptr_)--) = *((_buf_)++);				\
        *((_ptr_)--) = *((_buf_)++);				\
        *((_ptr_)--) = *((_buf_)++);				\
        *((_ptr_)--) = *((_buf_)++);				\
        *((_ptr_)--) = *((_buf_)++);				\
        *((_ptr_)--) = *((_buf_)++);				\
        *((_ptr_)--) = *((_buf_)++);				\
        *((_ptr_)--) = *((_buf_)++);				\
        (BUF) = _buf_;						\
        (COUNT) += 8; } while (0)
#endif

#define UGA_UNPACK_NDOUBLE(QUAN, BUF, NUM, COUNT)		\
        do { int _i;						\
          for (_i=0; _i<(int)NUM; _i++)				\
               UGA_UNPACK_DOUBLE(QUAN[_i], BUF, COUNT); } while (0)

#define UGA_UNPACK_WORD(QUAN, BUF, COUNT, TYPE)			\
        do { char *_buf_ = BUF, *_ptr_ = (char *)&(QUAN);	\
        *((_ptr_)++) = *((_buf_)++);				\
        *((_ptr_)++) = *((_buf_)++);				\
        *((_ptr_)++) = *((_buf_)++);				\
        *((_ptr_)++) = *((_buf_)++);				\
        (BUF) = _buf_;						\
        (QUAN) = (TYPE)ntohl((u_long)QUAN);			\
        (COUNT) += 4; } while (0)

#define UGA_UNPACK_NWORD(QUAN, BUF, NUM, COUNT, TYPE)		\
        do { int _i;						\
          for (_i=0; _i<(int)NUM; _i++)				\
                UGA_UNPACK_WORD(QUAN, BUF, COUNT, TYPE); } while (0)

#define UGA_UNPACK_BYTES(PTR, BUF, SIZE, COUNT)			\
        ((void)bcopy((char *)(BUF), (char *)(PTR), (int)(SIZE)),\
         BUF += (SIZE),(COUNT) += (SIZE))

/*
 * Additional types that are currently based on the above macros, but
 * could change, depending on architecture.
 * long = word
 * int  = word
 * enum = word
 * ptr  = word
 */

#define UGA_PACK_LONG(Q,B,C)		UGA_PACK_WORD(Q,B,C)
#define UGA_PACK_NLONG(Q,B,N,C) 	UGA_PACK_NWORD(Q,B,N,C)
#define UGA_PACK_INTEGER(Q,B,C) 	UGA_PACK_WORD(Q,B,C)
#define UGA_PACK_NINTEGER(Q,B,N,C) 	UGA_PACK_NWORD(Q,B,N,C)
#define UGA_PACK_ENUM(Q,B,C)		UGA_PACK_WORD(Q,B,C)
#define UGA_PACK_NENUM(Q,B,N,C)		UGA_PACK_NWORD(Q,B,N,C)
#define UGA_PACK_POINTER(Q,B,C)		UGA_PACK_WORD(Q,B,C)
#define UGA_PACK_NPOINTER(Q,B,N,C)	UGA_PACK_NWORD(Q,B,N,C)

#define UGA_UNPACK_LONG(Q,B,C)		UGA_UNPACK_WORD(Q,B,C,long)
#define UGA_UNPACK_NLONG(Q,B,N,C) 	UGA_UNPACK_NWORD(Q,B,N,C,long)
#define UGA_UNPACK_INTEGER(Q,B,C) 	UGA_UNPACK_WORD(Q,B,C,int)
#define UGA_UNPACK_NINTEGER(Q,B,N,C) 	UGA_UNPACK_NWORD(Q,B,N,C,int)
#define UGA_UNPACK_ENUM(Q,B,C,T)	UGA_UNPACK_WORD(Q,B,C,T)
#define UGA_UNPACK_NENUM(Q,B,N,C,T)	UGA_UNPACK_NWORD(Q,B,N,C,T)
#define UGA_UNPACK_POINTER(Q,B,C,T)	UGA_UNPACK_WORD(Q,B,C,T)
#define UGA_UNPACK_NPOINTER(Q,B,N,C,T)	UGA_UNPACK_NWORD(Q,B,N,C,T)

#define OPSYS_ALIGN_CHAR    sizeof(char)
#define OPSYS_ALIGN_INT     sizeof(int)
#define OPSYS_ALIGN_SHORT   sizeof(short)
#define OPSYS_ALIGN_LONG    sizeof(long)
#define OPSYS_ALIGN_FLOAT   sizeof(float)
#define OPSYS_ALIGN_DOUBLE  sizeof(double)
#define OPSYS_ALIGN_GENERIC (sizeof(double)>sizeof(long)?sizeof(double):sizeof(long))

#endif
