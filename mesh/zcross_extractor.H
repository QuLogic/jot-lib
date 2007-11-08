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
#ifndef ZCROSS_EXTRACT_H_IS_INCLUDED
#define ZCROSS_EXTRACT_H_IS_INCLUDED

/*!
 *  \file zcross_extractor.H
 *  \brief Contains the definition of the ZCrossExtractor class.  A class for
 *  extracting zero crossing lines of arbitrary scalar fields on a mesh.
 *
 *  \sa zcross_extractor.C
 *
 */

#include <vector>
#include <cassert>
#include <limits>

#include "std/support.H"
#include "mlib/points.H"
#include "mesh/bmesh.H"
#include "mesh/zcross_path.H"

/*!
 *  \brief A function intended to cast from a signed value to an unsigned value
 *  or from an unsigned value to a signed value.
 *
 *  Performs assertions (using the standard assert macro) to check that the value
 *  being cast in within the range of values that can be represented by the type
 *  it is being converted to.
 *
 */
template <typename To, typename From>
inline To
sign_cast(From val)
{
   
   assert(std::numeric_limits<To>::is_specialized);
   
   assert(!std::numeric_limits<To>::is_signed ||
          (val <= static_cast<From>(std::numeric_limits<To>::max())));
   assert(std::numeric_limits<To>::is_signed ||
          (val >= static_cast<From>(std::numeric_limits<To>::min())));
   
   return static_cast<To>(val);
                             
}

/*!
 *  \brief A set of barycentric coordinates on a triangular face.
 *
 */
class BarycentricCoord {
   
 public:
   
   BarycentricCoord(double c0 = 0.0, double c1 = 0.0, double c2 = 0.0)
      { coords[0] = c0; coords[1] = c1; coords[2] = c2; }
      
   double &operator[](int idx) { return coords[idx]; }
   double operator[](int idx) const { return coords[idx]; }
      
   //! \brief Returns the position of Barycentric coordinate on the given
   //! face in world coordinates.
   Wpt to_Wpt(const Bface *f) const
      { return f->v1()->loc()*coords[0] +
           f->v2()->loc()*coords[1] +
           f->v3()->loc()*coords[2]; }
      
   //! \brief Return the Barycentric coordinate store in a Wvec.
   Wvec to_Wvec() const
      { return Wvec(coords[0], coords[1], coords[2]); }
   
 private:
   
   double coords[3];
   
};

/*!
 *  \brief A segment on a zero crossing line.
 *
 *  This segment spans exactly one face on the mesh.
 *
 *  These segments are meant to be kept in a sequence (like an array or vector)
 *  with the \p end_pt of one segment corresponding to the \p start_pt of the
 *  next segment in the sequence.
 *
 */
class ZCrossSeg {
   
 public:
   
   ZCrossSeg(Bface *face_in,
             BarycentricCoord start_bc_in, double start_conf_in,
             BarycentricCoord end_bc_in, double end_conf_in,
             bool end_in = false)
      : face(face_in),
        start_bc(start_bc_in),
        end_bc(end_bc_in),
        start_conf(start_conf_in),
        end_conf(end_conf_in),
        end(end_in)
      { }
      
   ZCrossSeg flip() const {
      return ZCrossSeg(face, end_bc, end_conf, start_bc, start_conf, end);
   }
      
   Bface *get_face() const { return face; }
      
   BarycentricCoord get_start_bc() const { return start_bc; }
   BarycentricCoord get_end_bc() const { return end_bc; }
      
   Wpt get_start_pt() const { return start_bc.to_Wpt(face); }
   Wpt get_end_pt() const { return end_bc.to_Wpt(face); }
      
   double get_start_conf() const { return start_conf; }
   double get_end_conf() const { return end_conf; }
      
   bool is_end() const { return end; }
      
   void set_end(bool end_in = true) { end = end_in; }
   
 private:
   
   //! \brief The face the segment lies on.
   Bface *face;
   //! \brief The endpoints of the segment in barycentric coordinates.
   BarycentricCoord start_bc, end_bc;
   //! \brief The confidences of the endpoints.
   double start_conf, end_conf;
   //! \brief End flag.
   //!
   //! Marks this segment as the end of a contiguous zero crossing line.
   bool end;
   
};

