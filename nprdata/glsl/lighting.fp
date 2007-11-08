// Fragment program to compute OpenGL lighting model 
// for case of directional or point lights 0 thru 4. 
//
// Note: Assumes lights 0 - 3 are enabled (and others disabled).

varying vec3 N;  // per-fragment normal, eye space
varying vec4 P;  // per-fragment location, eye space

vec4 light_contribution(
   in gl_LightSourceParameters light, // light parameters
   in vec4 a,                         // material ambient color
   in vec4 d,                         // material diffuse color
   in vec4 s,                         // material specular color
   in vec3 n,                         // per-pixel unit normal, eye space
   in vec3 v                          // per-pixel unit view vector, eye space
   )
{
   // compute unit vector to light,
   // handle point or directional case:
   vec3 l = normalize(
      (light.position.w == 0.0 ?
       light.position :          // directional
       light.position - P).xyz   // positional
      );

   // attenuation:
   float t = 1.0;
   if (light.position.w != 0.0) { // if positional
      float d = distance(light.position, P);
      float k0 = light.constantAttenuation;
      float k1 = light.linearAttenuation;
      float k2 = light.quadraticAttenuation;
      t = 1.0 / (k0 + d*(k1 + d*k2));
   }

   // spotlight:
   if (light.spotCutoff < 180.0) { // if spotlight in effect
      float sl = dot(normalize(light.spotDirection), -l);
      t *= (sl < light.spotCosCutoff) ?
         0.0 :                                   // out of cone
         pow(max(sl,0.0), light.spotExponent);   // inside cone
   }
         
   // n dot l used in diffuse part:
   float nl = max(dot(n,l), 0.0);

   // h dot n raised to shininess power; used in specular part
   vec3  h = normalize(l + v);
   float hn = pow(max(0.0, dot(h,n)), gl_FrontMaterial.shininess);

   return t*((light.ambient  * a)      + // ambient contribution
             (light.diffuse  * d) * nl + // diffuse contribution
             (light.specular * s) * hn); // specular contribution
}

void main() 
{
   // material ambient, diffuse, and specular colors:
   vec4 a = gl_FrontMaterial.ambient;
   vec4 d = gl_FrontMaterial.diffuse;
   vec4 s = gl_FrontMaterial.specular;

   // N is interpolated across the triangle, so normalize it:
   vec3 n = normalize(N);

   // unit view vector in eye space:
   vec3 v = normalize(-P.xyz);

   // base color: global ambient light:
   vec4 color = gl_LightModel.ambient * a;

   // some ATI cards have problems with for loops
   color += light_contribution(gl_LightSource[0],a,d,s,n,v);
   color += light_contribution(gl_LightSource[1],a,d,s,n,v);
   color += light_contribution(gl_LightSource[2],a,d,s,n,v);
   color += light_contribution(gl_LightSource[3],a,d,s,n,v);

   gl_FragColor = color;
}

// lighting.fp
