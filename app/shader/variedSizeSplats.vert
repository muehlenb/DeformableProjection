#version 330

/** Color from vertex shader */
out vec4 color_from_vs;

/** World position */
out vec4 world_position;

/** Position texture */
uniform sampler2D positionTexture;

/** Color texture */
uniform sampler2D colorTexture;

/**
 * Projection matrix which performs a perspective transformation.
 */
uniform mat4 projection;

/**
 * View matrix which transforms the vertices from world space
 * into camera space.
 */
uniform mat4 view;

/**
 * Model matrix which transforms the vertices from model space
 * into world space.
 */
uniform mat4 model;

/** Additional scale of the varied sized splats */
uniform float pointScale;

/** Display width and height */
uniform int displayWidth;
uniform int displayHeight;


float getAverageNeighborDistance(vec4 p, vec2 uv, vec2 texelSize){
	int count = 0;
	float sumDist = 0.f;
	
	vec4 pWorld = model * p;
	vec4 pProjected = projection * view * pWorld;
				
	for(int y = -1; y <= 1; ++y){
		for(int x = -1; x <= 1; ++x){
			if(!(x != 0 && y != 0) && !(x == 0 && y == 0)){
				vec2 qUV = uv + vec2(x,y) * texelSize;
				vec4 q = texture(positionTexture, qUV);
				vec4 qWorld = model * q;
				
				if(distance(qWorld, pWorld) > 0.05)
					continue;
				
				vec4 qProjected = projection * view * qWorld;
				
				if(isnan(qProjected.x) || isnan(qProjected.y) || isnan(qProjected.w))
					continue;
				
				vec2 delta = (pProjected.xy / pProjected.w - qProjected.xy / qProjected.w) * vec2(displayWidth, displayHeight) * pointScale;
				sumDist += length(delta);
				++count;
			}				
		}
	}


	if(count == 0)
		return 1;
		
	float dist = sumDist / count;

	return dist;
}


/**
 * Since this is the vertex shader, the following program is
 * executed for each vertex (i.e. triangle point) individually.
 */
void main()
{
	ivec2 texSize = textureSize(positionTexture, 0);
	
	float u = float(gl_VertexID % texSize.x + 0.5) / texSize.x;
	float v = float(gl_VertexID / texSize.x + 0.5) / texSize.y;
	
	vec2 uv = vec2(u,v);
	
	vec4 position = texture(positionTexture, uv);
	vec4 color = texture(colorTexture, uv);
	
	color_from_vs = color;
	world_position = model * position;

    // Calculate gl_Position:
    gl_Position = projection * view * world_position;
	
	// Set point size:
	gl_PointSize = getAverageNeighborDistance(position, uv, 1.0 / texSize);
}
