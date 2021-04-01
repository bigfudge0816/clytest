#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

const float GRAVITY = 9.80;
const int CIRCLE_SECTORS = 12;

class Pipe
{
public:
	// ctor/dtor
	Pipe();
	Pipe(const std::vector<glm::vec3>& pathPoints, const std::vector<glm::vec3>& contourPoints);
	~Pipe() {}

	// setters/getters
	
	int getPathCount() const { return (int)path.size(); }
	const std::vector<glm::vec3>& getPathPoints() const { return path; }
	const glm::vec3& getPathPoint(int index) const { return path.at(index); }
	int getContourCount() const { return (int)contours.size(); }

	const std::vector< std::vector<glm::vec3> >& getContours() const { return contours; }
	const std::vector<glm::vec3>& getContour(int index) const { return contours.at(index); }
	const std::vector<glm::vec3>& getNormal(int index) const { return normals_contours.at(index); }
	const std::vector<glm::vec3>& getPath() const { return path; };

	// member functions
	void buildPath(float R, float H, float N, float M, float Phi, float bH, float E, float L, float f, float deltaA, bool plantType);
	void buildCircle(float radius, int steps);
	void transformFirstContour();
	std::vector<glm::vec3> projectContour(int fromIndex, int toIndex);
	std::vector<glm::vec3> computeContourNormal(int pathIndex);
private:
	std::vector<glm::vec3> path;
	std::vector<glm::vec3> contour;
	std::vector< std::vector<glm::vec3> > contours;
	std::vector< std::vector<glm::vec3> > normals_contours;

	std::vector<glm::vec3> tangents;
	std::vector<glm::vec3> bvectors;
	std::vector<glm::vec3> normals;
};
