// simple shader that shows the surface normal as an rgb color

varying vec3 normal;  // per-fragment normal

void main() 
{
   gl_FragColor = vec4(abs(normalize(normal)), 1);
}

// simple.fp
