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
#ifndef TAB_FN_H_IS_INCLUDED
#define TAB_FN_H_IS_INCLUDED

/*!
 *  \file tab_fn.H
 *  \brief Contains the definition of the TabulatedFunction class.
 *  \ingroup group_MLIB
 *
 */

#include "std/error.H"

namespace mlib {

//! \brief Function pointer type for the type of functions that can be tabulated
//! by the TabulatedFunction class.
//! \relates TabulatedFunction
typedef double (*real_fn_t)(double);

/*!
 *  \brief Tabulates values of a given function over an interval [a,b]
 *  using a specified resolution.
 *  \ingroup group_MLIB
 *  
 *  Approximations of the function at values in [a,b) are returned as
 *  interpolations of the tabulated values. at values outside that range the
 *  function itself is used to compute the value.
 *
 */
class TabulatedFunction
{
   
   protected:
 
      //! a (the start of the interval over which the function is tabulated)
      double       _a;
      //! b (the end of the interval over which the function is tabulated)
      double       _b;
      //! b - a (the length of the interval over which the function is tabulated)
      double       _len;
      int          _res;           //!< resolution (# of subintervals)
      double       _rlen;          //!< _res / _len (used in computation)
      double*      _tab;           //!< table of values
      real_fn_t    _f;             //!< the given function

   public:
   
      //! \name Constructors
      //@{
 
      //! \brief Constructor that tabulates the function \p f over the range
      //! [a,b) by taking \p res samples.
      TabulatedFunction(double a, double b, real_fn_t f, int res) :
         _tab(0) {
         init(a,b,f,res);
      }
      
      //! \brief Constructor that creates a tabulated function over the range
      //! [a,b) from the values in \p vals.
      //!
      //! \p vals should have at least \p res elements.
      TabulatedFunction(double a, double b, double vals[], int res) :
         _tab(0) {
         init(a,b,vals,res);
      }
      
      //@}
      
      //! \name Initialization Functions
      //@{
         
      //! \brief Creates a tabulated function over the range
      //! [a,b) from the values in \p vals.
      //!
      //! \note \p vals should have at least \p res elements.
      //!
      //! \param[in] a The start of the range of the tabulated function.
      //! \param[in] b The end of the range of the tabulated function.
      //! \param[in] vals The values to use as samples of the tabulated function.
      //! \param[in] res The number of samples in \p vals.
      //!
      //! \bug This function checks the for allocation errors from the new
      //! operator incorrectly (new doesn't return a null pointer on failure,
      //! it throws a bad_alloc exception).
      void init(double a, double b, double vals[], int res) {
         _a = a;
         _b = b;
         _len = (b-a);
         _f = 0;
         _res = res;
         
         delete _tab;
         _tab = 0;
      
         if (_len <= 0 ) {
            err_msg("TabulatedFunction::init: interval (%f,%f) is bad", a, b);
         } else if (_res < 1) {
            err_msg("TabulatedFunction::init: resolution (%d) is too small", _res);
         } else if ((_tab = new double [ _res + 1 ]) == 0) {
            err_ret("TabulatedFunction::init: can't allocate table");
         } else {
            _rlen = _res / _len;
            for (int n=0; n<=_res; n++) {
               _tab[n] = vals[n];
            }
         }
      }
      
      //! \brief Tabulates the function \p f over the range
      //! [a,b) by taking \p res samples.
      //!
      //! \param[in] a The start of the range over which to tabulate \p f.
      //! \param[in] b The end of the range over which to tabulate \p f.
      //! \param[in] f The function to tabulate.
      //! \param[in] res The number of samples to take while tabulating \p f.
      //!
      //! \bug This function checks the for allocation errors from the new
      //! operator incorrectly (new doesn't return a null pointer on failure,
      //! it throws a bad_alloc exception).
      void init(double a, double b, real_fn_t f, int res) {
         _a = a;
         _b = b;
         _len = (b - a);
         _f = f;
         _res = res;
      
         delete _tab;
         _tab = 0;
      
         if (_len <= 0 ) {
            err_msg("TabulatedFunction::init: interval (%f,%f) is bad", a, b);
         } else if (_res < 1) {
            err_msg("TabulatedFunction::init: resolution (%d) is too small", _res);
         } else if ((_tab = new double [ _res + 1 ]) == 0) {
            err_ret("TabulatedFunction::init: can't allocate table");
         } else {
            _rlen = _res / _len;
            for (int n=0; n<=_res; n++) {
               _tab[n] = (*_f)(n/_rlen + _a);
            }
         }
      }
      
      //@}
      
      //! \name Tabulated Function Evaulation
      //@{
      
      //! \brief Evaluates the tabulated function at \p x.
      //!
      //! Computes the interpolated values of the tabulated function at \p x.
      //!
      //! If \p x is out of bounds, evaluate the function directly if we have it.
      //! If we don't have it, extend the tabulated function to be constant
      //! outside of its defined range.
      double map(double x) const {
         if (x<_a || x>=_b || !_tab)
            return _f ? (*_f)(x) : _tab ? _tab[x<_a? 0 : _res] : 0;
         else {
            double t = _rlen*(x - _a);
            int n = (int) floor(t);
            double d = t - n;
            return _tab[n] + (_tab[n+1] - _tab[n])*d;
         }
      }
      
      //@}
      
};

} // namespace mlib

#endif // TAB_FN_H_IS_INCLUDED

/* end of file tab_fn.H */
