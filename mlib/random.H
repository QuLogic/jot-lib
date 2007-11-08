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
#ifndef RANDOM_H_IS_INCLUDED
#define RANDOM_H_IS_INCLUDED

/*!
 *  \file random.H
 *  \brief Contains the definition of the RandomGen class.
 *  \ingroup group_MLIB
 *
 */

namespace mlib {

/*!
 *  \brief Random number generator class.
 *  \ingroup group_MLIB
 *
 *  Private to objects (a la meshes) who need their own source of random
 *  variables.
 *
 */
class RandomGen
{

   public:
   
      //! \name Constructors
      //@{

      //! \brief Default constructor.  Creates a random number generator with
      //! a default seed (1).
      RandomGen() { srand(); }
      
      //! \brief Constructor that creates a random number generator with the
      //! seed given in the argument.
      RandomGen(long i) { srand(i);}
      
      //@}
      
      //! \brief The maximum integer value that can be produced by this random
      //! number generator.
      static long R_MAX;
      
      //! \name Seed Accessor Functions
      //@{
      
      //! \brief Seeds the random number generator with the seed \p i.
      void srand(long i=1) { _seed = i ; }
      
      //! \brief Gets the current seed value.
      //!
      //! \note This changes each time a new number is generated.
      long get_seed() { return _seed; }
      
      //@}
      
      //! \name Random Number Generation Functions
      //@{
      
      //! \brief Generate a random long integer.
      long lrand() { update(); return _seed;  }
      
      //! \brief Generate a random double precision floating point value in the
      //! range [0.0, 1.0].
      double drand() { return (double) lrand()/(double)R_MAX ; }
      
      //@}

   private:

      long _seed;    //!< The current seed value.
      
      //! \brief Generate the next seed (i.e. the next random value).
      void update();

};

} // namespace mlib

#endif // RANDOM_H_IS_INCLUDED
