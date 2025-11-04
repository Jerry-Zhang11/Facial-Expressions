#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "tiny_obj_loader.h"
#include <unistd.h>

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION

static unsigned int ss_id = 0;
void dump_framebuffer_to_ppm(std::string prefix, unsigned int width, unsigned int height) {
    
    int pixelChannel = 3;
    GLubyte* pixels = new GLubyte[width * height * pixelChannel];
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    std::string file_name = prefix + std::to_string(ss_id) + ".ppm";

    std::ofstream fout(file_name);
    fout << "P3\n" << width << " " << height << "\n" << 255 << std::endl;
    for (size_t i = 0; i < height; i++)
    {
        for (size_t j = 0; j < width; j++)
        {
            size_t cur = pixelChannel * ((height - i - 1) * width + j);
            fout << (int)pixels[cur] << " " << (int)pixels[cur + 1] << " " << (int)pixels[cur + 2] << " ";
        }
        fout << std::endl;
    }
    ss_id ++;
    delete [] pixels;
    fout.flush();
    fout.close();
}

//key board control
void processInput(GLFWwindow *window)
{
    //press escape to exit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    //press p to capture screen
    if(glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        std::cout << "Capture Window " << ss_id << std::endl;
        int buffer_width, buffer_height;
        glfwGetFramebufferSize(window, &buffer_width, &buffer_height);
        dump_framebuffer_to_ppm("/Users/jerryzhang/Desktop/oct1", buffer_width, buffer_height);

    }
}


int main(){
    
    std::vector<std::vector<tinyobj::shape_t>> shapes(36);
    std::vector<tinyobj::attrib_t> attribs(36);
    
    
    // load data from the obj file
    std::string obj_path = "/Users/jerryzhang/Desktop/data/faces/base.obj";
    std::vector<tinyobj::material_t> materials;
    // tinyobj load obj
    std::string warn, err;
    bool bTriangulate = true;
    bool bSuc = tinyobj::LoadObj(&attribs[0], &shapes[0], &materials, &warn, &err,
     obj_path.c_str(), nullptr, bTriangulate);
    if(!bSuc)
    {
     std::cout << "tinyobj error: " << err.c_str() << std::endl;
     return -1;
    }
    
    for(int i = 0; i < 35; i++){
        // load data from the obj file
        std::string obj_path = "/Users/jerryzhang/Desktop/data/faces/" + std::to_string(i) + ".obj";
        std::vector<tinyobj::material_t> materials;
        // tinyobj load obj
        std::string warn, err;
        bool bTriangulate = true;
        bool bSuc = tinyobj::LoadObj(&attribs[i+1], &shapes[i+1], &materials, &warn, &err,
         obj_path.c_str(), nullptr, bTriangulate);
        if(!bSuc)
        {
         std::cout << "tinyobj error: " << err.c_str() << std::endl;
         return -1;
        }
    }
    
    
    std::vector<std::vector<float>> vbuffers(36);
    std::vector<std::vector<float>> nbuffers(36);
    
    for(int i = 0; i < 36; i++){
        for(auto face : shapes[i][0].mesh.indices)
        {
            int vid = face.vertex_index;
            int nid = face.normal_index;
            //fill in vertex buffer with vertex positions
            vbuffers[i].push_back(attribs[i].vertices[vid*3]);//vertex vid’s x
            vbuffers[i].push_back(attribs[i].vertices[vid*3+1]);//vertex vid’s y
            vbuffers[i].push_back(attribs[i].vertices[vid*3+2]);//vertex vid’s z
            //fill in normal buffer with normal directions
            nbuffers[i].push_back(attribs[i].normals[nid*3]);
            nbuffers[i].push_back(attribs[i].normals[nid*3+1]);
            nbuffers[i].push_back(attribs[i].normals[nid*3+2]);
        }
    }
    
    std::ifstream infile;
    infile.open("/Users/jerryzhang/Desktop/data/weights/5.weights");
    std::string input;
    //read data from .weights and transfer string to float
    std::vector<float> weights;
    while (getline(infile, input)) {
    weights.push_back(std::stof(input));
    }
    
    
    
    
    std::vector<float> blendVertices = vbuffers[0];
    for(unsigned i = 0; i < weights.size(); i++){
        assert(vbuffers[0].size() == vbuffers[i+1].size());
        for(unsigned j = 0; j < vbuffers[0].size(); j++){
            blendVertices[j] += weights[i] * (vbuffers[i+1][j] - vbuffers[0][j]);
        }
    }
    
    glfwInit();
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(1024, 768, "Cube and Face", NULL, NULL);
    
    if (window == NULL){
        std::cout << "Fail to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    gladLoadGL();
    
    // Define the size of the window
    glViewport(0,0,1024,768);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // Set back buffer to the color we want to display, then swap back buffer with front buffer
    glClearColor(0.3f,0.4f,0.5f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);
    
    const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;

    uniform mat4 MVP;
    
    out vec3 Normal;

    void main() {
        gl_Position = MVP * vec4(aPos, 1.0);
        Normal = aNormal;
    }
    )";

    const char *fragmentShaderSource = R"(
    #version 330 core
    in vec3 Normal;
    out vec3 FragColor;
    void main()
    {
    float c = dot(Normal, vec3(0.0,0.0,1.0));
    FragColor = vec3(c,c,c);
    }
    )";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader); // Already in program, can be deleted now
    
    GLuint VAO, VBO_vertices, VBO_normals;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO_vertices);
    glGenBuffers(1, &VBO_normals);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_vertices);
    glBufferData(GL_ARRAY_BUFFER, blendVertices.size() * sizeof(float), blendVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_normals);
    glBufferData(GL_ARRAY_BUFFER, nbuffers[0].size() * sizeof(float), nbuffers[0].data(), GL_STATIC_DRAW);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //Set up transformations
    glm::mat4 view = glm::lookAt(glm::vec3(0,0.1,0.5), glm::vec3(0,0,0), glm::vec3(0,1,0));
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), 4.0f/3.0f, 0.1f, 10.0f);
    glm::mat4 mvp = projection * view;
    
    // Control render mode: solid or wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    while(!glfwWindowShouldClose(window)){
        processInput(window);
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glUseProgram(shaderProgram);
        GLuint MatrixID = glGetUniformLocation(shaderProgram, "MVP");
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);  // MVP in vertex shader is mvp
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vbuffers[0].size() / 3));
        
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    
    glDeleteProgram(shaderProgram);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
