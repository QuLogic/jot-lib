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
#include "std/time.H"
#include "disp/view.H"
#include "stroke/stroke_generator.H"

#include <cstdlib>

static const int    INITIAL_WINDOW_SIZE   = 4;
static const double INITIAL_RHO_PERCENT   = 0.01;
static const double INITIAL_Q_TOLERANCE   = 10.0;
static const double INITIAL_RESAMPLE_RATE = 4.0;

static const int    MAX_Q_ITERS           = 100;
static const double BIG_DIST              = 10000.0;
//static const double BIGGEST_VALID_DIST    = 500.0;
static const double SMALLEST_VALID_DIST   = 0.0001;
static const int    NUM_BUCKETS           = 32;


StrokeGenerator::StrokeGenerator() {
   _regen_table   = true;
   _window_size   = INITIAL_WINDOW_SIZE;
   _rho_percent   = INITIAL_RHO_PERCENT;
   _q_tolerance   = INITIAL_Q_TOLERANCE;
   _resample_rate = INITIAL_RESAMPLE_RATE;
   _do_q_learning = true;
   reseed((long) the_time());
}


StrokeGenerator::~StrokeGenerator() {
}


void
StrokeGenerator::clear() {
   _examples.clear();
   _regen_table = true;
}


void
StrokeGenerator::add_example(BaseStrokeOffsetLISTptr o) {

   if(o->num() < 2) {
      return;
   }

   // Linearly resample example stroke.
   // XXX - Try putting in cubic resampling later?
   ARRAY<double> ex;
   double pix_len    = o->get_pix_len();
   if(pix_len <= 0) {
      cerr << "BaseStrokeOffsetLIST pix_len not set" << endl;
      return;
   }

   double len1       = (*o)[0]._len;
   double len2       = (*o)[1]._len;
   double delta_len  = len2 - len1;
   double pos1       = (*o)[0]._pos * pix_len;
   double pos2       = (*o)[1]._pos * pix_len;
   double delta_pos  = pos2 - pos1;
   double curr_pos   = 0.0;
   double seg_left   = 0.0;
   int    i = 1;
   while(i < o->num()) {
      if(delta_pos >= seg_left + curr_pos &&
         delta_pos >  0.0) {
         // The next resampled point lies on the current segment.
         curr_pos += seg_left;
         seg_left = _resample_rate;
         double p = len1 + (len2 - len1) * (curr_pos / delta_pos);
         ex.add(p);

      } else {
         // The next resampled point lies beyond the current segment.
         i ++;
         seg_left  -= (delta_pos - curr_pos);
         len1      = len2;
         len2      = (*o)[i]._len;
         delta_len = len2 - len1;
         pos1      = pos2;
         pos2      = (*o)[i]._pos * pix_len;
         delta_pos = pos2 - pos1;
         curr_pos  = 0.0;
      }
   }
   //ex.add(len2);
   
   //for(i = 0; i < ex.num(); i ++) {
      //cerr << i << " " << ex[i] << endl;
   //}
   _examples.add(ex);
   _regen_table = true;
}


void
StrokeGenerator::generate(double pix_len, BaseStrokeOffsetLISTptr o) {
//  unsigned short int saved_state[3];
//     unsigned short *prev_state;
//     int i;

   // Make sure table is up-to-date.
   if(_regen_table) {
      regenerate_table();
      _regen_table = false;
   }

   o->clear();
   if(_start_states.num() == 0 ||
      pix_len <= 0) {
      return;
   }

   // Save global random generator state/restore internal state.

   // XXX -- commenting out because seed48 isn't available on WIN32
//     prev_state = seed48(_rand_state);
//     for(i = 0; i < 3; i ++) {
//        saved_state[i] = prev_state[i];
//     }

   //const ARRAY<double> &ex = _examples[0];
   //i = 0;

   // Pick a start state at random.
   int si = (int) (drand48() * (double) _start_states.num());
   int state = _start_states[si];
   double curr_pos = 0.0;
   while(curr_pos < pix_len) {
      const ARRAY<int> &prob_table = _table[state];

      // Pick a transition.
      int t = (int) (drand48() * (double) prob_table.num());
      state = prob_table[t];
      if(state == -1) {
         // End of example.  Pick a new start state.
         cerr << "Stroke generation hit end of example.  Starting again."
              << endl;
         si = (int) (drand48() * (double) _start_states.num());
         state = _start_states[si];
      }

      // Add point.
      BaseStrokeOffset bso;
      bso._pos  = curr_pos / pix_len;
      bso._len  = _state_vals[state];
      bso._type = BaseStrokeOffset::OFFSET_TYPE_MIDDLE;
      bso._press = 1;

      //bso._len = ex[i % ex.num()];
      //i ++;

      curr_pos += _resample_rate;
      o->add(bso);
   }

   // Fix types at beginning and end.
   int n = o->num();
   (*o)[0]._type = BaseStrokeOffset::OFFSET_TYPE_BEGIN;
   (*o)[n-1]._type = BaseStrokeOffset::OFFSET_TYPE_END;

   // Save internal random generator state, restore global state.
   // XXX -- commenting out because seed48 isn't available on WIN32
//     prev_state = seed48(saved_state);
//     for(i = 0; i < 3; i ++) {
//        _rand_state[i] = prev_state[i];
//     }
}


