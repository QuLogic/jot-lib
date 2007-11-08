// ridge.fp

uniform sampler2D tone_map;
uniform float x_1;
uniform float y_1;
uniform int start_offset, end_offset; 
uniform float curv_thd, dist_thd;
uniform float vari_thd;

uniform float dark_thd, bright_thd;

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

float A[9]; 

float grey_val(in vec3 col)
{
   return col.r*0.27 + col.g*0.67 + col.b*0.06;
}

vec4 compute_quadric_coefficients()
{
   float T[6];
   int i, j;
   float x, y;
   vec4 quad;

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

   float a, b, c, d, e, f, l1, l2, cos2theta, sin2theta;
   // quadric form ax^2 + 2bxy + cy^2 + 2dx + 2ey + f

   // 2x2 symmetric matrix used in the quadratic form:
   //   a b 
   //   b c 
   a = T[0];
   b = 0.5*T[1];
   c = T[2];

   // compute eigenvalues 
   // can optimize:

   float sqrt_term = sqrt((a-c)*(a-c) + 4.0*(b*b));
   l1 = (a+c + sqrt_term)*0.5;
   l2 = (a+c - sqrt_term)*0.5;

   if(abs(l1) >= abs(l2)){
      quad[0] = l1;
   }
   else{
      quad[0] = l2;
   }

   if(b == 0.0){
      if(abs(a) > abs(b)){
	 sin2theta = 0.0;
	 cos2theta = 1.0;
      }
      else{
	 sin2theta = 1.0;
	 cos2theta = 0.0;
      }
   }
   else{
      if( abs(l1) >= abs(l2))
	 sin2theta = -(a-l1)/b;
      else
	 sin2theta = -(a-l2)/b;

      cos2theta = 1.0;
   }

   quad[2] = atan(sin2theta,cos2theta); // angle

   // XXX - need comments
   d = 0.5*T[3];
   e = 0.5*T[4];
   f = T[5];

   float t[2], invS[4], x0, y0;

   t[0] = -d;
   t[1] = -e;

   float det = a*c-b*b;

   if(abs(det) > 0.0){
      invS[0] = c/det;
      invS[1] = -b/det;
      invS[2] = -b/det;
      invS[3] = a/det;

      x0 = invS[0]*t[0]+invS[1]*t[1];
      y0 = invS[2]*t[0]+invS[3]*t[1];
   }
   else if(abs(b*d - e*a) < 1.0e-10){
// ax + by = -d
// x, y = -(ax+d)/b
// x^2 + (a^2x^2 + 2adx + d^2)/b^2
// x0 = -ad/(a^2+b^2), y0 = -((db)/(a^2+b^2))
      float tt = a*a + b*b;
      x0 = -a*d/tt;
      y0 = -b*d/tt;
   }
   else{
      x0 = y0 = 10.0;
   }

   float len = sqrt(cos2theta*cos2theta + sin2theta*sin2theta);

   sin2theta = sin2theta/len;
   cos2theta = cos2theta/len;

   quad[1] = 2.0-abs(cos2theta*x0 + sin2theta*y0);

   return quad;
}

vec4 compute_quadrics(in vec2 pix)
{
   // get tone at given pixel location via tone reference image

   float x = pix.x*x_1;
   float y = pix.y*y_1;
   float avg, vari, center_val;
   int m, n, i, j;
   float ii, jj;
   float bi;
   int size, offset, inter;

   vec4 quad;
   vec4 param;
   float neighbor_val;

   if(texture2D(tone_map, vec2(x, y)).r == 1.0){
      param = vec4(0.0, 0.0, 0.0, 0.0);
   }
   else{
      center_val = texture2D(tone_map, vec2(x, y)).r;

      for(offset = start_offset; offset <= end_offset; offset += 2){

      avg = 0.0;
      vari = 0.0;

      for(m = -offset; m <= offset; m+=offset){
	 for(n = -offset; n <= offset; n+=offset){

	 ii = pix.y+float(m);
	 jj = pix.x+float(n);

	 ii = ii*y_1;
	 jj = jj*x_1;

	 neighbor_val = texture2D(tone_map, vec2(jj, ii)).r;
	 avg = avg + neighbor_val;
	 vari = vari + neighbor_val*neighbor_val;

	 }
      }

      avg = avg/9.0;
      vari = sqrt(vari/9.0 - avg*avg);

      if(vari >= vari_thd || avg >= dark_thd || avg < bright_thd)
	 break;
      }

      m = -offset;

      for(i = 0; i < 3; i++){
	 n = -offset;

	 for(j = 0; j < 3; j++){

	    ii = pix.y+float(m);
	    jj = pix.x+float(n);

	    ii = ii*y_1;
	    jj = jj*x_1;

	    neighbor_val = texture2D(tone_map, vec2(jj, ii)).r;

	    A[i*3+j] = neighbor_val;

	    n += offset;
	 }
	 m += offset;
      }

      quad = compute_quadric_coefficients();

      if(quad.r > curv_thd && quad.g > dist_thd){
	 param.r = (quad.r+1.0)/2.0;
      }
      else{
	 param.r = 0.0;
      }

      param.g = ((quad.b+M_PI)/(2.0*M_PI)); // angle
      param.b = (float(offset)-float(start_offset))
		  /(float(end_offset)-float(start_offset)+0.1); // radius
      param.a = 1.0;

   }

   return param;
}

void main()
{
   gl_FragColor = compute_quadrics(gl_FragCoord.xy);

} 

// ridge.fp
