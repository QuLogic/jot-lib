// imge line shader fragment program.

uniform sampler2D toon_tex; 
uniform sampler2D id_map; 
uniform mat4 P_matrix;

varying vec4 P; // vertex position, eye space
varying float local_e_l;
uniform float global_edge_len;

uniform int detail_func; // control line width according to depth or user factor
uniform float unit_len, ratio_scale, edge_len_scale; // parameters for depth computation
uniform float user_depth; // user depth for detail control

uniform sampler2D tone_map;
uniform float x_1, y_1; // scale for uv computation
uniform float line_width;
uniform float curv_thd0, curv_thd1; // curvature, distance, gradient thresholds
uniform vec3  dk_color;     // line color in dark regions
uniform vec3  ht_color;     // line color in highlight regions
uniform vec3  lt_color;     // line color in light regions
uniform int   debug; // debug drawing...
uniform int line_mode; // 0: None, 1: All, 2: Dark, 3: Light, 4: HighLight,
                       // 5: DK + LT, 6: DK + HT, 7: LT + HT
uniform int confidence_mode; // 1: apply confidence with curvature to alpha
uniform int silhouette_mode; // 1: apply attenuation around silhouettes
uniform int draw_silhouette;
uniform float alpha_offset; // add some constant to all alpha
uniform int enable_feature;
uniform float highlight_control; // ratio to curvature thresholds of dk lines
uniform float light_control;   // ratio to curvature thresholds of dk lines
uniform int tapering_mode;    // tapering with distance
uniform float moving_factor;  // control the amount of moving distance
uniform int tone_effect;      // apply tone to alpha
uniform float ht_width_control; // control line width when searching highlights


// H 6x9
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


float depth(float maxFactor,float targetLength, float z)
{
   float length = (1000.0 * targetLength) + (1.0 * (1.0 - targetLength));
   return log(z/length) / log(maxFactor);
}

void 
compute_quadric_coefficients(in vec2 uv, in int channel, in float half_width, out float max_curv, out float axis_dist, out vec2 axis)
{

   float T[6]; // quadric coefficients
   float A[9]; // height values

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

	 A[i*3+j] = texture2D(tone_map, neigh_uv)[channel];
	 n += half_width;

      }
      m += half_width;
   }


   // H * A
   T[0] = (H0*A[0]) + (H1*A[1]) + (H2*A[2]) + (H3*A[3]) + (H4*A[4]) + (H5*A[5]) 
	    + (H6*A[6]) + (H7*A[7]) + (H8*A[8]);
   T[1] = (H9*A[0]) + (H10*A[1]) + (H11*A[2]) + (H12*A[3]) + (H13*A[4]) 
	    + (H14*A[5]) + (H15*A[6]) + (H16*A[7]) + (H17*A[8]);
   T[2] = (H18*A[0]) + (H19*A[1]) + (H20*A[2]) + (H21*A[3]) + (H22*A[4]) 
	    + (H23*A[5]) + (H24*A[6]) + (H25*A[7]) + (H26*A[8]);
   T[3] = (H27*A[0]) + (H28*A[1]) + (H29*A[2]) + (H30*A[3]) + (H31*A[4]) 
	    + (H32*A[5]) + (H33*A[6]) + (H34*A[7]) + (H35*A[8]);
   T[4] = (H36*A[0]) + (H37*A[1]) + (H38*A[2]) + (H39*A[3]) + (H40*A[4]) 
	    + (H41*A[5]) + (H42*A[6]) + (H43*A[7]) + (H44*A[8]);
   T[5] = (H45*A[0]) + (H46*A[1]) + (H47*A[2]) + (H48*A[3]) + (H49*A[4]) 
	    + (H50*A[5]) + (H51*A[6]) + (H52*A[7]) + (H53*A[8]);

   float a, b, c, d, e, f, l1, l2;
   // quadric form ax^2 + 2bxy + cy^2 + 2dx + 2ey + f
   
   a = T[0];
   b = 0.5*T[1];
   c = T[2];
   d = 0.5*T[3];  
   e = 0.5*T[4];
   f = T[5];

   // 2x2 symmetric matrix used in the quadratic form:
   //   a b 
   //   b c 

   // compute eigenvalues 
   // can optimize:

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

   if(abs(b) <= 1.0e-8){           // eigen vector corresponding 
                           // to larger magnitude of eigen value
      if(abs(a) > abs(b)){ // axis: axis perpendicular to line
	 axis.x = 1.0;
	 axis.y = 0.0;   
      }
      else{
	 axis.x = 0.0;
	 axis.y = 1.0;
      }
   }
   else{
      if( abs(l1) >= abs(l2))
	 axis.y = -(a-l1)/b;
      else
	 axis.y = -(a-l2)/b;

      axis.x = 1.0;
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
   float t[2], invS[4], x0, y0;

   t[0] = -d;
   t[1] = -e;

   float det = a*c-b*b;

   if(abs(det) > 1.0e-15){ // unique solution
      invS[0] = c/det;
      invS[1] = -b/det;
      invS[2] = -b/det;
      invS[3] = a/det;

      x0 = invS[0]*t[0]+invS[1]*t[1];
      y0 = invS[2]*t[0]+invS[3]*t[1];


      // axis_dist: distance to the axis
      axis_dist = axis.x*x0 + axis.y*y0;
   }
   else if(abs(b*d - e*a) < 1.0e-15){
      axis_dist = abs(d)/sqrt(a*a + b*b);
   }
   else{
      axis_dist = max_dist;
   }

   // Gx = 2ax + 2by + d 
   // Gy = 2bx + 2cy + e
   // at (0, 0), G = (d, e)
   //grad = vec2(T[3], T[4]);

}

