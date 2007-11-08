// imge line shader fragment program.
// This is a simple version which draws only dark lines and doesn't support
// level of abstraction with depth

uniform sampler2D tone_map; // tone map
uniform float x_1, y_1; // scale for uv computation
uniform float half_width; // half of line width
uniform float upper_curv_thd, lower_curv_thd; // curvature thresholds
uniform int curv_opacity_ctrl; // 1: apply opacity control with curvature 
uniform int dist_opacity_ctrl; // 1: apply opacity control with distance
uniform int tone_opacity_ctrl; // 1: apply opacity control with tone 
uniform float moving_factor;  // control the amount of moving distance

// X = [x_i^2 2x_i*y_i y_i^2 x_i y_1 1]
// XA = T, A: unknown quadric coefficients, T: height values of samples
// H = (X^T X)^(-1)X^T, 6x9 matrix
const float H0 =  0.1666667, H1 = -0.3333333, H2 =  0.1666667; 
const float H3 =  0.1666667, H4 = -0.3333333, H5 =  0.1666667; 
const float H6 =  0.1666667, H7 = -0.3333333, H8 =  0.1666667; 
const float H9 =  0.25, H10 = 0.0, H11 = -0.25, H12 = 0.0; 
const float H13 = 0.0, H14 = 0.0, H15 =-0.25, H16 = 0.0; 
const float H17 = 0.25, H18 =  0.1666667, H19 = 0.1666667; 
const float H20 =  0.1666667, H21 = -0.3333333, H22 = -0.3333333; 
const float H23 = -0.3333333, H24 =  0.1666667, H25 =  0.1666667; 
const float H26 =  0.1666667, H27 = -0.1666667, H28 =  0.0; 
const float H29 =  0.1666667, H30 = -0.1666667, H31 =  0.0; 
const float H32 =  0.1666667, H33 = -0.1666667, H34 =  0.0; 
const float H35 =  0.1666667, H36 = -0.1666667, H37 = -0.1666667; 
const float H38 = -0.1666667, H39 =  0.0, H40 = 0.0, H41 = 0.0; 
const float H42 =  0.1666667, H43 =  0.1666667, H44 = 0.1666667; 
const float H45 = -0.1111111, H46 =  0.2222222, H47 = -0.1111111; 
const float H48 =  0.2222222, H49 =  0.5555556, H50 =  0.2222222; 
const float H51 = -0.1111111, H52 =  0.2222222, H53 = -0.1111111; 

const float M_PI = 3.141592654;
const int max_iter = 5;
const float max_dist = 100.0;

//
// by using least squares of degree-2 polynomial, we analytically compute 
// maximum curvature (max_curv), its principal axis (axis), 
// and the distance from the pixel center (uv) to the axis (axis_dist), 
// where the axis goes through the quadric center (x0, y0).
//
// We use a local parameterization so that (0, 0) corresponds to the pixel
// center. 
//

void 
compute_ridge_properties(in vec2 uv, out float max_curv, out float axis_dist, out vec2 axis)
{

   // quadric form ax^2 + 2bxy + cy^2 + 2dx + 2ey + f
   float a, b, c, d, e, f, l1, l2;
   
   float T[9]; // height values of 9 sample points

   int i, j;
   float m, n;
   vec2 neigh_uv, diff_uv;
   vec2 scale = vec2(x_1, y_1), diff;

   m = -half_width;

   for(i = 0; i < 3; i++){ // set 3x3 matrix
      n = -half_width;

      for(j = 0; j < 3; j++){

	 diff_uv = vec2(n, m) * scale;
	 neigh_uv = uv + diff_uv;

         // get tone at given pixel location via tone reference image
	 T[i*3+j] = texture2D(tone_map, neigh_uv).r;
	 n += half_width;

      }
      m += half_width;
   }

   // A = H * T, A = [ a 2*b c 2*d 2*e f ]^T
   //
   a = (H0*T[0]) + (H1*T[1]) + (H2*T[2]) + (H3*T[3]) + (H4*T[4]) + (H5*T[5]) 
	    + (H6*T[6]) + (H7*T[7]) + (H8*T[8]);
   b = 0.5 * ((H9*T[0]) + (H10*T[1]) + (H11*T[2]) + (H12*T[3]) + (H13*T[4]) 
	    + (H14*T[5]) + (H15*T[6]) + (H16*T[7]) + (H17*T[8]));
   c = (H18*T[0]) + (H19*T[1]) + (H20*T[2]) + (H21*T[3]) + (H22*T[4]) 
	    + (H23*T[5]) + (H24*T[6]) + (H25*T[7]) + (H26*T[8]);
   d = 0.5 * ((H27*T[0]) + (H28*T[1]) + (H29*T[2]) + (H30*T[3]) + (H31*T[4]) 
	    + (H32*T[5]) + (H33*T[6]) + (H34*T[7]) + (H35*T[8]));
   e = 0.5 * ((H36*T[0]) + (H37*T[1]) + (H38*T[2]) + (H39*T[3]) + (H40*T[4]) 
	    + (H41*T[5]) + (H42*T[6]) + (H43*T[7]) + (H44*T[8]));
   f = (H45*T[0]) + (H46*T[1]) + (H47*T[2]) + (H48*T[3]) + (H49*T[4]) 
	    + (H50*T[5]) + (H51*T[6]) + (H52*T[7]) + (H53*T[8]);

   // 2x2 symmetric matrix M used in the quadratic form:
   //   a b 
   //   b c 

   // compute eigenvalues of M 
   float sqrt_term = sqrt((a-c)*(a-c) + 4.0*(b*b));
   l1 = (a+c + sqrt_term)*0.5;
   l2 = (a+c - sqrt_term)*0.5;

   // max_curv: maximum curvature
   if(abs(l1) >= abs(l2)){
      max_curv = l1;
   }
   else{
      max_curv = l2;
   }

   // eigen vector corresponding
   // to larger magnitude of eigen value
   // axis: axis perpendicular to line

   if(abs(b) > 1.0e-8){ // ax + by = l * x
      if( abs(l1) >= abs(l2))
	 axis.y = -(a-l1)/b;
      else
	 axis.y = -(a-l2)/b;

      axis.x = 1.0;
   }
   else{ // b = 0
      if(abs(a) > abs(b)){ 
	 axis.x = 1.0;
	 axis.y = 0.0;   
      }
      else{
	 axis.x = 0.0;
	 axis.y = 1.0;
      }
   }

   float len = length(axis);

   axis /= len; // normalize the eigen vector

   // compute the center (x0, y0) of quadric function
   // quadric form ax^2 + 2bxy + cy^2 + 2dx + 2ey + f
   // put x = x' + x0, y = y' + y0
   // x' term and y' term should be 0
   // a*x0 + b*y0 +d = 0 (1)
   // b*x0 + c*y0 +e = 0 (2)

   // solve (1) and (2)
   float invS[4], x0, y0;

   float det = a*c-b*b;

   if(abs(det) > 1.0e-15){ // unique solution
      invS[0] = c/det;  // inverse of matrix M 
      invS[1] = -b/det; //            
      invS[2] = -b/det;
      invS[3] = a/det;

      x0 = -(invS[0]*d+invS[1]*e);
      y0 = -(invS[2]*d+invS[3]*e);

      // axis_dist: distance from the pixel center (0, 0) to the axis
      axis_dist = axis.x*x0 + axis.y*y0;
   }
   else if(abs(b*d - e*a) < 1.0e-15){
      axis_dist = -2.0 * (d * e)/max_curv;
   }
   else{
      axis_dist = max_dist;
   }

}


