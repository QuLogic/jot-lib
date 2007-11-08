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
#ifndef ERROR_H_IS_INCLUDED
#define ERROR_H_IS_INCLUDED

#include "platform.H"
#include "iostream.H"


// Warning Level
//
// Variable JOT_WARNING_LEVEL is querried before printing messages.
// Only message with ERR_LEV <= JOT_WARNING_LEVEL are printed.
// JOT_WARNING_LEVEL defaults to ERR_LEV_WARN (2)

//
// ERR_LEV_ERROR - (1) - Fatal errors (e.g. file not found, failed calculation, etc.)
// ERR_LEV_WARN  - (2) - Non-fatal/recovered errors (e.g. tossed out bad gesture, data, etc.)
// ERR_LEV_INFO  - (3) - Something informational (e.g. dimensions, bitplanes of loaded image)
// ERR_LEV_SPAM  - (4) - Gratuitious information (e.g. "Class::Class() - Constructed!")

#define ERR_LEV_ERROR    0x01
#define ERR_LEV_WARN     0x02
#define ERR_LEV_INFO     0x03
#define ERR_LEV_SPAM     0x04

#define ERR_LEV_MASK     0x0F

// ERRNO String - Appends errno string to error message
//
#define ERR_INCL_ERRNO   0x10

// Internal method -- don't call this one...
void err_(int flags, const char *fmt, va_list ap);

// Print error message:
//
// flags = ERR_INCL_ERRNO | one of ERR_LEV_* 
//
// E.g.: the following always prints:
//    err_mesg(ERR_LEV_ERROR, ...);
// but this only prints when JOT_WARNING_LEVEL == ERR_LEV_SPAM:
//    err_mesg(ERR_LEV_SPAM, ...);
inline void
err_mesg(int flags, const char *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   err_(flags, fmt, ap);
   va_end(ap);
}

// Conditionally print error message:
//
// doit = only print message if doit=true
// flags = ERR_INCL_ERRNO | one of ERR_LEV_* 
inline void
err_mesg_cond(bool doit, int flags, const char *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   if (doit) err_(flags, fmt, ap);
   va_end(ap);
}

// convenience form of err_mesg: always prints
inline void
err_msg(const char *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   err_(ERR_LEV_ERROR, fmt, ap);
   va_end(ap);
}

// err_ret: print message about a system error,
//          and return.
inline void
err_ret(const char *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   err_(ERR_INCL_ERRNO | ERR_LEV_ERROR, fmt, ap);
   va_end(ap);
}

// err_sys: like err_ret, but also exits the program.
inline void
err_sys(const char *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   err_(ERR_INCL_ERRNO | ERR_LEV_ERROR, fmt, ap);
   va_end(ap);
   exit(1);
}

// err_adv: (error advisory):
//
//   convenience method used in debugging.
//   instead of:
//      if (print_errs)
//         err_msg("problem...");
//   use:
//      err_adv(print_errs, "problem...");
inline void
err_adv(bool doit, const char *fmt, ...)
{
   if (!doit) return;

   va_list ap;
   va_start(ap, fmt);
   err_(ERR_LEV_ERROR, fmt, ap);
   va_end(ap);
}

#endif // ERROR_H_IS_INCLUDED

/* end of file error.H */