void
StrokeGenerator::reseed(long int seedval) {
//     unsigned short int saved_state[3];
//     unsigned short int dummy[3];
//     const unsigned short int *prev_state;
//     int i;

   // Save global state.
   // XXX -- seed48 not available on WIN32
//     prev_state = seed48(dummy);
//     for(i = 0; i < 3; i ++) {
//        saved_state[i] = prev_state[i];
//     }

   // Reseed and save internal state.
   srand48(seedval);

   // XXX -- seed48 not available on WIN32
//     prev_state = seed48(dummy);
//     for(i = 0; i < 3; i ++) {
//        _rand_state[i] = prev_state[i];
//     }

   // Restore global state.
   //seed48(saved_state);
}


static void
s_qLearning(ARRAY<ARRAY<double> > &in_mat,
            ARRAY<ARRAY<double> > &out_mat,
            double q_tolerance) {
   int i, j;
   double alpha = 0.999;
   double p = 2.0;

   // Raise the input matrix to the power of p.
   int n = in_mat.num();
   for(i = 0; i < n; i ++) {
      for(j = 0; j < n; j ++) {
         in_mat[i][j] = pow(in_mat[i][j], p);
      }
   }

   // Copy input matrix to output matrix.
   out_mat = in_mat;

   // Recalculate m.
   ARRAY<double> m(n);
   ARRAY<double> m_pre_mult(n);
   for(i = 0; i < n; i ++) {
      m += 0.0;
      m_pre_mult += 0.0;
   }
   for(i = 0; i < n; i ++) {
      m[i] = out_mat[i][0];
      for(j = 1; j < n; j ++) {
         if(out_mat[i][j] < m[i] && out_mat[i][j] > 0.0) {
            m[i] = out_mat[i][j];
         }
      }
      m_pre_mult[i] = m[i] * alpha;
   }

   double total_change = q_tolerance + 1.0;
   int    iters        = 0;
   while(total_change > q_tolerance) {
      // Recalculate out_mat.
      total_change = 0.0;

      for(j = n - 1; j >= 0; j --) {
         for(i = 0; i < n; i ++) {
            double v = in_mat[i][j] + m_pre_mult[j];
            double change = v - out_mat[i][j];
            if(change < 0) {
               total_change -= change;
            } else {
               total_change += change;
            }
            out_mat[i][j] = v;
            if(v > 0.0 && v < m[i]) {
               m[i] = v;
               m_pre_mult[i] = v * alpha;
            }
         }
      }
      iters ++;
      if(iters >= MAX_Q_ITERS) {
         cerr << "Stroke generator Q-learning quitting after " <<
                 iters << " iterations." << endl;
         break;
      }
      //cerr << "Total change: " << total_change << endl;
   }
}

