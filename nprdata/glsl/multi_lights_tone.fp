// tone shader fragment program.

uniform sampler2D toon_tex; 
uniform sampler2D tex_2d; 
uniform bool is_tex_2d;

varying vec4 P; // fragment position, eye space
varying vec3 N; // fragment normal, eye space

varying float e_l;

uniform bool is_enabled; // constant or not?
uniform bool remap_nl;   // convert n*l in [-1,1] to [0,1]
uniform int  remap;      // remap tone?    0: none, 1: toon, 2: smoothstep
uniform int  backlight;  // add backlight? 0: none, 1: dark, 2: light
uniform float e0;        // smoothstep edge 0
uniform float e1;        // smoothstep edge 1
uniform float s0;        // backlight edge 0
uniform float s1;        // backlight edge 1
uniform int show_channel;

float luminance(in vec4 col)
{
   // convert RGB color to luminance:
   return dot(col.rgb, vec3(0.3, 0.59, .11));
}

vec2 light_contribution(
   vec3 n,                             // unit surface normal, eye space
   vec3 v                              // unit view vector, eye space
   )
{
   vec4 ret = gl_LightModel.ambient * gl_FrontMaterial.ambient;

   for(int i = 0; i < 4; i++){
      vec4 q = gl_LightSource[i].position;

      // Direction to light from P:
      vec3 l = normalize(((q.w == 0.0) ? q : (q - P)).xyz);

      // diffuse part:
      float nl = dot(n,l);
      nl = remap_nl ? (nl + 1.0)/2.0 : nl;
      nl = clamp(nl, 0.0, 1.0);

      // specular part:
      vec3  r = reflect(-l,n);
      float rv = dot(r, v);
      rv = clamp(rv, 0.0, 1.0);
      rv = pow(rv, gl_FrontMaterial.shininess);

      // skipping spotlight effect..

      // attenuation:
      float t = 1.0;
      if (q.w != 0.0) { // if positional
	 float d = distance(q, P);
	 float k0 = gl_LightSource[i].constantAttenuation;
	 float k1 = gl_LightSource[i].linearAttenuation;
	 float k2 = gl_LightSource[i].quadraticAttenuation;
	 t = 1.0 / (k0 + d*(k1 + d*k2));
      }

      // resulting color (before attenuation):
      ret.r += t* ((gl_LightSource[i].ambient  * gl_FrontMaterial.ambient).r +
	   ((gl_LightSource[i].diffuse  * gl_FrontMaterial.diffuse)  * nl).r);
      ret.g += t* ((gl_LightSource[i].specular * gl_FrontMaterial.specular) * rv).r;
   }

   // return result with attenuation:
   return vec2(clamp(ret.r, 0.0, 1.0), clamp(ret.g, 0.0, 1.0));
}

float sil_val(vec3 n, vec3 v)
{
   // return 1 at silhoutte, falling to 0 away from silhouette
   return smoothstep(s0, s1, 1.0 - dot(n,v));
}

vec2 tone_val(
   vec3 n,                             // unit surface normal, eye space
   vec3 v                              // unit view vector, eye space
   )
{
   float ret = 0.0;
   vec2 val;
   if (is_enabled) {
      // compute tone via standard lighting:

      val = light_contribution(n, v);
      ret = val.r;

      // optionally remap tone:
      if (remap == 1) {                   // toon remapping
         // lookup alpha via light strength;
         // for strength near 0, alpha is near 1,
         // so remapping needs 1.0 - alpha:
         ret = 1.0 - texture2D(toon_tex, vec2(ret, 0.0)).a;
      } else if (remap == 2) {            // smoothstep remapping
         ret = smoothstep(e0, e1, ret);
      }

      // optionally add backlight
      if (backlight == 1) {               // dark                  
         float val = sil_val(n,v);
         ret = min(ret, 1.0 - val);
      } else if (backlight == 2) {        // light
         float val = sil_val(n,v);
         ret = max(ret, val);
      }
      // reduce the effect if incoming alpha value is < 1:
      ret = mix(1.0, ret, gl_Color.a);

   }

   return vec2(ret, val.g);
}

void main() 
{
   vec3 n = normalize(N);       // normal
   vec3 v = normalize(-P.xyz);  // view vector

   // BTW, the compiler can't deal with for loops
   vec2 val;
   if(!is_tex_2d){
      val = tone_val(n, v);
   }
   else{
      float lumi = luminance(texture2D(tex_2d, gl_TexCoord[0].st));      
      val.r = lumi;
      val.g = 0.0;
   }

   if(show_channel == 0){
      gl_FragColor = vec4(val.r, val.g, 0.0, 1.0); 
   }
   if(show_channel == 1){
      gl_FragColor = vec4(val.r, val.r, val.r, 1.0); 
   }
   if(show_channel == 2){
      gl_FragColor = vec4(val.g, val.g, val.g, 1.0); 
   }
   if(show_channel == 3){
      float s = clamp(val.r + val.g, 0.0, 1.0);
      gl_FragColor = vec4(s, s, s, 1.0); 
   }
}

// multi_lights_tone.fp
