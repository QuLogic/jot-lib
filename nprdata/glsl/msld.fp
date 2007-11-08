// msld shader that shows the surface normal as an rgb color

uniform sampler2D tone_map;
uniform float x_1;
uniform float y_1;
varying float nv;

float H[6][9], A[9];

vec3 checkNeighbors(vec2 pix)
{
	int m, n;
	float ii, jj;
	vec3 col;
	bool t = true;

	for(m = -3; m <= 3; m++){
		for(n = -3; n <= 3; n++){
				ii = pix.x+float(m);
				jj = pix.y+float(n);

				ii = ii * x_1;
				jj = jj * y_1;

				if(texture2D(tone_map, vec2(ii, jj)).a == 0.0){
					t = false;
					break;
				}

		}
		if(t == false)
			break;
	}

	if(t == true)
		col = vec3(1.0, 1.0, 1.0);
	else
		col = vec3(1.0, 0.0, 0.0);

	return col;

}

float averageNeighbors(vec2 pix)
{
	int m, n;
	float ii, jj, avg = 0.0;

	for(m = -3; m <= 3; m++){
		for(n = -3; n <= 3; n++){
			ii = pix.x+float(m);
			jj = pix.y+float(n);

			ii = ii * x_1;
			jj = jj * y_1;

			avg += texture2D(tone_map, vec2(ii, jj)).r;

		}
	}
	avg = avg/49.0;

	return avg;

}

vec4 computeQuadricCoefficients()
{
	float T[6];
	int i, j;
	float x, y;
	vec4 quad;

	for(i = 0; i < 6; i++){
		T[i] = 0.0;
		for(j = 0; j < 9; j++)
			T[i] += (H[i][j]*A[j]);
	}

	float a, b, c, d, e, f, l1, l2, cos2theta, sin2theta;
	a = T[0];
	b = 0.5*T[1];
	c = T[2];

	l1 = (a+c + sqrt((a-c)*(a-c) + 4.0*(b*b)))*0.5;
	l2 = (a+c - sqrt((a-c)*(a-c) + 4.0*(b*b)))*0.5;

	if(abs(l1) >= abs(l2)){
		quad[0] = l1;
		quad[1] = l2;
	}
	else{
		quad[0] = l2;
		quad[1] = l1;
	}

	if( abs(b) < 5.0e-5){
		if(abs(a) > abs(b)){
			cos2theta = 0.0;
			sin2theta = 1.0;
		}
		else{
			cos2theta = 1.0;
			sin2theta = 0.0;
		}
	}
	else{
		if( abs(l1) >= abs(l2))
			cos2theta = -(a-l1)/b;
		else
			cos2theta = -(a-l2)/b;
	}

	sin2theta = 1.0;

	quad[3] = atan(sin2theta,cos2theta);

	d = 0.5*T[3];
	e = 0.5*T[4];
	f = T[5];

	float S[2][2], t[2], invS[2][2], X0[2];

	S[0][0] = a;
	S[0][1] = b;
	S[1][0] = b;
	S[1][1] = c;

	t[0] = -d;
	t[1] = -e;

	float det = S[0][0]*S[1][1]-S[0][1]*S[1][0];

	if(abs(det) > 0.0){
		invS[0][0] = S[1][1]/det;
		invS[0][1] = -S[0][1]/det;
		invS[1][0] = -S[1][0]/det;
		invS[1][1] = S[0][0]/det;

		X0[0] = invS[0][0]*t[0]+invS[0][1]*t[1];
		X0[1] = invS[1][0]*t[0]+invS[1][1]*t[1];
	}
	else{
		X0[0] = X0[1] = 0.0;
	}

	float len = sqrt(cos2theta*cos2theta + sin2theta*sin2theta);

	sin2theta = sin2theta/len;
	cos2theta = cos2theta/len;


	x = X0[1] - 2.0;
	y = X0[0] - 2.0;

	quad[2] = 1.0-abs(cos2theta*x + sin2theta*y);

	return quad;
}

