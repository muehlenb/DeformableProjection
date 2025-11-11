/**
 * Maps value from the range (min1, max1) to range (min2, max2).
 */
float map(float value, float min1, float max1, float min2, float max2) {
  	return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float folds(vec2 uv){
    vec2 p = fract(uv * 15.0);
	
	float foldPattern = clamp(1 - map(max(max(p.x, p.y), max(1-p.x, 1-p.y)), 0.6,1.0, 0.0, 1.0),0.0,1.0);


    // Hinzufügen von einfachem Rauschen für Variation
    float noise = fract(sin(dot(uv, vec2(12.9898,78.233))) * 43758.5453);

    // Kombination des Faltenmusters mit dem Rauschen
    return foldPattern;// + noise * 0.1;
}

float mod_f(float x, float y)
{
	return x - y * floor(x/y);
}

vec4 wind(vec4 position, float time) {

	float t = mod_f(time, 9.0);
	
	if(t < 1)
		t = t;
	else if(t < 4)
		t = 1;
	else if(t < 5)
		t = 5 - t;
	else if(t < 9)
		t = 0;

    float frequency = 50.5;
    float speed = 1.8;
    float magnitude = 0.005;

    vec4 windOffset;
    windOffset.x = 0.0;
    windOffset.y = cos(position.x * frequency + t * speed);
    windOffset.z = 0.0;
	windOffset.w = 0.0;

    return windOffset * magnitude;
}
