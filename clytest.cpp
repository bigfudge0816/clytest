// clytest.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "Pipe.h"
#include "ShaderProgram.h"
#include "Mesh.h"
#include "Camera.h"
#include <Mathematics/BSplineCurveFit.h>


Camera camera = Camera(glm::vec3(0.0f, 0.0f, 0.0f), 0.7853981634f, // 45 degrees vs 75 degrees
(float)VIEWPORT_WIDTH_INITIAL / VIEWPORT_HEIGHT_INITIAL, 0.01f, 2000.0f, 0.0f, -31.74f, 5.4f, VIEWPORT_HEIGHT_INITIAL, VIEWPORT_WIDTH_INITIAL);
const float camMoveSensitivity = 0.03f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	camera.SetAspect((float)width / height);
	camera.SetViewportHeight(height);
	camera.SetViewportWidth(width);
}


// Keyboard controls
void processInput(GLFWwindow *window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		camera.TranslateAlongRadius(-camMoveSensitivity);
	}
	else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		camera.TranslateAlongRadius(camMoveSensitivity);
	}
	else if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		camera.RotatePhi(-camMoveSensitivity);
	}
	else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		camera.RotatePhi(camMoveSensitivity);
	}
	else if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		camera.RotateTheta(-camMoveSensitivity);
	}
	else if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		camera.RotateTheta(camMoveSensitivity);
	}
	else if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		camera.TranslateRefAlongWorldY(-camMoveSensitivity);
	}
	else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		camera.TranslateRefAlongWorldY(camMoveSensitivity);
	}
}

int main()
{
	// GLFW Window Setup
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef ENABLE_MULTISAMPLING
	glfwWindowHint(GLFW_SAMPLES, 4);
#endif

	GLFWwindow* window = glfwCreateWindow(1280, 720, "Pipe", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize Glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	glViewport(0, 0, 1280, 720);
	// Set window callbacks - must happen after setting up Imgui for some reason
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	ShaderProgram sp = ShaderProgram("point-vert.vert", "point-frag.frag");
	// Array/Buffer Objects
	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// TODO Create an init() function
	glPointSize(2);
	glLineWidth(2);
	glEnable(GL_DEPTH_TEST);

#ifdef ENABLE_MULTISAMPLING
	glEnable(GL_MULTISAMPLE);
#endif

	Pipe pipe;
	//pipe.buildCircle(1, 10);

	pipe.buildPath(1, 5, 0.9, 1.1, acos(-1) / 6, 1, 1, 1, 1, 0.17, true);
	std::vector<glm::vec3> path = pipe.getPathPoints();


	std::vector<glm::vec3> branchMeshPoints = std::vector<glm::vec3>();
	std::vector<glm::vec3> branchMeshNormals = std::vector<glm::vec3>();
	std::vector<unsigned int> branchMeshIndices = std::vector<unsigned int>();
	
	std::unique_ptr<gte::BSplineCurveFit<float>> mSpline;
	mSpline = std::make_unique<gte::BSplineCurveFit<float>>(3, static_cast<int>(path.size()),
		reinterpret_cast<float const*>(&path[0]), 3, 20);
	float multiplier = 1.0f / (path.size() - 1.0f);
	for (int i = 0; i < path.size(); i++)
	{
		float t = multiplier * i;
		mSpline->GetPosition(t, reinterpret_cast<float*>(&path[i].x));
		glm::vec3 dir;
		if (i < path.size()-1)
			dir = path[i + 1] - path[i];		
		else
			dir =  path[i] - path[i-1];

		dir = glm::normalize(dir);
		pipe.buildCircle(pipe.getRadius()[i], 10, path[i],dir);
		branchMeshPoints.insert(branchMeshPoints.end(),pipe.getContour(i).begin(), pipe.getContour(i).end());
	}

	for (int i = 0; i < branchMeshPoints.size()-10; i++)
	{
		branchMeshIndices.emplace_back(i);
		branchMeshIndices.emplace_back(i+10);
		if ((i + 1) % 10 == 0)
		{
			branchMeshIndices.emplace_back(i-9);
			branchMeshIndices.emplace_back(i+1);
		}
	}

	Mesh m = Mesh();
	m.AddIndices(branchMeshIndices);
	//m.AddNormals(branchMeshNormals);
	m.AddPositions(branchMeshPoints);
	m.create();
	
	// Render loop
	while (!glfwWindowShouldClose(window)) {
		processInput(window);

		glClearColor(0.5, 0.5, 0.6, 1.0);
		glClearDepth(1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Draw scene

		//sp2.setCameraViewProj("cameraViewProj", camera.GetViewProj());
		sp.setCameraViewProj("cameraViewProj", camera.GetViewProj());
		sp.setUniformColor("u_color", glm::vec3(0.467f, 0.41f, 0.25f));
		sp.Draw(m);

		//drawPath();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &VAO);
	glfwTerminate();

	return 0;
}