float edge_length_in_pix(in float e_l)
{
   vec4 v0 = vec4(0.0, -e_l, length(P), 1.0); 
   vec4 v1 = vec4(0.0, e_l, length(P), 1.0);

   vec4 p0, p1;

   p0 = P_matrix * v0;
   p1 = P_matrix * v1;

   return length(p0/p0.w-p1/p1.w);
}

vec4 
debug_color(bool is_line, float max_curv, float axis_dist, vec2 axis)
{
   vec4 col;

   if(debug == 1){ // sign change
      if(max_curv > 0.0){
	 col = vec4(1.0, 0.0, 0.0, 1.0);
      }
      else{
	 col = vec4(0.0, 1.0, 0.0, 1.0);
      }
   }
   if(debug == 2){ // pixel movement
      if(!is_line){
	 if(max_curv > 0.0) 
	    col = vec4(1.0, 1.0, 0.0, 1.0);
	 else 
	    col = vec4(0.0, 1.0, 1.0, 1.0);
      }
      else{
      if(max_curv > 0.0)
	 col = vec4(1.0, 0.0, 0.0, 1.0);
      else
	 col = vec4(0.0, 1.0, 0.0, 1.0);
      }
   }
   if(debug == 3){ // curvature
      if(max_curv > 0.0)
	 col = vec4(max_curv*10.0, 0.0, 0.0, 1.0);
      if(max_curv <= 0.0)
	 col = vec4(0.0, -max_curv*10.0, 0.0, 1.0);
   }
   if(debug == 4){ // distance
//      float val = abs(axis_dist*max_curv)*10.0;
      float val = 1.0/sqrt((abs(axis_dist)+0.1))*0.5;
      if(axis_dist == max_dist){
	 col = vec4(0.0, 0.0, 1.0, 1.0);
      }
      else if(abs(max_curv) < 0.0015){
      col = vec4(0.0, 0.0, 0.0, 1.0);
      }
      else if(max_curv > 0.0)
      col = vec4(val, 0.0, 0.0, 1.0);
      else
      col = vec4(0.0, val, 0.0, 1.0);
   }
/*   if(debug == 5){ // gradient magnitude
      float len = (1.0-length(grad)*10.0);

      col = vec4(len, len, len, 1.0);
   }*/

   return col;
}

float compute_line_width()
{
   float factor;
   float half_width;

   if(detail_func == 0){
      half_width = line_width;
   }
   if(detail_func == 1){
      factor = edge_length_in_pix(unit_len)*edge_len_scale;
      half_width = line_width*factor;
   }
   if(detail_func == 2){
      factor = edge_length_in_pix(global_edge_len)*edge_len_scale;
      half_width = line_width*factor;
   }
   if(detail_func == 3){
      factor = edge_length_in_pix(local_e_l)*edge_len_scale;
      half_width = line_width*factor;
   }
   if(detail_func == 4){
      float ratio = local_e_l/global_edge_len;
      if(ratio > 1.0){
	 factor = 1.0 + log(ratio*ratio_scale + 1.0);
      }
      else{
	 factor = 1.0 - log(2.0 - ratio*ratio_scale);
      }
      half_width = line_width*factor;
   }
   if(detail_func == 5){
      half_width = line_width*(1.0-user_depth);
   }

/*   if(enable_feature == 1){
      float t = edge_length_in_pix(local_e_l);
      half_width *= t * 0.1;
   }*/

   half_width = max(half_width, 1.0);

   return half_width;
}

int is_silhouette(in vec2 uv)
{
   int i, j;
   vec2 scale = vec2(x_1, y_1), neigh_uv;
   int sil_cnt = 0;
   vec4 id, center_id = texture2D(id_map, uv);

   for(i = -1; i <= 1; i++){ // set 3x3 matrix

      for(j = -1; j <= 1; j++){

	 neigh_uv = uv + vec2(j, i) * scale;

	 id = texture2D(id_map, neigh_uv);

	 if(id != center_id){
	    sil_cnt++;
	 }
      }
   }

   return sil_cnt;
}

