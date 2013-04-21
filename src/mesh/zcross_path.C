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
/**********************************************************************
 * zcross_path.C:
 **********************************************************************/


#include "bmesh.H"
#include "stripcb.H"

/**********************************************************************
 * ZcrossPath:
 **********************************************************************/
void
ZcrossPath::get_g(Bface * f , double g[], int& ex1, int&  ex2)
{
   int i;
   Wvec v1;
   for ( i=0; i < 3; i++ ) {
      g[i] = (_eye - f->v(i+1)->loc()).normalized() * f->vert_normal(f->v(i+1), v1);
      if ( g[i] == 0 ) {
         //cerr << "g = 0! setting to -1e-8\n";
         g[i] = -1e-8;    // sinful but convenient
      }
   }

   if ( g[0] > 0 ) {
      if( g[1] > 0 ){
         ex1 = 1; ex2 = 2;
      } else if( g[2] > 0 ){
         ex1 = 0; ex2 = 1;
      } else {
         ex1 = 0; ex2 = 2;
      }
   } else { //g0 < 0
      if( g[1] < 0 ){
         ex1 = 1; ex2 = 2;
      } else if( g[2] < 0 ){
         ex1 = 0; ex2 = 1;
      } else {
         ex1 = 0; ex2 = 2;
      }
   }
}

bool
ZcrossPath::has_sil(Bface * f)
{

   if ( f->zx_query() )
      return false;
   f->zx_mark();

   int i;
   double g[3];
   Wvec v1;
   for ( i=0; i < 3; i++ ) {
      g[i] = ( _eye - f->v(i+1)->loc() ).normalized() * f->vert_normal(f->v(i+1), v1);
      if ( g[i] == 0 ) {
         //cerr << "g = 0! setting to -1e-8\n";
         g[i] = -1e-8;      // sinful but convenient
      }

   }

   // if a zero crossing is on the face, it crosses two edges
   // so we only need to check from one vertex
   // >=, <= for rare case that a vertex g = 0.0

   if ( ( g[0] > 0 && ( g[1] < 0 || g[2] < 0 ) ) ||
        ( g[0] < 0 && ( g[1] > 0 || g[2] > 0 ) ) )
      return true;

   return false;
}

void
ZcrossPath::build(CBface_list& list )
{
   int k;
   for (k = 0 ; k < list.num(); k++)
      if ( has_sil(list[k]))
         start_sil(list[k]);
}

void
ZcrossPath::start_sil(Bface *f )
{

   /*
    *
    *  the face class accessor to vertices, edges, etc 
    *  f->v() , f->e(), etc. all take a point from 1 to 3
    *  this is a huge pain when you are trying to move around
    *  the triangle, so i store my values , - g , ex1, ex2, 
    *  in the 0 to 2 range 
    *
    */



   /* the call to has_sil returns if mesh has been looked at
    * and ALWAYS marks it as such. be careful..
    *
    *  this should really be called sil_marked(); or something
    */
   if (!has_sil(f))
      return; //f has been zx_checked this frame


   double g[3]; //
   int ex1, ex2 , nex1, nex2; //indices of cross edges, in 0-2;
   //ex1  is vertex 1 of edge 1 ( and index to edge )
   //nex1 is vertex 2 of edge 1
   bool bgrad;

   get_g(f, g, ex1, ex2 ); //calculates g values, and where

   Bface *f1;
   Bface *f2;

   Bface *iter_f; //iterator for mesh traversal;
   nex1 = (ex1+1)%3;
   nex2 = (ex2+1)%3;

   // new iterators for mesh traversal
   Bvert * svrt[3];
   double sg[3];

   //never check across a crease boundary ()
   //it's a discontinuity in the isosurface

   f1 = ( f->e(ex1+1)->is_crease()) ? NULL : f->nbr(ex1+1);
   f2 = ( f->e(ex2+1)->is_crease()) ? NULL : f->nbr(ex2+1);


   //case 1 - we search and close the loop
   //case 2 - we do not close loop -> so search backward, fix

   double alph1 = -g[ex1] / (g[nex1] - g[ex1]);
   Wpt pt1 = interp ( f->v(ex1+1)->loc(), f->v(nex1+1)->loc(), alph1);

   double alph2 = -g[ex2] / (g[nex2] - g[ex2]);
   Wpt pt2 = interp ( f->v(ex2+1)->loc(), f->v(nex2+1)->loc(), alph2);


   // gradient test;
   int gmax = ( g[0] > g[1] ) ? 0 : 1;
   gmax = ( g[2] > g[gmax] ) ? 2 : gmax;

   Wvec v_iso = pt1 - pt2;
   Wvec v_max = f->v(gmax+1)->loc() - pt2;
   Wvec v_grad = v_max.orthogonalized(v_iso);
   bgrad = ( ( _eye - pt2 ) * v_grad  > 0 ) ;

   //we always move clockwise
   // more specifically, if gradient is up, we always
   // check in the same direction

   // you only have to check once

   if ( cross( v_grad , f->norm()) * v_iso > 0 ) {
      swap ( pt1, pt2);
      swap ( alph1, alph2);
      swap ( f1, f2);
      swap ( ex1, ex2 );
      swap ( nex1, nex2);
   }

   //if everything switches, so does bgrad!

   //vert is the front ( according to gradient ) vertex on the list
   //we were going to use it for visibility, but at the moment it's
   //useless.

   // store our own barycentric coordinate

   Wvec bc;
   bc[ex2] = 1.0-alph2;
   bc[nex2] = alph2;


   //start search on f1



   int cstart = num(); // index start of this chain (for case 2 );

   iter_f = f;
   add_seg ( f , pt2, f->v(ex2+1), bgrad, f);

   if (f1) {

      svrt  [0]   = f->v(ex2+1) ;     //initialize cache values
      sg    [0]   = g[ex2];        //for sil_search
      svrt  [1]   = f->v(nex2+1);     //skee-daddle
      sg    [1]   = g[nex2];

      //we start at f, having already added the previous point
      while ( iter_f ) {

         iter_f = sil_walk_search(iter_f, svrt, sg);

         if ( iter_f == f ) {  //closed loop

            if ( !( _segs[cstart].p()  == _segs.last().p() ) ) {
               //cerr << "XXX ZcrossPath::start_sil():: points are not equal" << endl;
               //                 Wpt a = _segs[cstart].p();
               //                 Wpt b = _segs.last().p();
               //cerr << "diff:" << (a-b).length() << endl;
               _segs.last().p() = _segs[cstart].p() ;
            }
            _segs.last().setf( NULL );
            _segs.last().set_end(); 
            _segs.last().set_bary(f2);

            return;
         }

      }
   } else {
      add_seg ( NULL , pt1, f->v(ex1+1), bgrad, f);
   }

   //if we are this far, we have a section of
   //silhouette in the forward direction, ( it may only cross one triangle tho )
   //which ran across a discontinuity

   //now check for the stretch in the opposite direction

   int chain2start = num(); // index of start of next chain

   if ( !f2 )
      return; // first face in reverse direction is NULL, we are done.

   //find second chain, which needs to be reversed
   //starts at pt2;


   iter_f  = f2;
   add_seg ( f2 , pt2, f->v(ex2+1), bgrad, f2);

   svrt[0] = f->v(ex2+1) ;     //initialize cached values
   sg[0]  = g[ex2];  //for sil_search
   svrt[1] = f->v(nex2+1);
   sg[1]  = g[nex2];

   while ( iter_f ) {
      iter_f = sil_walk_search ( iter_f, svrt, sg );
   }
   int chain2end = num();
   // second chain is found in wrong order, so we flip it


   // reversal code


   int chain2num = (chain2end - chain2start) -1 ;
   if ( chain2num < 0 )
      return;

   _segs.insert(cstart, chain2num);
   int last_ind = num()-1;

   int j=0;
   for ( ; j < chain2num ; j++ ) {
      _segs[last_ind-j].setf( _segs[last_ind-(j+1)].f() );      //shift the faces over by one
      //AND any per-face values
      _segs[last_ind-j].setg(_segs[last_ind-(j+1)].g());        //grad associates with face
      _segs[last_ind-j].set_bary(_segs[last_ind-j].f());        //recalc barycentric
   }
   for ( j=0; j < chain2num ; j++) {
      _segs[cstart+j] = _segs.pop();
   }

   _segs.pop(); //last point was the duplicate

   return;
}

