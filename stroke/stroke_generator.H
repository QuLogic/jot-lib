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
//--------------------------------------------------------------------------
//
// Contents:
//    class StrokeGenerator
//
// Description:
//    This class, given sets of stroke examples, generates new strokes
//    using a variation on the SIGGRAPH 2000 video textures algorithm.
//
// Author:
//    jcl (12/10/01)
//
//--------------------------------------------------------------------------



#ifndef _STROKE_GEN_H_
#define _STROKE_GEN_H_

#include "base_stroke.H"

class StrokeGenerator {
 protected:
   ARRAY<ARRAY<double> > _examples;

   // Indicates that the transition table needs to be regenerated.
   bool _regen_table;

   // The number of past samples to consider in generating the next sample.
   int _window_size;

   // Controls the pixel rate at which the examples are resampled.
   double _resample_rate;

   // Controls transition probability.  High values mean strokes look
   // more like examples, low values mean strokes look more random.
   double _rho_percent;

   // Controls the amount of time spent in q-learning.  Low tolerences
   // mean longer processing time, but better dead-end avoidance.
   double _q_tolerance;

   // Controls whether q-learning is performed.
   bool _do_q_learning;

   // The random number generator state.
   unsigned short int _rand_state[3];

   // Transition table.
   ARRAY<ARRAY<int> > _table;
   ARRAY<int>         _start_states;
   ARRAY<double>      _state_vals;

   // Regenerates the transition table.
   void regenerate_table();

 public:
   StrokeGenerator();
   virtual ~StrokeGenerator();

   // Clears the examples from the generator.
   void clear();

   // Adds an example to the generator.
   void add_example(BaseStrokeOffsetLISTptr o);

   // Generates a new stroke from examples.
   void generate(double pixLen, BaseStrokeOffsetLISTptr o);

   // Reseeds the random number generator.
   void reseed(long int seedval);

   void set_do_q_learning(bool do_q_learning) {
      _do_q_learning = do_q_learning;
   }

   bool get_do_q_learning() {
      return _do_q_learning;
   }
};


#endif