float compute_alpha(in float center_val, in float max_curv, in float dist, in int sil_cnt, in float c_thd0, in float c_thd1)
{
   float val;

   if(tone_effect == 1){
      if(max_curv > 0.0){
	 val = clamp(1.0-center_val + alpha_offset, 0.0, 1.0);
      }
      else{
	 val = clamp(center_val + alpha_offset, 0.0, 1.0);
      }
   }
   else{
      val = 1.0;
   }

   if(abs(max_curv) < c_thd0){
      if(confidence_mode == 1)
	 val *= (abs(max_curv)-c_thd1)/(c_thd0-c_thd1);
      else
	 val = 0.0;
   }

   if(tapering_mode == 1){
      float a = sqrt(2.0 - dist)*0.5;

      if(a > 0.6)
	 a = 1.0;

      val *= a;
   }

   if(silhouette_mode == 1 && sil_cnt > 0){
      val *= float(9-sil_cnt)/9.0;
      val = max(val, 0.1);
   }

   return val;
} 

vec4 draw_line_pixel(in vec2 pix)
{
   // get tone at given pixel location via tone reference image
   vec2 scale = vec2(x_1, y_1), diff;
   vec2 uv = pix * scale, pix2;
   float center_val, half_width;
   int k;
   vec4 col;                           // output color
   vec4 center_id, id;


   bool is_line = true, is_basecoat = false, is_moved = false;
   float max_curv, axis_dist;
   vec2 axis;
   int sil_cnt;

   half_width = compute_line_width(); 

   float c_thd0, c_thd1; // c_thd0: upper bound c_thd1: lower bound
   float h_c_thd0, h_c_thd1; // highlight thresholds
   float l_c_thd0, l_c_thd1; // light thresholds

   c_thd0 = curv_thd0 * half_width;
   c_thd1 = curv_thd1 * half_width;
   l_c_thd0 = c_thd0 * light_control;
   l_c_thd1 = c_thd1 * light_control;
   h_c_thd0 = c_thd0 * highlight_control;
   h_c_thd1 = c_thd1 * highlight_control;

   center_val = texture2D(tone_map, uv).r;
   center_id = texture2D(id_map, uv);

   sil_cnt = is_silhouette(uv);

   pix2 = pix;

   for(k = 0; k < max_iter; k++){ // move to the axis...

      compute_quadric_coefficients(uv, 0, half_width, max_curv, axis_dist, axis);
      // too low curvature 
      if(max_curv > 0.0 && abs(max_curv) <= c_thd1){
	 col.a = 0.0;
	 is_line = false;
	 break;
      }
      else if(max_curv < 0.0 && abs(max_curv) <= l_c_thd1){
	 col.a = 0.0;
	 is_line = false;
	 break;
      }
      // move a pixel whose curvature is larger than a lower bound 
      else{
	 diff = min((axis_dist* axis * half_width)*moving_factor, 2.0);
	 pix2 = pix2 + diff;

	 uv = pix2 * scale;
	 is_moved = true;
      }

   }

   if(is_line == true || draw_silhouette == 1 && sil_cnt > 0){
      // dk stroke
      if(max_curv > 0.0){
	 col.rgb = dk_color;
      }
      // highlight
      else{
	 col.rgb = lt_color;
      }

      float dist = clamp(length(pix - pix2)/half_width, 0.0, 2.0);
      if(max_curv > 0.0){
	 if(line_mode == 3 || line_mode == 4 || line_mode == 7)
	    col.a = 0.0;
	 else
	    col.a = compute_alpha(center_val, max_curv, dist, 
	                                   sil_cnt, c_thd0, c_thd1);
      }
      else{
	 if(line_mode == 2 || line_mode == 4 || line_mode == 6)
	    col.a = 0.0;
	 else
	    col.a = compute_alpha(center_val, max_curv, dist, 
	                                    sil_cnt, l_c_thd0, l_c_thd1);
      }
   }
   else if(line_mode == 1 || line_mode == 4 || line_mode == 6 || line_mode == 7){
      pix2 = pix;
      center_val = texture2D(tone_map, uv).g;
      half_width *=  ht_width_control;
      half_width = max(half_width, 1.0);
      is_line = true;

      for(k = 0; k < max_iter; k++){ // move to the axis...

	 compute_quadric_coefficients(uv, 1, half_width, max_curv, axis_dist, axis);
	 // too low curvature 

	 if(max_curv > 0.0 || max_curv < 0.0 && abs(max_curv) <= h_c_thd1){
	    col.a = 0.0;
	    is_line = false;
	    break;
	 }
	 // move a pixel whose curvature is larger than a lower bound 
	 else{
	    diff = min((axis_dist* axis * half_width)*moving_factor, 2.0);
	    pix2 = pix2 + diff;

	    uv = pix2 * scale;
	    is_moved = true;
	 }

      }

      if(is_line == true){
	 col.rgb = ht_color;
	 float dist = clamp(length(pix - pix2)/half_width, 0.0, 2.0);
	 col.a = compute_alpha(center_val, max_curv, dist, sil_cnt, h_c_thd0, h_c_thd1);
      }
   }

   return col;
}

void main() 
{
   if(line_mode == 0){
      gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
   }
   else{
      gl_FragColor = draw_line_pixel(gl_FragCoord.xy);
   }

}

// img_line.fp
