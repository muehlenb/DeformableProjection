// © 2025, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Andre Mühlenbrock (muehlenb@uni-bremen.de)

#version 330 core

in vec2 vScreenPos;

uniform int screenWidth;
uniform int screenHeight;
uniform float cursorX;
uniform float cursorY;

out vec4 FragColor;

float distanceToNearestScreenEdge(vec2 uv) {
    vec2 aspect = vec2(float(screenWidth) / float(screenHeight), 1.0);
    // Fall 1: Punkt innerhalb des Bildrahmens
    if (all(greaterThanEqual(uv, vec2(0.0))) && all(lessThanEqual(uv, vec2(1.0)))) {
		return 0;
	}

    vec2 clamped = clamp(uv, vec2(0.0), vec2(1.0));
    vec2 dir = (uv - clamped) * aspect;
    return length(dir);
}


vec2 projectToNearestEdgeIfOutside(vec2 uv) {
    // Check: Ist der Punkt im Frustum?
    if (all(greaterThanEqual(uv, vec2(0.0))) && all(lessThanEqual(uv, vec2(1.0)))) {
        return uv; // Punkt liegt innerhalb -> keine Projektion
    }

    vec2 center = vec2(0.5);
    vec2 dir = uv - center;
	

    // Skaliere den Richtungsvektor so, dass der Punkt auf dem Rand landet
    // Finde maximalen Faktor, bei dem x oder y den Rand trifft (entlang dir)
    vec2 scale = vec2(
        (dir.x > 0.0) ? (1.0 - center.x) / dir.x : -center.x / dir.x,
        (dir.y > 0.0) ? (1.0 - center.y) / dir.y : -center.y / dir.y
    );

    // Nutze kleinsten gültigen Skalierungsfaktor (um zuerst eine Kante zu treffen)
    float k = min(scale.x, scale.y);

    // Projektionspunkt auf Kante
    vec2 edgePoint = center + dir * k;

    // Sicherheitshalber clampen (wegen Gleitkommaungenauigkeit)
    return clamp(edgePoint, vec2(0.0), vec2(1.0));
}


/**
 * Calculates a smoothed vertex for each input vertex / pixel
 */
void main()
{
    vec2 aspect = vec2(float(screenWidth) / float(screenHeight), 1.0);
	
	vec2 cursorPos = vec2(1 - cursorX, cursorY);
	vec2 edgePos = projectToNearestEdgeIfOutside(cursorPos);
	float dist = distanceToNearestScreenEdge(cursorPos);

	float pixelDist = length(edgePos * aspect - vScreenPos * aspect);

	if(pixelDist < dist){
	
		float normedDist = 1 - pixelDist / dist;
		normedDist *= normedDist;
		normedDist *= normedDist;
	
		FragColor = vec4(normedDist * 0.4, normedDist * 0.4, normedDist * 0.4, 1.0);
	
	} else {
		FragColor = vec4(0.0, 0.0, 0.0, 1.0);	
	}
}
