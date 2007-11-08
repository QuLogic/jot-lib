uniform sampler2D tone_map;
uniform float x_1;
uniform float y_1;
uniform float blur_size;

uniform mat4 P_matrix;
uniform int detail_func; // control line width according to depth or user factor
uniform float unit_len, ratio_scale, edge_len_scale; // parameters for depth computation
uniform float user_depth; // user depth for detail control

varying vec4 P; // vertex position, eye space
varying float local_e_l;
uniform float global_edge_len;

float edge_length_in_pix(in float e_l)
{
   vec4 v0 = vec4(0.0, -e_l, length(P), 1.0); 
   vec4 v1 = vec4(0.0, e_l, length(P), 1.0);

   vec4 p0, p1;

   p0 = P_matrix * v0;
   p1 = P_matrix * v1;

   return length(p0/p0.w-p1/p1.w);
}

float compute_blur_size(in float size)
{
   float factor;
   float computed_size;

   if(detail_func == 0){
      computed_size = size;
   }
   if(detail_func == 1){
      factor = edge_length_in_pix(unit_len)*edge_len_scale;
      computed_size = size*factor;
   }
   if(detail_func == 2){
      factor = edge_length_in_pix(global_edge_len)*edge_len_scale;
      computed_size = size*factor;
   }
   if(detail_func == 3){
      factor = edge_length_in_pix(local_e_l)*edge_len_scale;
      computed_size = size*factor;
   }
   if(detail_func == 4){
      float ratio = local_e_l/global_edge_len;
      if(ratio > 1.0){
	 factor = 1.0 + log(ratio + 1.0);
      }
      else{
	 factor = 1.0 - log(2.0 - ratio);
      }
      computed_size = size*factor;
   }
   if(detail_func == 5){
      computed_size = size*(1.0-user_depth);
   }

   computed_size = clamp(computed_size, 0.0, 7.0);

   return computed_size;
}


vec4 average_neighbors(vec2 pix)
{
   int m, n;
   float ii, jj; 
   vec3 avg = vec3(0.0, 0.0, 0.0);
   float o_f = compute_blur_size(blur_size);
   int o = int(o_f); 
   int num = (o*2+1)*(o*2+1);
   int s = o*2 + 1;
   float diff = o_f - float(o);

   for(m = 0; m < num; m++){
      ii = pix.x + float(m/s - o);
      jj = pix.y + float(m - (m/s)*s - o);
      ii = ii * x_1;
      jj = jj * y_1;

      avg += texture2D(tone_map, vec2(ii, jj)).rgb;
   }
   avg = avg/float(num);

   if(diff > 1.0e-8){
      float diff_num = diff*float(s);
      vec3 avg_t[4];
      avg_t[0] = vec3(0.0, 0.0, 0.0);
      ii = float(pix.x-o_f) * x_1;
      for(m = 0; m < s; m++){
	 jj = pix.y + float(m - o);
	 jj = jj * y_1;

	 avg_t[0] += texture2D(tone_map, vec2(ii, jj)).rgb*diff;
      }

      avg_t[1] = vec3(0.0, 0.0, 0.0);
      ii = float(pix.x+o_f) * x_1;
      for(m = 0; m < s; m++){
	 jj = pix.y + float(m - o);
	 jj = jj * y_1;

	 avg_t[1] += texture2D(tone_map, vec2(ii, jj)).rgb*diff;
      }

      avg_t[2] = vec3(0.0, 0.0, 0.0);
      jj = float(pix.y-o_f) * y_1;
      for(m = 0; m < s; m++){
	 ii = pix.x + float(m - o);
	 ii = ii * x_1;

	 avg_t[2] += texture2D(tone_map, vec2(ii, jj)).rgb*diff;
      }

      avg_t[3] = vec3(0.0, 0.0, 0.0);
      jj = float(pix.y+o_f) * y_1;
      for(m = 0; m < s; m++){
	 ii = pix.x + float(m - o);
	 ii = ii * x_1;

	 avg_t[3] += texture2D(tone_map, vec2(ii, jj)).rgb*diff;
      }

/*      float avg_tt[4];
      ii = (pix.x - o_f)*x_1;
      jj = (pix.y - o_f)*y_1;
      avg_tt[0] = texture2D(tone_map, vec2(ii, jj)).r*diff*diff;

      ii = (pix.x + o_f)*x_1;
      jj = (pix.y - o_f)*y_1;
      avg_tt[1] = texture2D(tone_map, vec2(ii, jj)).r*diff*diff;

      ii = (pix.x - o_f)*x_1;
      jj = (pix.y + o_f)*y_1;
      avg_tt[2] = texture2D(tone_map, vec2(ii, jj)).r*diff*diff;

      ii = (pix.x + o_f)*x_1;
      jj = (pix.y + o_f)*y_1;
      avg_tt[3] = texture2D(tone_map, vec2(ii, jj)).r*diff*diff;*/

      avg = (avg*float(num) + avg_t[0] + avg_t[1] + avg_t[2] + avg_t[3] )/(float(num) + 4.0 * diff_num);
//	    + avg_tt[0] + avg_tt[1] + avg_tt[2] + avg_tt[3])/ (num + 4.0 * diff_num + 4.0 * diff_num * diff);
   }

   return vec4(avg, 1.0);
}

void main() 
{
   gl_FragColor = average_neighbors(gl_FragCoord.xy);
}
