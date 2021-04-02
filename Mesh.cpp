#include "Mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION // Define once in a cc/cpp file
#include "tiny_obj_loader.h"

#include <iostream>
#include <fstream>

// Implementation based on example usage here: https://github.com/syoyo/tinyobjloader
void Mesh::LoadFromFile(const char* filepath) {
    filename = std::string(filepath, 0, 100); // max 100 characters for internal file name
    filename = filename.substr(5, filename.size()); // trim the "OBJs/"
    std::cout << filename << std::endl;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
	std::string warn;
    std::string err;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath);

    if (!err.empty()) { // `err` may contain warning message
        std::cerr << err << std::endl;
    }

    if (!ret) {
        exit(EXIT_FAILURE);
    }

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); ++s) {
        // Loop over faces (polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f) {
            int fv = shapes[s].mesh.num_face_vertices[f];

            Triangle t = Triangle();

            // Loop over vertices in the face
            for (size_t v = 0; v < fv; ++v) {
                const unsigned int index = (unsigned int)(index_offset + v);
                tinyobj::index_t idx = shapes[s].mesh.indices[index];
                Vertex newVert;
                newVert.pos = { attrib.vertices[3 * idx.vertex_index], attrib.vertices[3 * idx.vertex_index + 1], attrib.vertices[3 * idx.vertex_index + 2] };
                newVert.nor = { attrib.normals[3 * idx.normal_index], attrib.normals[3 * idx.normal_index + 1], attrib.normals[3 * idx.normal_index + 2] };
                
                // Store data
                vertices.emplace_back(newVert);
                indices.emplace_back(index);
                t.AppendVertex(newVert.pos);
                positions.emplace_back(newVert.pos);
                normals.emplace_back(newVert.nor);
            }
            index_offset += fv;
            t.ComputePlaneNormal();
            triangles.emplace_back(t);
        }
    }
    return;
}

void Mesh::ExportToFile() const {
    std::ofstream outputFile;
    std::string outputFileName = "output_" + filename + ".obj";
    outputFile.open(outputFileName);
    for (unsigned int i = 0; i < (unsigned int)positions.size(); ++i) {
        outputFile << "v ";
        outputFile << positions[i].x << " ";
        outputFile << positions[i].y << " ";
        outputFile << positions[i].z << "\n";
    }
    for (unsigned int i = 0; i < (unsigned int)positions.size(); ++i) {
        outputFile << "vt ";
        outputFile << 0.00000 << " ";
        outputFile << 0.00000 << "\n";
    }
    for (unsigned int i = 0; i < (unsigned int)normals.size(); ++i) {
        outputFile << "vn ";
        outputFile << normals[i].x << " ";
        outputFile << normals[i].y << " ";
        outputFile << normals[i].z << "\n";
    }
    for (unsigned int i = 0; i < (unsigned int)indices.size(); i += 3) {
        outputFile << "f ";
        outputFile << (indices[i] + 1) << "/";
        outputFile << (indices[i] + 1) << "/";
        outputFile << (indices[i] + 1) << "/ ";

        outputFile << (indices[i] + 2) << "/";
        outputFile << (indices[i] + 2) << "/";
        outputFile << (indices[i] + 2) << "/ ";

        outputFile << (indices[i] + 3) << "/";
        outputFile << (indices[i] + 3) << "/";
        outputFile << (indices[i] + 3) << "/\n";
    }
    outputFile.close();
}

// Inherited from Drawable

void Mesh::create() {
    // Indices
    genBufIdx();           
    count = (int)indices.size();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufIdx);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), indices.data(), GL_STATIC_DRAW);

    // Positions
    genBufPos();
    glBindBuffer(GL_ARRAY_BUFFER, bufPos);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * positions.size(), positions.data(), GL_STATIC_DRAW);

	/*
    // Normals
    genBufNor();
    glBindBuffer(GL_ARRAY_BUFFER, bufNor);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * normals.size(), normals.data(), GL_STATIC_DRAW);
	*/
}