/*!
 *  \brief A class for extracting the zero crossing lines of arbitrary scalar
 *  fields on a mesh.
 *
 *  This class is templated on a series of functors (function objects) that
 *  determine what type of zero crossing lines it extracts, what confidence it
 *  assigns to each extracted point, and how the faces of a mesh (in terms of
 *  ordering and number) are processed to extract the zero crossing lines.  The
 *  interfaces of the three functors can be found in the following abstract base
 *  classes:
 *
 *  \li \c ScalarField - \sa ZCrossScalarFieldInterface
 *  \li \c Confidence - \sa ZCrossConfidenceInterface
 *  \li \c FaceGenerator - \sa ZCrossFaceGeneratorInterface
 *
 */
template <typename ScalarField, typename Confidence, typename FaceGenerator>
class ZCrossExtractor {
   
 public:
   
   //! \name Constructors
   //@{
      
   ZCrossExtractor(const BMESH *mesh_in,
                   ScalarField sfield_in = ScalarField(),
                   Confidence conf_in = Confidence(),
                   FaceGenerator fgen_in = FaceGenerator())
      : mesh(mesh_in), sfield(sfield_in), conf(conf_in), fgen(fgen_in)
      { }
      
   //@}
      
   //! \name Extraction Functions
   //@{
      
   //! \brief Extract the zero crossing lines from the given mesh.
   void extract();
      
   //@}
      
   //! \name Accessors
   //@{
      
   const BMESH *get_mesh() const { return mesh; }
      
   //@}
      
   //! \name Result Accessors
   //@{
      
   //! \brief Access the resulting extracted segments.
   const std::vector<ZXseg> &segs() const
      { return extracted_segs; }
      
   //! \brief Clear the extracted segments.
   void reset()
      { extracted_segs.clear(); }
      
   //@}
      
 private:
   
   bool get_zcross_points(const Bface *f,
                          BarycentricCoord bc_pts[2], int edge_nums[2]);
      
   double get_confidence(const Bface *f, const BarycentricCoord &bc);
      
   bool walk_line(Bface *start_face,
                  Bface *next_face,
                  std::vector<ZCrossSeg> &line);
      
   void extract_line(Bface *f);
      
   void add_seg(const ZCrossSeg &seg);
   
   std::vector<ZXseg> extracted_segs;
   
   std::vector<bool> face_markers;
      
   const BMESH *mesh;
      
   ScalarField sfield;
   Confidence conf;
   FaceGenerator fgen;
   
};

/*!
 *  \brief The interface for the ZCrossExtractor ScalarField functor template
 *  argument.
 *
 *  This functor should take a single \c const \c Bvert* as an argument and return
 *  a \c double that is the value of the scalar field that the zero crossing lines
 *  are being extracted from at the given vertex.
 *
 */
class ZCrossScalarFieldInterface {
   
 public:
   
   virtual ~ZCrossScalarFieldInterface() {}
   virtual double operator()(const Bvert*) = 0;
   
};

/*!
 *  \brief The interface for the ZCrossExtractor Confidence functor template
 *  argument.
 *
 *  This functor should take two arguments:  a \c const \c Bface* and a \c const
 *  \c BarycentricCoord&.  It should return a double between 0.0 and 1.0 that is
 *  the confidence of the point on the given face at the given Barycentric
 *  coordinates.
 *
 */
class ZCrossConfidenceInterface {
   
 public:
   
   virtual ~ZCrossConfidenceInterface() {}
   virtual double operator()(const Bvert*) = 0;
   
};

/*!
 *  \brief The interface for the ZCrossExtractor FaceGenerator functor template
 *  argument.
 *
 *  This functor should have two overloaded forms.
 *  
 *  The first form should take a single \c const \c ZCrossExtractor* as an
 *  argument and should have a \c void return type.  This form should reset the
 *  generator and perform any initialization needed (specifically using
 *  information available through the \c ZCrossExtractor*).
 *
 *  The second form should take no arguments and return an \c int.  The \c int is
 *  the index of the face that the \c ZCrossExtractor should examine next to look
 *  for zero crossing lines (the index is into the \c ZCrossExtractor's mesh's
 *  list of faces).  When the \c ZCrossExtractor should stop looking for zero
 *  crossing lines, a value of -1 should be returned.
 *
 */