vec3 computeQuadrics(in vec2 pix)
{
	// get tone at given pixel location via tone reference image

	float x = pix.x*x_1;
	float y = pix.y*y_1;
	float avg;
	int m, n, i, j;
	float ii, jj;
	float bi;
	int size, offset, inter;

	vec4 quad;

	if(texture2D(tone_map, vec2(x, y)).r == 1.0){
		quad = vec4(0.0, 0.0, 0.0, 0.0);
	}
	else{

		H[0][0] =  0.1666667 ; H[0][1] = -0.3333333 ; H[0][2] =  0.1666667 ; H[0][3] =  0.1666667 ; H[0][4] = -0.3333333 ; H[0][5] =  0.1666667 ; H[0][6] =  0.1666667 ; H[0][7] = -0.3333333 ; H[0][8] =  0.1666667 ; 

		H[1][0] = 0.25 ; H[1][1] = 2.220446e-016 ; H[1][2] = -0.25 ; H[1][3] = -6.661338e-016 ; H[1][4] = -1.110223e-015 ; H[1][5] = -6.661338e-016 ; H[1][6] =      -0.25 ; H[1][7] = -2.220446e-016 ; H[1][8] = 0.25 ; 

		H[2][0] =  0.1666667 ; H[2][1] =  0.1666667 ; H[2][2] =  0.1666667 ; H[2][3] = -0.3333333 ; H[2][4] = -0.3333333 ; H[2][5] = -0.3333333 ; H[2][6] =  0.1666667 ; H[2][7] =  0.1666667 ; H[2][8] =  0.1666667 ; 

		H[3][0] =  -1.333333 ; H[3][1] =   1.333333 ; H[3][2] = 1.065814e-014 ; H[3][3] = -0.8333333 ; H[3][4] =   1.333333 ; H[3][5] = -0.5 ; H[3][6] = -0.3333333 ; H[3][7] =   1.333333 ; H[3][8] = -1.0 ; 

		H[4][0] =  -1.333333 ; H[4][1] = -0.8333333 ; H[4][2] = -0.3333333 ; H[4][3] =   1.333333 ; H[4][4] =   1.333333 ; H[4][5] =   1.333333 ; H[4][6] = -5.329071e-015 ; H[4][7] = -0.5 ; H[4][8] = -1.0 ; 

		H[5][0] =   2.888889 ; H[5][1] = -0.1111111 ; H[5][2] =  0.2222222 ; H[5][3] = -0.1111111 ; H[5][4] =  -2.111111 ; H[5][5] = -0.7777778 ; H[5][6] =  0.2222222 ; H[5][7] = -0.7777778 ; H[5][8] =   1.555556 ; 

		for(offset = 5; offset < 21; offset += 2){

			avg = 0.0;

			for(m = -offset; m <= offset; m+=offset){
				for(n = -offset; n <= offset; n+=offset){

					ii = pix.x+float(m);
					jj = pix.y+float(n);

					bi = averageNeighbors(vec2(ii, jj));
					avg = avg + bi;

				}
			}

			avg = avg/9.0;

			if(avg > 0.10)
				break;
		}

		m = -offset;

		for(i = 0; i < 3; i++){

			n = -offset;

			for(j = 0; j < 3; j++){

				ii = pix.y+float(m);
				jj = pix.x+float(n);

				bi = averageNeighbors(vec2(jj,ii));

				A[i+j*3] = bi;

				n += offset;
			}
			m += offset;
		}

		quad = computeQuadricCoefficients();

	}
	return quad.rgb;
}

void main()
{
	vec2 pix = gl_FragCoord.xy;
	vec3 col = computeQuadrics(pix);


	//gl_FragColor = vec4(abs(col.r), abs(col.r), abs(col.r), 1.0);
	if(abs(col.r) > 0.2 && col.b > 0.95)
	gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
  else
	gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);

} 
// msld.fp
