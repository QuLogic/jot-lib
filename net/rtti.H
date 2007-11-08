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
#ifndef RTTI_H_IS_INCLUDED
#define RTTI_H_IS_INCLUDED

/*****************************************************************
 * rtti.H:
 *
 *      Macros for run-time type identification.
 *****************************************************************/

#define IS(N)          (N == static_name())
#define ISA(O)         ((O) && (O)->is_of_type(static_name()))
#define ISOF(N,PAR)    (IS(N) || PAR::is_of_type(N))

#define ISOF2(N,PAR1,PAR2) \
   (IS(N) || PAR1::is_of_type(N) || PAR2::is_of_type(N))

#ifdef JOT_AVOID_STATIC_LOCAL_INLINE_VAR
#define STAT_STR_RET str_ptr
#else
#define STAT_STR_RET Cstr_ptr &
#endif

//
// For base class definitions:
//
#define DEFINE_RTTI_METHODS_BASE(CLASS_NAME, BASECLASS_PTR) \
   static  STAT_STR_RET static_name()         { RET_STAT_STR(CLASS_NAME); } \
   virtual STAT_STR_RET class_name () const   { return static_name(); } \
   virtual int  is_of_type(Cstr_ptr &n)const  { return n == static_name(); } \
   static  int  isa       (BASECLASS_PTR o)   { return ISA (o); }

//
// For derived class definitions:
//
#define DEFINE_RTTI_METHODS2(CLASS_NAME, CLASS_PAR, BASECLASS_PTR) \
   static  STAT_STR_RET static_name()         { RET_STAT_STR(CLASS_NAME);} \
   virtual STAT_STR_RET class_name () const   { return static_name(); } \
   virtual int  is_of_type(Cstr_ptr &n)const  { return ISOF(n,CLASS_PAR);} \
   static  int  isa       (BASECLASS_PTR o)   { return ISA (o); }

#define DEFINE_RTTI_METHODS(CLASS_NAME, CLASS_PAR, BASECLASS_PTR) \
        DEFINE_RTTI_METHODS2(CLASS_NAME, CLASS_PAR, BASECLASS_PTR &)

//
// Handy macro for doing upcasts:
//
#define DEFINE_UPCAST(CLASS_PTR, BASECLASS_PTR) \
   static CLASS_PTR upcast(BASECLASS_PTR p) { \
      return isa(p) ? (CLASS_PTR)p : 0; \
   }

//
// Same as DEFINE_RTTI_METHODS2, but also defines upcast:
//
#define DEFINE_RTTI_METHODS3(CLASS_NAME, CLASS_PTR, CLASS_PAR, BASECLASS_PTR)\
   DEFINE_RTTI_METHODS2(CLASS_NAME, CLASS_PAR, BASECLASS_PTR) \
   DEFINE_UPCAST(CLASS_PTR, BASECLASS_PTR)

#endif // RTTI_H_IS_INCLUDED

/* end of file rtti.H */