class ZCrossFaceGeneratorInterface {
   
 public:
   
   virtual ~ZCrossFaceGeneratorInterface() {}

   template <typename ScalarField, typename Confidence, typename FaceGenerator>
   void operator()(const ZCrossExtractor<ScalarField,Confidence,FaceGenerator>*)
      { }
   virtual int operator()() = 0;
   
};

/*!
 *  \brief A simple FaceGenerator for the ZCrossExtractor class that generates
 *  all the faces on the mesh being processed.
 *
 */
class ZCrossAllFaceGenerator : public ZCrossFaceGeneratorInterface {
   
 public:
   
   ZCrossAllFaceGenerator()
      : i(0), num_faces(0)
      { }
   
   virtual ~ZCrossAllFaceGenerator() {}
   template <typename ScalarField, typename Confidence, typename FaceGenerator>
   void operator()(const ZCrossExtractor<ScalarField,Confidence,FaceGenerator> *extractor)
      { i = 0; num_faces = extractor->get_mesh()->nfaces(); }
      
   virtual int operator()()
      { return (i < num_faces) ? i++ : -1; }
      
 private:
   
   int i, num_faces;
   
};

/*!
 *  \brief A FaceGenerator for the ZCrossExtractor class that generates random
 *  faces on the mesh being processed.
 *
 */
class ZCrossRandFaceGenerator : public ZCrossFaceGeneratorInterface {
   
 public:
   
   ZCrossRandFaceGenerator()
      : i(0), num_faces(0)
      { }
   
   virtual ~ZCrossRandFaceGenerator() {}
   template <typename ScalarField, typename Confidence, typename FaceGenerator>
   void operator()(const ZCrossExtractor<ScalarField,Confidence,FaceGenerator> *extractor)
      { i = 0; total_faces = extractor->get_mesh()->nfaces();
      num_faces = compute_num_faces(total_faces); }
      
   virtual int operator()()
      { return (i < num_faces) ? i++, gen_rand_face() : -1; }
   
 private:
   
   int compute_num_faces(int n)
      { return static_cast<int>(sqrt(4.0 * n)); }
      
   int gen_rand_face()
      { return gen_rand_val() % total_faces; }
      
   int gen_rand_val()
      { return static_cast<int>(drand48() * total_faces); }
   
   int i, num_faces, total_faces;
   
};

/*!
 *  \brief A FaceGenerator for the ZCrossExtractor class that generates the faces
 *  that were extracted in the previous extraction (or all faces in none have
 *  been extracted).
 *
 */
class ZCrossPreviousFaceGenerator : public ZCrossFaceGeneratorInterface {
   
 public:
   
   virtual ~ZCrossPreviousFaceGenerator() {}
   ZCrossPreviousFaceGenerator();
   
   template <typename ScalarField, typename Confidence, typename FaceGenerator>
   void operator()(const ZCrossExtractor<ScalarField,Confidence,FaceGenerator> *extractor)
      { all_faces(extractor); init(extractor->segs()); }
      
   virtual int operator()()
      { return gen_face(this); }
   
 private:
   
   void init(const std::vector<ZXseg> &segs);
   
   static int gen_all_faces(ZCrossPreviousFaceGenerator *self)
      { return self->all_faces(); }
      
   static int gen_prev_faces(ZCrossPreviousFaceGenerator *self);
      
   int (*gen_face)(ZCrossPreviousFaceGenerator *);
   
   ZCrossAllFaceGenerator all_faces;
      
   std::vector<int> prev_faces;
   
};

/*!
 *  \brief A FaceGenerator for the ZCrossExtractor class that generates previous
 *  faces (as per the ZCrossPreviousFaceGenerator) followed by random faces (as
 *  per the ZCrossRandFaceGenerator).
 *
 */
class ZCrossPreviousAndRandFaceGenerator : public ZCrossFaceGeneratorInterface {
   
 public:
   
   virtual ~ZCrossPreviousAndRandFaceGenerator() {}
   ZCrossPreviousAndRandFaceGenerator()
      { }
   