//
// We can vary line opacity with maximum curvature, the distance to the
// ridge line, and tone value of the center pixel
//

float compute_line_opacity(in float center_val, in float max_curv, 
		      in float dist, in float upper_c, in float lower_c)
{
   float val = 1.0;

   if(curv_opacity_ctrl == 1){
      val *= clamp((abs(max_curv)-lower_c)/(upper_c-lower_c), 0.0, 1.0);
   }
   else if(abs(max_curv) < upper_c){
      val = 0.0;
   }

   if(dist_opacity_ctrl == 1){
      float a = sqrt(2.0 - clamp(dist, 0.0, 2.0))*0.5;

      if(a > 0.6)
	 a = 1.0;

      val *= a;
   }

   if(tone_opacity_ctrl == 1){
       val *= clamp(1.0-center_val, 0.0, 1.0);
   }

   return val;
} 

//
// If a pixel belongs to a line stroke, we output black color with appropriate
// opacity. Otherwise, we output color with 0 opacity.
//
vec4 draw_line_pixel(in vec2 center_pix)
{
   vec2 scale = vec2(x_1, y_1), diff;
   vec2 uv = center_pix * scale, pix2;
   vec4 col;         // output color
   int k;

   bool is_line = true;
   float max_curv, axis_dist;
   vec2 axis;

   float upper_c, lower_c; // upper_c: upper bound lower_c: lower bound

   upper_c = upper_curv_thd * half_width;
   lower_c = lower_curv_thd * half_width;

   pix2 = center_pix;

   // # 1
   // ridge searching
   for(k = 0; k < max_iter; k++){ // move to the axis...

      compute_ridge_properties(uv, max_curv, axis_dist, axis);

      // too low curvature 
      if(abs(max_curv) <= lower_c){
	 col.a = 0.0;
	 is_line = false;
	 break;
      }
      // move a pixel whose curvature is larger than a lower bound 
      else{
	 diff = (axis_dist * axis * half_width) * moving_factor;
	 pix2 = pix2 + diff;

	 uv = pix2 * scale;
      }

   }

   if(is_line == true){ 
      // dark line
      if(max_curv > 0.0){
         col.rgb = vec3(0.0, 0.0, 0.0);

	 // normalized distance
         float dist = length(center_pix - pix2)/half_width; 
	 // center tone value
         float center_val = texture2D(tone_map, uv).r;

	 // compute line opacity
         col.a = compute_line_opacity(center_val, max_curv, dist, 
			                                 upper_c, lower_c);
      }
      else{
      // highlight line
      // Here, we can draw highlight lines from a diffuse tone image 
      // if a opacity is set appropriately.
         col.a = 0.0;
      }
   }

   // # 2
   // We can draw highlight lines from a specular tone image
   // if we do the same procedure as #1 for the specular tone image

   return col;
}

void main() 
{
   gl_FragColor = draw_line_pixel(gl_FragCoord.xy);
}

// img_line.fp
