

#include "Pipe.h"
#include "Line.h"
#include "Plane.h"
#include <Mathematics/BSplineCurveFit.h>
#include <Mathematics/BSplineCurve.h>

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
	glm::vec3 vertex(0, 0, 0);
	vertices.emplace_back(vertex);

	float S = 0;//distance to base
	float currentR = R;
	float curvature = 0;
	float er = 0;
	int Num = 30;
	radius.emplace_back(R);
	glm::vec3 dir(1, 1, 0);
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
	this->path = vertices;
}

void Pipe::buildCircle(float radius, int mNumCtrl,glm::vec3 point, glm::vec3 dir)
{
	/*this->contour.clear();
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
	this->contour = points;*/
	this->contour.clear();
	std::vector<glm::vec3> positions;
	int mDegree = 3;
	float mA = radius, mB = radius;
	std::unique_ptr<gte::BSplineCurve<3, float>> mSkirtTop;
	
	glm::vec3 yaxis{ 0,1,0 };
	float snphi = glm::dot(dir, yaxis);

	for (int i = 0; i < mNumCtrl; ++i)
	{
		float ratio = static_cast<float>(i) / static_cast<float>(mNumCtrl);
		float angle = ratio * static_cast<float>(GTE_C_TWO_PI);
		float sn = std::sin(angle);
		float cs = std::cos(angle);
		float v = 1.0f - std::fabs(2.0f * ratio - 1.0f);

		// Set a vertex for the skirt top.
		positions.emplace_back(glm::vec3(mA*cs + point.x, 0.0 + point.y, mB * sn + point.z));
	}
	this->contour = positions;
	this->contours.emplace_back(this->contour);
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