   template <typename ScalarField, typename Confidence, typename FaceGenerator>
   void operator()(const ZCrossExtractor<ScalarField,Confidence,FaceGenerator> *extractor)
      { prev_faces(extractor); rand_faces(extractor); }
      
   virtual int operator()()
      { int prev_faces_val = prev_faces();
      return prev_faces_val != -1 ? prev_faces_val : rand_faces(); }
   
 private:
   
   ZCrossPreviousFaceGenerator prev_faces;
   ZCrossRandFaceGenerator rand_faces;
   
};

//----------------------------------------------------------------------------//

template <typename ScalarField, typename Confidence, typename FaceGenerator>
void
ZCrossExtractor<ScalarField, Confidence, FaceGenerator>::extract()
{
   
   // Reset the FaceGenerator
   fgen(this);
   
   // Reset the face markers:
   assert(face_markers.size() == 0);
   face_markers.resize(mesh->nfaces(), false);
   
   // Clear out previously extracted lines:
   reset();
   
   int face_idx;
   
   while((face_idx = fgen()) != -1){
      
      if(face_markers[face_idx]) continue;
      
      face_markers[face_idx] = true;
      
      extract_line(mesh->bf(face_idx));
      
   }
   
   face_markers.clear();
   
}

template <typename ScalarField, typename Confidence, typename FaceGenerator>
bool
ZCrossExtractor<ScalarField,
                Confidence,
                FaceGenerator>::get_zcross_points(const Bface *f,
                                                  BarycentricCoord bc_pts[2],
                                                  int edge_nums[2])
{
   
   double sf_vals[3];
   
   // Get scalar field values for all vertices on this face:
   for(int i = 0; i < 3; ++i){
      
      sf_vals[i] = sfield(f->v(i + 1));
      
      // Perturb scalar field slightly so that we don't have any zeros exactly
      // on vertices:
      sf_vals[i] = (sf_vals[i] == 0.0) ? 1.0e-8 : sf_vals[i];
      
   }
   
   bool has_zcrossing = false;
   
   int cur_crossing_idx = 0;
   
   // Find zero crossing in Barycentric coordinates:
   for(int i = 0; i < 3; ++i){
      
      if(((sf_vals[i] < 0.0) && (sf_vals[(i + 1) % 3] > 0.0)) ||
         ((sf_vals[i] > 0.0) && (sf_vals[(i + 1) % 3] < 0.0))){
         
         has_zcrossing = true;
         
         double sf_val_diff = sf_vals[i] - sf_vals[(i + 1) % 3];
         
         // Should never have more than two edges with zero crossings:
         assert((cur_crossing_idx >= 0) && (cur_crossing_idx < 2));
         
         bc_pts[cur_crossing_idx][i] = sf_vals[i] / sf_val_diff;
         bc_pts[cur_crossing_idx][(i + 1) % 3] = sf_vals[(i + 1) % 3] / -sf_val_diff;
         bc_pts[cur_crossing_idx][(i + 2) % 3] = 0.0;
         
         edge_nums[cur_crossing_idx] = i;
         
         // Make sure the Barycentric coords add up to 1.0:
         assert(isEqual(bc_pts[cur_crossing_idx][0] +
                              bc_pts[cur_crossing_idx][1] +
                              bc_pts[cur_crossing_idx][2],
                              1.0));
         
         // Make sure none of the Barycentric coords are negative:
         assert((bc_pts[cur_crossing_idx][0] >= 0.0) &&
                (bc_pts[cur_crossing_idx][1] >= 0.0) &&
                (bc_pts[cur_crossing_idx][2] >= 0.0));
         
         ++cur_crossing_idx;
            
      }
      
   }
   
   // If this has face has a zero crossing, there must be exactly two edges that
   // contain the crossing.  So, two Barycentric points should have been computed.
   assert(!has_zcrossing || (cur_crossing_idx == 2));
   
   return has_zcrossing;
   
}

template <typename ScalarField, typename Confidence, typename FaceGenerator>
double
ZCrossExtractor<ScalarField,
                Confidence,
                FaceGenerator>::get_confidence(const Bface *f,
                                               const BarycentricCoord &bc)
{
   
   double total_conf = 0.0;
   
   for(int i = 0; i < 3; ++i){
      
      total_conf += (conf(f->v(i + 1)) * bc[i]);
      
   }
   
   return total_conf;
   
}

template <typename ScalarField, typename Confidence, typename FaceGenerator>
bool
ZCrossExtractor<ScalarField,
                Confidence,
                FaceGenerator>::walk_line(Bface *start_face,
                                          Bface *next_face,
                                          std::vector<ZCrossSeg> &line)
{
   
   // Make sure the starting face pointer is non-NULL:
   assert(start_face != 0);
   
   // A NULL next face indicates that we're at a boundary.  Don't extract any
   // segments in this case and say that the loop wasn't completed:
   if(next_face == 0)
      return false;
   
   // Make sure starting face and next face share an edge:
   assert(start_face->shared_edge(next_face) != 0);
   
   Bface *prev_face = start_face;
   Bface *cur_face = next_face;
   
   BarycentricCoord zcross_pts[2];
   int zcross_edges[2];
   bool loop_complete = false;
   
#ifndef NDEBUG

   long face_count = 0;

#endif // NDEBUG
   
   while(true){
      
#ifndef NDEBUG

      ++face_count;
      
      // Make sure the number of faces processed doesn't exceed the number of
      // faces in the mesh:
      assert(start_face->mesh());      
      assert(face_count < start_face->mesh()->nfaces());

#endif // NDEBUG
      
      // This face shouldn't have been seen before:
      assert(face_markers[cur_face->index()] == false);
      
      face_markers[cur_face->index()] = true;
      
      // This should never fail:
      if(!get_zcross_points(cur_face, zcross_pts, zcross_edges))
         break;
      
      int next_pt = (cur_face->nbr(zcross_edges[0] + 1) == prev_face) ? 1 : 0;
      
      next_face = cur_face->nbr(zcross_edges[next_pt] + 1);
      
      line.push_back(ZCrossSeg(cur_face,
                               zcross_pts[(next_pt + 1) % 2],
                               get_confidence(cur_face, zcross_pts[(next_pt + 1) % 2]),
                               zcross_pts[next_pt],
                               get_confidence(cur_face, zcross_pts[next_pt])));
      
      // See if we are at a boundary edge:
      if(next_face == 0)
         break;
      
      // See if we are back at the beginning (i.e. the loop is complete):
      if(next_face == start_face){
         
         loop_complete = true;
         break;
         
      }
      
      prev_face = cur_face;
      cur_face = next_face;
      
   };
   
   return loop_complete;
   
}

