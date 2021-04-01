#pragma once
#include <glm/glm.hpp>

class Line
{
public:
	// ctor/dtor
	Line() : direction(glm::vec3(0, 0, 0)), point(glm::vec3(0, 0, 0)) {}
	Line(const glm::vec3& v, const glm::vec3& p) : direction(v), point(p) {}    // with 3D direction and a point	                             
	~Line() {};

	// getters/setters
	void set(const glm::vec3& v, const glm::vec3& p);               // from 3D
	const glm::vec3& getPoint() const { return point; }
	const glm::vec3& getDirection() const { return direction; }
	// find intersect point with other line
	glm::vec3 intersect(const Line& line);
	bool isIntersected(const Line& line);

protected:

private:
	glm::vec3 direction;
	glm::vec3 point;
};