void StrokeGenerator::regenerate_table() {
   _table.clear();
   _start_states.clear();
   _state_vals.clear();

   if(_examples.num() == 0) {
      return;
   }

   // Count up the number of points and find the biggest magnitude
   // (absolute value) example.
   int i, j, k;
   int total_points      = 0;
   double biggestExample = 0.0;
   for(i = 0; i < _examples.num(); i ++) {
      const ARRAY<double> &ex = _examples[i];
      // Count points.
      total_points += ex.num();
      for(j = 0; j < ex.num(); j ++) {
         if(fabs(ex[j]) > biggestExample) {
            biggestExample = fabs(ex[j]);
         }
      }

      // Count separator.
      total_points += 1;
   }
   //total_points += 1;

   // Aggregate len values and separators.
   _state_vals = ARRAY<double>(total_points);
   ARRAY<bool>   seps(total_points);
   int           point_index = 0;
   for(i = 0; i < _examples.num(); i ++) {
      const ARRAY<double> &ex = _examples[i];

      // Add points.
      for(j = 0; j < ex.num(); j ++) {
         _state_vals += ex[j];
         seps += false;
         point_index ++;
      }

      // Add separator.
      _state_vals += 0.0;
      seps        += true;
      point_index ++;
   }
   //_state_vals += 0.0;
   //seps += true;
   //point_index ++;

   assert(point_index == total_points);

   // Calculate distance matrix.
   ARRAY<ARRAY<double> > D(total_points);
   for(i = 0; i < total_points; i ++) {
      D += ARRAY<double>(total_points);
      for(j = 0; j < total_points; j ++) {
         if(seps[i] || seps[j]) {
            //if(!seps[i] || !seps[j]) {
            if(seps[j]) {
               D[i] += BIG_DIST;
            } else {
               D[i] += 0.0;
            }
         } else {
            D[i] += fabs(_state_vals[i] - _state_vals[j]);
         }
      }
   }

   // Set filter values.
   ARRAY<double> W(_window_size);
   for(i = 0; i < _window_size; i ++) {
      W += 1.0 / (double) _window_size;
      //W += 1.0 / (1.0 + i);
   }

   // Filter distance matrix.
   ARRAY<ARRAY<double> > D1(total_points);
   for(i = 0; i < total_points; i ++) {
      D1 += ARRAY<double>(total_points);
      for(j = 0; j < total_points; j ++) {
         double total = 0.0;
         for(k = 0; k < _window_size; k ++) {
            //if(seps[i - k] || seps[j - k]) {
            if(i - k < 0 || j - k < 0) {
               //break;
               total += 0.0;
            } else {
               total += W[k] * D[i - k][j - k];
            }
         }
         D1[i] += total;

         //if(j == total_points - 1 && i != j) {
            //D1[i] += BIG_DIST;
         //} else if(k == _window_size) {
         //if(k == _window_size) {
            //D1[i] += total;
         //} else {
            /*if(i == j) {
               D1[i] += 0.0;
            } else {
               D1[i] += BIG_DIST;
            }*/
            //D1[i] += BIG_DIST + (total - BIG_DIST) * (k / _window_size);
         //}
      }
   }

   // Do q-learning, if requested.
   ARRAY<ARRAY<double> > D2(total_points);
   if(_do_q_learning) {
      for(i = 0; i < total_points; i ++) {
         D2 += ARRAY<double>(total_points);
         for(j = 0; j < total_points; j ++) {
            D2[i] += 0.0;
         }
      }
      s_qLearning(D1, D2, _q_tolerance);
   } else {
      D2 = D1;
   }

   // Calculate rho.
   double rho_total = 0.0;
   int rho_count = 0;
   for(i = 0; i < total_points; i ++) {
      for(j = 0; j < total_points; j ++) {
         if(D2[i][j] > SMALLEST_VALID_DIST &&
            D2[i][j] < biggestExample) {
            rho_total += D2[i][j];
            rho_count ++;
         }
      }
   }
   double rho;
   if(rho_count > 0) {
      rho = rho_total / (double) rho_count;
      rho *= _rho_percent;
   } else {
      rho = 1.0;
   }

   // Generate probability values.
   ARRAY<ARRAY<double> > P(total_points);
   for(i = 0; i < total_points; i ++) {
      P += ARRAY<double>(total_points);
      for(j = 0; j < total_points; j ++) {
         if(i < total_points - 1) {
            P[i] += exp(-D2[i+1][j] / rho);
         } else {
            if(j == i + 1) {
               P[i] += 1.0;
            } else {
               P[i] += 0.0;
            }
         }
      }
   }

   // Normalize probability matrix.
   for(i = 0; i < total_points; i ++) {
      double total = 0.0;
      for(j = 0; j < total_points; j ++) {
         total += P[i][j];
      }
      if(total == 0.0) {
         continue;
      }
      double scale_factor = (double) NUM_BUCKETS / total;
      for(j = 0; j < total_points; j ++) {
         P[i][j] = P[i][j] * scale_factor;
      }
   }

   // Construct transition table.
   _table = ARRAY<ARRAY<int> >(total_points);
   for(i = 0; i < total_points; i ++) {
      _table += ARRAY<int>(NUM_BUCKETS);
      for(j = 0; j < NUM_BUCKETS; j ++) {
         _table[i] += 0;
      }
      _table[i][0] = -1;
      double total_prob = 0.0;
      for(j = 0; j < total_points; j ++) {
         double new_total_prob = total_prob + P[i][j];
         int start_val = (int) total_prob;
         int end_val   = (int) new_total_prob;
         for(k = start_val; k < end_val; k ++) {
            _table[i][k] = j;
         }
         total_prob = new_total_prob;
      }
      for(j = (int)total_prob; j < NUM_BUCKETS; j ++) {
         if(total_prob < 1.0) {
            _table[i][j] = -1;
         } else {
            _table[i][j] = _table[i][j - 1];
         }
      }
   }

   // Construct starting points.
   _start_states.clear();
   for(i = 0; i < total_points - 1; i ++) {
      if(seps[i]) {
         _start_states.add(i + 1);
      }
   }
}


// vim:et:sta:sw=3:cino=(0:
