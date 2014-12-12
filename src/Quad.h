/*
* Copyright Tobias Rapp
* 29.05.2014
*/
#ifndef AW_QUAD_H
#define AW_QUAD_H

#include "cpvs.h"

class Quad {
protected:
	enum AttributeLog {
		vPosLoc = 0,
		vTexCoordLoc,
	};

public:
	Quad(vec2 min, vec2 max);
	~Quad();
	
	Quad(const Quad& rhs) = delete;
	Quad& operator=(const Quad& rhs) = delete;

	Quad(Quad&& rhs) = default;
	Quad& operator=(Quad&& rhs) = default;

	void resize(vec2 min, vec2 max);

	void setTextureCoords(vec2 min, vec2 max);
	
	void draw() const;

private:
	void createBuffers();

protected:
	vec2 m_min, m_max;
	vec2 m_texMin, m_texMax;

	GLuint m_vao;
};

#endif
