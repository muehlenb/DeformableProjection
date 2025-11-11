// © 2025, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

in vec2 vScreenPos;

uniform sampler2D inputTexture;
uniform usampler2D inputPingPong;

/** Determines whether to expect boolean texture or */
uniform bool isFirstPass;

uniform int k = 256;

out uvec3 FragColor;

const ivec2 offsets[9] = ivec2[](ivec2(-1,-1),ivec2(1,-1),ivec2(-1,1),ivec2(1,1),ivec2(0,-1),ivec2(-1,0),ivec2(1,0),ivec2(0,1),ivec2(0,0));

// True, when a is nearer to p then b to p:
bool firstIsNearer(uvec2 a, uvec2 b, uvec2 p){
	// TODO: Calculate the distance of corresponding vertices projected
	// into the projector's image
	
	// TODO: Projector uniforms!!s

	vec2 pa = vec2(p) - vec2(a);
	vec2 pb = vec2(p) - vec2(b);
	
	return dot(pa,pa) < dot(pb,pb);
}

void main()
{	
	uvec2 nearestCoords[3] = uvec2[](uvec2(0,0),uvec2(0,0),uvec2(0,0));

	// In first pass, all pixels are unset, so we need no distance check:
	if(isFirstPass){
		uvec2 uScreenPos = uvec2(round(vScreenPos * vec2(511))); // 255 ist hier sinnvoll mit round, weil so immer im Pixelrahmen geblieben wird
		for(int i=0; i < 9; ++i){
			vec2 nowCoords = vScreenPos + offsets[i] * k / 512.0; // 256 ergibt hier sind, weil der Abstand zweier Pixel 1/256 ist!
			uvec2 uNowCoords = uvec2(round(nowCoords * vec2(511)));
			
			if(nowCoords.x >= 0 && nowCoords.x < 1 && nowCoords.y >= 0 && nowCoords.y < 1){
				vec4 boolTexture = texture(inputTexture, nowCoords);
				// Prüfe, ob Kanal in boolTextur belegt ist. Wenn ja, schreibe Koordinate als kleinste Zahl:
				if(boolTexture[0] > 0.5)
					nearestCoords[0] = uNowCoords;
				if(boolTexture[1] > 0.5)
					nearestCoords[1] = uNowCoords;
				if(boolTexture[2] > 0.5)
					nearestCoords[2] = uNowCoords;

				// Wenn vorherige Textur auch eine JumpFlood PingPong Textur war, muss der Wert vorher auf Gültigkeit (d.h. nicht 0)
				// und Distanz geprüft werden.
			}
		}
	} else { // In other passes, we need to calculate the distance:
		uvec2 uScreenPos = uvec2(round(vScreenPos * vec2(511))); 
		for(int i=0; i < 9; ++i){
			ivec2 uNowCoords = ivec2(uScreenPos) + offsets[i] * k;
			
			if(uNowCoords.x >= 0 && uNowCoords.x < 512 && uNowCoords.y >= 0 && uNowCoords.y < 512){
				uvec3 pongTexture = texelFetch(inputPingPong, uNowCoords, 0).xyz;
				
				
				// Prüfe, ob Kanal einen Wert besitzt, wenn ja, schaue, ob dieser näher ist als der Aktuelle:
				if(pongTexture[0] > 0u){
					uvec2 coords0 = uvec2(pongTexture.x & 65535u, (pongTexture.x >> 16u) & 65535u);
					if((nearestCoords[0].x == 0u && nearestCoords[0].y == 0u) || firstIsNearer(coords0, nearestCoords[0], uScreenPos))
						nearestCoords[0] = coords0;					
				}
				if(pongTexture[1] > 0u){
					uvec2 coords1 = uvec2(pongTexture.y & 65535u, (pongTexture.y >> 16u) & 65535u);
					if((nearestCoords[1].x == 0u && nearestCoords[1].y == 0u) || firstIsNearer(coords1, nearestCoords[1], uScreenPos))
						nearestCoords[1] = coords1;		
				}
				if(pongTexture[2] > 0u){
					uvec2 coords2 = uvec2(pongTexture.z & 65535u, (pongTexture.z >> 16u) & 65535u);	
					if((nearestCoords[2].x == 0u && nearestCoords[2].y == 0u) || firstIsNearer(coords2, nearestCoords[2], uScreenPos))
						nearestCoords[2] = coords2;			
				}

				// Wenn vorherige Textur auch eine JumpFlood PingPong Textur war, muss der Wert vorher auf Gültigkeit (d.h. nicht 0)
				// und Distanz geprüft werden.
			}
		}
	
	}
	
	
	FragColor = uvec3(
		nearestCoords[0].x | (nearestCoords[0].y << 16),
		nearestCoords[1].x | (nearestCoords[1].y << 16),
		nearestCoords[2].x | (nearestCoords[2].y << 16)
	);
}
