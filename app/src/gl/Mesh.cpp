#include "src/gl/Mesh.h"

#include <iostream>

// We use the tiny_obj_loader library to load data of .obj-files into this mesh:
#define TINYOBJLOADER_IMPLEMENTATION

// We want to use triangulation when loading non-triangulated polygons:
#define TINYOBJLOADER_USE_MAPBOX_EARCUT

// Include tiny_obj_loader:
#include "src/tiny_obj_loader/tiny_obj_loader.h"

/**
 * PREV IMPLEMENTATION
 *
 * NOTE: If you want to use your own implementation, you have to
 * create a fourth VBO for 2D texture coordinates (see Vertex.h)
 * with an index of 3.
 */
Mesh::Mesh(std::vector<Triangle> triangles)
    : triangles(triangles)
    , numOfCopies(new int(1))
{
    createBuffers();
}

void Mesh::createBuffers(){
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &vio);

    std::vector<Vertex> vboData;

    for(unsigned int i=0; i < triangles.size(); ++i){
        vboData.push_back(*(&triangles[i].a));
        vboData.push_back(*(&triangles[i].b));
        vboData.push_back(*(&triangles[i].c));
    }

    std::vector<unsigned int> indices;

    for(unsigned int i=0; i < vboData.size(); ++i){
        indices.push_back(i);
    }

    verticesNum = (unsigned int)vboData.size();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vio);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(4), &indices[0], GL_STATIC_DRAW );
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vboData.size() * sizeof(Vertex), &vboData[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)sizeof(Vec4f));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2*sizeof(Vec4f)));
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(Vec4f)));

    for(int i=0;i<4;++i)
        glEnableVertexAttribArray(i);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

/* This contructor loads an obj file into the VAO and VBO (see header) */
Mesh::Mesh(std::string filepath)
    : numOfCopies(new int(1))
{
    // Define buffers to load the data:
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    // Extract the directory path from the filepath (to locate .mtl):
    std::string dir_path = filepath.substr(0, filepath.find_last_of("/\\"));

    // Try to load the obj file into:
    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(), dir_path.c_str())){
        std::cerr << "Obj-file " << filepath << " could not be loaded. Check if file is available." << std::endl;
    }

    if (!warn.empty()) {
        std::cout << "Warning while loaded obj-file " << filepath << ":" << std::endl;
        std::cout << warn << std::endl;
    }
    if (!err.empty()) {
        std::cout << "Error while loaded obj-file " << filepath << ":" << std::endl;
        std::cerr << err << std::endl;
    }

    // Loop over shapes of the obj:
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces of that shapes:
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            int fv = shapes[s].mesh.num_face_vertices[f];

            // Create a triangle for our mesh:
            Triangle triangle;

            // Loop over vertices in the face.
            for (int v = 0; v < fv; v++) {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                // Copy vertex position into triangle:
                std::memcpy(&triangle[v].position, &attrib.vertices[3*size_t(idx.vertex_index)], sizeof(float) * 3);

                // Copy vertex normal into triangle if available:
                if(idx.normal_index >= 0 && attrib.normals.size() > 3*size_t(idx.normal_index)){
                    std::memcpy(&triangle[v].normal, &attrib.normals[3*size_t(idx.normal_index)], sizeof(float) * 3);
                    triangle[v].normal = triangle[v].normal.normalized();
                }

                // Copy vertex texcoord into triangle if available:
                if(idx.texcoord_index >= 0)
                    std::memcpy(&triangle[v].uv, &attrib.texcoords[2*size_t(idx.texcoord_index)], sizeof(float) * 2);

                // Copy vertex color into triangle:
                std::memcpy(&triangle[v].color, &attrib.colors[3*size_t(idx.vertex_index)], sizeof(float) * 3);
            }
            index_offset += fv;

            triangles.push_back(triangle);
        }
    }

    // This method creates VAO and VBOs like you have it done
    // in the previous exercise:
    createBuffers();
}

/**
 * This is an explicit copy constructor which is automatically
 * called every time an existing Mesh is assigned to a variable
 * via the = Operator. Not important for CG1 and C++ specific,
 * but it is needed here to avoid problems because of early
 * deletion of the shaderProgram when the destructor is called.
 */
Mesh::Mesh(const Mesh& mesh){
    vao = mesh.vao;
    vbo = mesh.vbo;
    vio = mesh.vio;
    verticesNum = mesh.verticesNum;
    triangles = mesh.triangles;
    numOfCopies = mesh.numOfCopies;
    ++(*numOfCopies);
}

Mesh::~Mesh(){
    // We only want the VAO and VBO to be destroyed
    // when the last copy of the mesh is deleted:
    if(--(*numOfCopies) > 0)
        return;

    // If there is a vao, delete it from the GPU:
    if(vao != 0)
        glDeleteVertexArrays(1, &vao);

    // Same for the vbo:
    if(vbo != 0)
        glDeleteBuffers(1, &vbo);

    // Same for the vio:
    if(vio != 0)
        glDeleteBuffers(1, &vio);

    delete numOfCopies;
}

void Mesh::render(){

    // Draws the geometry data that is defined by the vao:
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vio);
    glDrawElements(GL_TRIANGLES,verticesNum,GL_UNSIGNED_INT,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

std::vector<Triangle>& Mesh::getTriangles(){
    return triangles;
}
