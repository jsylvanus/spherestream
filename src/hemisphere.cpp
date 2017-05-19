#include "hemisphere.hpp"

#include <cmath>

using namespace std;

HemisphereGeometry::HemisphereGeometry(float rad, int circleSeg, int heightSeg, float ap) :
	radius(rad), circleSegments(circleSeg), heightSegments(heightSeg), aperture(ap)
{
	generatePoints();
	createVAO();
}

void HemisphereGeometry::Draw() const {
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void HemisphereGeometry::generatePoints() {
	// create geometry and uvs
	for (int hs = 0; hs < heightSegments; hs++) {
		float theta = hs / (float)(heightSegments - 1) * TAU * 0.25;
		float x = cos(theta); // depth of current ring
		float scale = sin(theta); // scale of current ring
		for (int cs = 0; cs < circleSegments; cs++) {
			float phi = cs / (float)(circleSegments - 1) * TAU;
			point pt = { x, sin(phi) * scale, cos(phi) * scale, 0, 0 };

			float r = 2 * atan2(sqrt(pt.y*pt.y + pt.z*pt.z), pt.x) / aperture;
			float angle = atan2(pt.z, pt.y);
			pt.u = r * cos(angle);
			pt.v = r * sin(angle);

			pointdata.push_back(pt);
		}
	}

	// create indices
	for (int hs = 0; hs < heightSegments; hs++) {
		for (int cs = 0; cs < (circleSegments - 1); cs++) {
			unsigned int a = hs * circleSegments + cs;
			unsigned int b = a + 1;
			unsigned int c = a + circleSegments;
			unsigned int d = c + 1;

			indices.push_back(a);
			indices.push_back(d);
			indices.push_back(b);
			if (hs != heightSegments - 1) { // last row only needs one tri
				indices.push_back(a);
				indices.push_back(c);
				indices.push_back(d);
			}
		}
	}
}

void HemisphereGeometry::createVAO() {
	glGenBuffers(1, &vertexBuffer);
	glGenBuffers(1, &elementBuffer);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
		// bind vertex and uv data
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, pointdata.size() * sizeof(point), pointdata.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(point), (GLvoid*)0); // vertex
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(point), (GLvoid*)(sizeof(float) * 3)); // uv
		glEnableVertexAttribArray(1);
		// bind indices
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
	glBindVertexArray(0);
}

