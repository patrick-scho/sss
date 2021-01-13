#include <stdio.h>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


const char *vertexShaderSource =
"#version 330 core\n"
"layout (location = 0) in vec3 pos;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"\n"
"void main()\n"
"{\n"
"    gl_Position = projection * view * model * vec4(pos, 1.0);\n"
"}\n";

const char *fragmentShaderSource =
"#version 330 core\n"
"out vec4 FragColor;\n"
"\n"
"void main()\n"
"{\n"
"    FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);\n"
"}\n";

std::vector<float> vertices;
std::vector<GLuint> indices;


void load(const std::string &filename, std::vector<float> &vertices, std::vector<GLuint> &indices) {
  Assimp::Importer importer;

  const aiScene *scene = importer.ReadFile(
      filename,
      aiProcess_CalcTangentSpace |
      aiProcess_Triangulate |
      aiProcess_JoinIdenticalVertices |
      aiProcess_SortByPType
  );

  for (int i = 0; i < scene->mMeshes[0]->mNumVertices; i++) {
    aiVector3D v = scene->mMeshes[0]->mVertices[i];
    vertices.push_back(v.x);
    vertices.push_back(v.y);
    vertices.push_back(v.z);
  }

  for (int i = 0; i < scene->mMeshes[0]->mNumFaces; i++) {
    aiFace f = scene->mMeshes[0]->mFaces[i];
    for (int j = 0; j < f.mNumIndices; j++) {
      indices.push_back(f.mIndices[j]);
    }
  }
}

int main() {
  // Window Setup

  sf::ContextSettings settings;
  settings.depthBits = 24;
  settings.antialiasingLevel = 0;
  settings.majorVersion = 4;
  settings.minorVersion = 6;

  sf::Window window(sf::VideoMode(800, 600), "Subsurface Scattering",
                    sf::Style::Default, settings);
  window.setVerticalSyncEnabled(true);

  window.setActive(true);

  // Initialize GLEW

  if (glewInit() != GLEW_OK) {

  }

  load("models/Isotrop-upperjaw.ply", vertices, indices);

  // Compile Shaders

  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  int  success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
      glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
      printf("Error compiling vertex shader: %s\n", infoLog);
  }

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success) {
      glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
      printf("Error compiling fragment shader: %s\n", infoLog);
  }

  // Link Shader Program

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
      glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
      printf("Error linking shader program: %s\n", infoLog);
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // Create VBO

  GLuint VBO;
  glGenBuffers(1, &VBO);

  // Create EBO

  GLuint EBO;
  glGenBuffers(1, &EBO);
  
  // Create VAO

  GLuint VAO;
  glGenVertexArrays(1, &VAO);

  glBindVertexArray(VAO);
  
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), indices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, NULL);
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);

  // Perspective

  glm::mat4 model = glm::mat4(1.0f);

  glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -200.0f));

  glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 1000.0f);

  bool wireframe = false;

  bool running = true;
  while (running) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        running = false;
      } else if (event.type == sf::Event::Resized) {
        glViewport(0, 0, event.size.width, event.size.height);
      } else if (event.type == sf::Event::KeyReleased) {
        using keys = sf::Keyboard;
        switch (event.key.code) {
        case keys::W:
          wireframe = !wireframe;
          break;
        case keys::Escape:
          running = false;
          break;
        }
      }
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    if (wireframe)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    model = glm::rotate(model, glm::radians(0.2f), glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    window.display();
  }

  return 0;
}