Bface*
ZcrossPath::sil_search(Bface *last, Bface * f)
{
   return last;
}

Bface*
ZcrossPath::sil_walk_search ( Bface *f, Bvert * vrt[3], double g[3] )
{

   //this walks along the mesh finding the leading edge of the silhouette

   //previous entry must exist in the seglist
   //"starter" edge with a face equal to this one

   assert ( f == _segs.last().f() );

   f->zx_mark();

   Wvec dummy_vec;

   // we have arrived from the last face, which shares two vertices with this face
   // get the non-cached values and store in the third array index

   vrt[2] = f->other_vertex ( vrt[0], vrt[1] );
   g[2]   = (_eye - vrt[2]->loc()).normalized() * f->vert_normal(vrt[2], dummy_vec);
   if ( g[2] == 0 )
      g[2] = -1e-8;    // sinful but convenient

   // get last point
   Wpt last_pt = last();

   // find crossing edge

   int cross_vrt   = ( g[2] > 0 ) ? ((g[0] < 0)? 0:1) : ((g[0] > 0)? 0:1);
   double alph     = -g[2] / ( g[cross_vrt]-g[2] );
   // interpolate new point on edge

   Wpt new_pt = interp ( vrt[2]->loc(), vrt[cross_vrt]->loc(), alph );

   int gmax = ( g[0] > g[1] ) ? 0 : 1 ;
   gmax = ( g[2] > gmax ) ? 2  : gmax;


   Wvec v_iso ( last_pt - new_pt ) ;
   Wvec v_max = vrt[gmax]->loc() - new_pt;
   Wvec v_grad = v_max.orthogonalized(v_iso);
   bool bgrad = ( (_eye - new_pt) * v_grad > 0 );

   //this gradient is measured for this point and
   //the point before - so update the last point.

   _segs.last().setg(bgrad);

   //

   Bface * next_f = f->opposite_face(vrt[1-cross_vrt]);
   Wvec bc(0,0,0);

   if ( !next_f || f->shared_edge(next_f)->is_crease() ) {
      bc[f->vindex(vrt[2])-1]      = 1.0-alph;
      bc[f->vindex(vrt[cross_vrt])-1] = alph;
      add_seg (NULL  , new_pt, vrt[2], bgrad, f );
      next_f = NULL; //adding a proper null terminator
   } else {
      bc[next_f->vindex(vrt[2])-1]   = 1.0-alph;
      bc[next_f->vindex(vrt[cross_vrt])-1] = alph;
      add_seg (next_f, new_pt, vrt[2], bgrad, next_f); //adding bgrad here is bogus, but we correct
      //on the next seg
   }

   //update the state for the next iteration
   vrt[1-cross_vrt] = vrt[2];
   g[1-cross_vrt] = g[2];
   vrt[2] = NULL;
   g[2] = 0;
   return next_f;
}


Bface*
ZcrossPath::sil_finish(Bface *last, Bface * f)
{
   return f;
}

/* end of file zcross_list.C */
