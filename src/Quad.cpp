/*
* Copyright Tobias Rapp
* 29.05.2014
*/
#include "Quad.h"

Quad::Quad(vec2 pmin, vec2 pmax)
	: m_min(pmin), m_max(pmax), m_texMin(0.0, 0.0), m_texMax(1.0, 1.0)
{
	createBuffers();
}

Quad::~Quad() {
	glDeleteVertexArrays(1, &m_vao);
}

void Quad::resize(vec2 pmin, vec2 pmax)
{
	assert(m_min != m_max);
	m_min = pmin;
	m_max = pmax;

	glDeleteVertexArrays(1, &m_vao);
	createBuffers();
}

void Quad::setTextureCoords(vec2 min, vec2 max)
{
	assert(m_min != m_max);
	m_texMin = min;
	m_texMax = max;
}

void Quad::draw()
{
	GL_CHECK_ERROR("Quad::draw() - begin");
	glBindVertexArray(m_vao);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glBindVertexArray(0);
	GL_CHECK_ERROR("Quad::draw() - end");
}

void Quad::createBuffers() {
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	GLuint vbo, tbo;

	vector<vec3> vertices;
	vertices.push_back(vec3(m_min.x, m_min.y, 0.0f));
	vertices.push_back(vec3(m_max.x, m_min.y, 0.0f));
	vertices.push_back(vec3(m_max.x, m_max.y, 0.0f));
	vertices.push_back(vec3(m_min.x, m_max.y, 0.0f));

	vector<vec2> texs;
	texs.push_back(vec2(m_texMin.x, m_texMin.y));
	texs.push_back(vec2(m_texMax.x, m_texMin.y));
	texs.push_back(vec2(m_texMax.x, m_texMax.y));
	texs.push_back(vec2(m_texMin.x, m_texMax.y));

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT) * 3,
		(GLvoid*) vertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(vPosLoc);
	glVertexAttribPointer(vPosLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &tbo);
	glBindBuffer(GL_ARRAY_BUFFER, tbo);

	glBufferData(GL_ARRAY_BUFFER, texs.size() * sizeof(GL_FLOAT) * 2,
		(GLvoid*) texs.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(vTexCoordLoc);
	glVertexAttribPointer(vTexCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