template <typename ScalarField, typename Confidence, typename FaceGenerator>
void
ZCrossExtractor<ScalarField,
                Confidence,
                FaceGenerator>::extract_line(Bface *f)
{
   
   // Make sure the face pointer is non-NULL:
   assert(f != 0);
   
   // Make sure the face has a non-NULL mesh pointer:
   assert(f->mesh() != 0);
   
   BarycentricCoord f_zcross_pts[2];
   int f_zcross_edges[2];
   
   if(!get_zcross_points(f, f_zcross_pts, f_zcross_edges))
      return;
   
   int starting_pt = 0;
   
   assert(starting_pt != -1);
   
   std::vector<ZCrossSeg> zcross_line_part;
   
   if(!walk_line(f, f->nbr(f_zcross_edges[starting_pt] + 1), zcross_line_part)){
      
      // Wasn't able to complete the zero crossing loop, need to walk the other
      // end of the line as well...
      
      vector<ZCrossSeg> zcross_line_other_part;
      
      walk_line(f, f->nbr(f_zcross_edges[(starting_pt + 1) % 2] + 1),
                zcross_line_other_part);
      
      // Make sure number of line segments is less than the number of faces on
      // the mesh:
      assert(zcross_line_other_part.size() < sign_cast<unsigned long>(f->mesh()->nfaces()));
      
      // Add all of these segments in reverse order so they 
      for(long i = (sign_cast<long>(zcross_line_other_part.size()) - 1); i >= 0; --i){
         
         // Make sure end point of previous segment equals start point of this segment:
         assert((i == (sign_cast<long>(zcross_line_other_part.size()) - 1)) || // Don't check if i == index of first segment
                (zcross_line_other_part[i + 1].flip().get_end_pt()
                 == zcross_line_other_part[i].flip().get_start_pt()));
      
         // Make sure end point of this segment equals start point of next segment:
         assert((i == 0) || // Don't check if i == index of last segment
                (zcross_line_other_part[i].flip().get_end_pt()
                 == zcross_line_other_part[i - 1].flip().get_start_pt()));
         
         add_seg(zcross_line_other_part[i].flip());
         
      }
      
   }
   
   // Make sure number of line segments is less than the number of faces on
   // the mesh:
   assert(sign_cast<long>(zcross_line_part.size()) < f->mesh()->nfaces());
   
   bool start_is_end = (zcross_line_part.size() == 0);
   
   if(!start_is_end)
      zcross_line_part.back().set_end();
   
   add_seg(ZCrossSeg(f,
                     f_zcross_pts[(starting_pt + 1) % 2],
                     get_confidence(f, f_zcross_pts[(starting_pt + 1) % 2]),
                     f_zcross_pts[starting_pt],
                     get_confidence(f, f_zcross_pts[starting_pt]),
                     start_is_end));
   
   for(unsigned i = 0; i < zcross_line_part.size(); ++i){
      
      // Make sure end point of previous segment equals start point of this segment:
      assert((i == 0) || // Don't check if i == 0
             (zcross_line_part[i - 1].get_end_pt() == zcross_line_part[i].get_start_pt()));
   
      // Make sure end point of this segment equals start point of next segment:
      assert((i == (zcross_line_part.size() - 1)) || // Don't check if i == index of last segment
             (zcross_line_part[i].get_end_pt() == zcross_line_part[i + 1].get_start_pt()));
      
      add_seg(zcross_line_part[i]);
      
   }
   
}

template <typename ScalarField, typename Confidence, typename FaceGenerator>
inline void
ZCrossExtractor<ScalarField,
                Confidence,
                FaceGenerator>::add_seg(const ZCrossSeg &seg)
{
   
//    const double SC_THRESH = 0.05;
//    
//    if((seg.get_start_conf() <= SC_THRESH) &&
//       (extracted_segs.size() > 0) && (extracted_segs.back().end())){
//       
//       return;
//       
//    }
   
   // Make sure at least one of the Barycentric coords is zero:
   assert(isZero(seg.get_start_bc()[0]) ||
          isZero(seg.get_start_bc()[1]) ||
          isZero(seg.get_start_bc()[2]));
   
   // Make sure at least two of the Barycentric coords are not zero:
   assert(((seg.get_start_bc()[0] != 0.0) && (seg.get_start_bc()[1] != 0.0)) ||
          ((seg.get_start_bc()[1] != 0.0) && (seg.get_start_bc()[2] != 0.0)) ||
          ((seg.get_start_bc()[2] != 0.0) && (seg.get_start_bc()[0] != 0.0)));
   
   int vert_index = (seg.get_start_bc()[0] == 0.0) ? 1 :
      (seg.get_start_bc()[1] == 0.0) ? 2 :
      0; // assume seg.get_start_pt()[2] == 0.0 here
   
   Bface *f = seg.get_face();
   
   extracted_segs.push_back(
      ZXseg(f,                            // Face
            seg.get_start_pt(),           // World space point
            f->v(vert_index + 1),         // Vertex (what's this for?)
            seg.get_start_conf(),         // Confidence
            seg.get_start_bc().to_Wvec(), // Barycentric coords
            STYPE_SUGLINE,                // Type
            false)                        // End flag
      );
   
   if(seg.is_end() /* || (seg.get_end_conf() <= SC_THRESH) */ ){
   
      extracted_segs.push_back(
         ZXseg(0,                            // Face
               seg.get_end_pt(),             // World space point
               f->v(vert_index + 1),         // Vertex (what's this for?)
               seg.get_end_conf(),           // Confidence
               seg.get_end_bc().to_Wvec(),   // Barycentric coords
               STYPE_SUGLINE,                // Type
               true)                         // End flag
         );
         
   }
   
}

#endif // ZCROSS_EXTRACT_H_IS_INCLUDED
