

#include "Pipe.h"
#include "Line.h"
#include "Plane.h"
#include <Mathematics/BSplineCurveFit.h>

///////////////////////////////////////////////////////////////////////////////
// ctors
///////////////////////////////////////////////////////////////////////////////
Pipe::Pipe()
{
}
///////////////////////////////////////////////////////////////////////////////
// setters
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// generate a spiral path along y-axis
// R: starting radius
// N: the parameter defining the taper distribution
// M: the parameter defining the mass distribution
// Phi: an angle Phi from vertical
// bH: 
// E: the homogenized modulus of elasticity
// L: the ratio between the total mass of the tree Mtot and the stem volume V
// f: the form factor
void Pipe::buildPath(float R, float H, float N, float M, float Phi, float bH, float E, float L, float f, float deltaA, bool plantType)
{
	// reset
	this->path.clear();
	contours.clear();
	normals.clear();

	std::vector<glm::vec3> vertices;
	std::vector<float> radius;
	glm::vec3 vertex(0, 0, 0);
	vertices.emplace_back(vertex);

	float S = 0;//distance to base
	float currentR = 0;
	float curvature = 0;
	float er = 0;
	int Num = 200;
	radius.emplace_back(R);
	for (int i = 1; i < Num ; i++)
	{
		//calculate the  point's position
		float distanceX2 = pow(H / Num, 2) / (pow(tan(curvature * H / Num), 2) + 1);

		vertex.x = sqrt(distanceX2) + vertex.x;
		vertex.y = sqrt(pow(H / Num, 2) - distanceX2) + vertex.y;
		vertices.emplace_back(vertex);
		if (plantType) f += 0.1 / Num;
		else f -= 0.1 / Num;
		er = deltaA * f;
		S += H / Num;
		currentR = R * pow(1 - S / H / 3, N);
		radius.emplace_back(currentR);
		float deltaC = -2 * er / currentR / currentR;
		deltaC += 8 * GRAVITY * L * sin(Phi) * (1 + bH) * pow(H, 2) / (E * (M + 1)*(2 * N + 1) * pow(currentR, 3));
		deltaC *= radius[i - 1] - radius[i];
		curvature += deltaC;	
	}
		
	std::unique_ptr<gte::BSplineCurveFit<float>> mSpline;
	mSpline = std::make_unique<gte::BSplineCurveFit<float>>(3, static_cast<int>(vertices.size()),
		reinterpret_cast<float const*>(&vertices[0]), 3, 20);
	float multiplier = 1.0f / (vertices.size() - 1.0f);
	for (int i = 0; i < vertices.size(); i++)
	{
		float t = multiplier * i;
		mSpline->GetPosition(t, reinterpret_cast<float*>(&vertices[i].x));
		if (i == 0)
			tangents.emplace_back(vertices[i + 1] - vertices[i]);
		else if (i == vertices.size() - 1)
			tangents.emplace_back(vertices[i] - vertices[i - 1]);
		else
			tangents.emplace_back(vertices[i + 1] - vertices[i - 1]);
		
		tangents[i] = glm::normalize(tangents[i]);
		Plane plane(tangents[i], vertices[i]);
		glm::vec3 y(0, vertices[i].y, 0);
		glm::vec3 temp = glm::dot(y, tangents[i])*tangents[i];
		normals.emplace_back(glm::normalize(y - temp));
		bvectors.emplace_back(glm::cross(tangents[i], normals[i]));

		glm::mat3 matrix(normals[i], bvectors[i], vertices[i]);
		std::vector<glm::vec3> tempcontour;
		std::vector<glm::vec3> tempnormal;
		for (int j = 0; j < contour.size(); j++)
		{
			glm::vec3 p = matrix * contour[j];
			glm::mat4 trans(1.0);
			trans = glm::scale(trans, glm::vec3(radius[i] / radius[0], radius[i] / radius[0], radius[i] / radius[0]));
			p = glm::vec3(trans * glm::vec4(p, 1));
			tempcontour.emplace_back(p);
			tempnormal.push_back(glm::normalize(contour[j] - vertices[i]));
		}
		normals_contours.emplace_back(tempnormal);
		contours.emplace_back(tempcontour);
	}
	this->path = vertices;
}

void Pipe::buildCircle(float radius, int steps)
{
	this->contour.clear();
	std::vector<glm::vec3> points;
	if (steps < 2) return ;

	const float PI2 = acos(-1) * 2.0f;
	float x, y, a;
	for (int i = 0; i <= steps; ++i)
	{
		a = PI2 / steps * i;
		x = radius * cosf(a);
		y = radius * sinf(a);
		points.push_back(glm::vec3(x, y, 1));
	}
	this->contour = points;
}



///////////////////////////////////////////////////////////////////////////////
// project a contour to a plane at the path point
///////////////////////////////////////////////////////////////////////////////
std::vector<glm::vec3> Pipe::projectContour(int fromIndex, int toIndex)
{
	glm::vec3 dir1, dir2, normal;
	Line line;
	
	dir1 = path[toIndex] - path[fromIndex];
	if (toIndex == (int)path.size() - 1)
		dir2 = dir1;
	else
		dir2 = path[toIndex + 1] - path[toIndex];

	normal = dir1 + dir2;               // normal vector of plane at toIndex
	Plane plane(normal, path[toIndex]);

	// project each vertex of contour to the plane
	std::vector<glm::vec3>& fromContour = contours[fromIndex];
	std::vector<glm::vec3> toContour;
	int count = (int)fromContour.size();
	for (int i = 0; i < count; ++i)
	{
		line.set(dir1, fromContour[i]);
		toContour.push_back(plane.intersect(line));
	}

	return toContour;
}



///////////////////////////////////////////////////////////////////////////////
// transform the contour at the first path point
///////////////////////////////////////////////////////////////////////////////
void Pipe::transformFirstContour()
{
	int pathCount = (int)path.size();
	int vertexCount = (int)contour.size();
	glm::mat4 matrix(1.0f);
	if (pathCount > 0)
	{
		// transform matrix
		if (pathCount > 1)
			matrix = { 0,0,-1,0,
					   0,1,0,0,
					   1,0,0,0,
					   0,0,0,1 };
		
		// multiply matrix to the contour
		// NOTE: the contour vertices are transformed here
		//       MUST resubmit contour data if the path is resset to 0
		
		for (int i = 0; i < vertexCount; ++i)
		{	
			contour[i] = glm::vec3(matrix * glm::vec4(contour[i], 1));
		}
	}
}



///////////////////////////////////////////////////////////////////////////////
// return normal vectors at the current path point
///////////////////////////////////////////////////////////////////////////////
std::vector<glm::vec3> Pipe::computeContourNormal(int pathIndex)
{
	// get current contour and center point
	std::vector<glm::vec3>& contour = contours[pathIndex];
	glm::vec3 center = path[pathIndex];

	std::vector<glm::vec3> contourNormal;
	glm::vec3 normal;
	for (int i = 0; i < (int)contour.size(); ++i)
	{
		normal = glm::normalize(contour[i] - center);
		contourNormal.push_back(normal);
	}

	return contourNormal;
}
