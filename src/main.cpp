#include <stdio.h>
#include <fstream>

#include <GL/glew.h>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp>

#include <imgui.h>
#include <imgui-SFML.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

struct model {
  std::vector<float> vertices;
  std::vector<GLuint> indices;

  void draw() {
    if (VAO == 0) initVAO();
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  }

private:
  void initVAO() {
    GLuint VBO;
    glGenBuffers(1, &VBO);

    GLuint EBO;
    glGenBuffers(1, &EBO);

    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(),
                vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(),
                indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
  }
  GLuint VAO = 0;
};


struct freecam {
  glm::vec3 pos = glm::vec3(0, 0, -1);
  glm::vec2 rot = glm::vec2(0, 0);

  void update(sf::Window &window) {
    int mouseDeltaX = sf::Mouse::getPosition(window).x - window.getSize().x / 2;
    int mouseDeltaY = sf::Mouse::getPosition(window).y - window.getSize().y / 2;

    rot.x += mouseDeltaX;
    rot.y += mouseDeltaY;

    forward = glm::rotate(glm::vec3(0, 0, 1), rot.y / angleFactor, glm::vec3(1, 0, 0));
    forward = glm::rotate(forward, -rot.x / angleFactor, glm::vec3(0, 1, 0));

    glm::vec3 left = glm::rotate(glm::vec3(0, 0, 1), -rot.x / angleFactor + glm::radians(90.0f), glm::vec3(0, 1, 0));

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
      moveFactor = 200;
    else
      moveFactor = 20;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
      pos += forward / moveFactor;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
      pos -= forward / moveFactor;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
      pos += left / moveFactor;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
      pos -= left / moveFactor;

    limit();
  }

  void limit() {
    rot.x = fmod(rot.x, glm::radians(360.0f) * angleFactor);
    rot.y = fmod(rot.y, glm::radians(360.0f) * angleFactor);
  }

  glm::mat4 getViewMatrix() {
    forward = glm::rotate(glm::vec3(0, 0, 1), rot.y / angleFactor, glm::vec3(1, 0, 0));
    forward = glm::rotate(forward, -rot.x / angleFactor, glm::vec3(0, 1, 0));
    glm::mat4 result = glm::lookAt(pos, pos + forward, up);
    return result;
  }

private:
  glm::vec3 forward = glm::vec3(0, 0, 1);
  glm::vec3 up = glm::vec3(0, 1, 0);

  const float angleFactor = 200;
  float moveFactor = 20;
};


struct arccam {
  glm::vec2 rot = glm::vec2(0, 0);
  float radius = 1;

  void update(sf::Window &window) {
    int mouseDeltaX = sf::Mouse::getPosition(window).x - window.getSize().x / 2;
    int mouseDeltaY = sf::Mouse::getPosition(window).y - window.getSize().y / 2;

    rot.x += mouseDeltaX;
    rot.y += mouseDeltaY;

    limit(-89, 89);
  }

  void limit(float minY, float maxY) {
    float angleX = rot.x / angleFactor;
    float angleY = rot.y / angleFactor;

    rot.x = fmod(rot.x, glm::radians(360.0f) * angleFactor);

    if (angleY > glm::radians(maxY))
      rot.y = glm::radians(maxY) * angleFactor;
    if (angleY < glm::radians(minY))
      rot.y = glm::radians(minY) * angleFactor;
  }

  glm::mat4 getViewMatrix() {
    float angle = rot.y / angleFactor;
  
    float camY = sin(angle) * exp(radius);
    float camZ = cos(angle) * exp(radius);
    glm::mat4 result = glm::lookAt(glm::vec3(0.0, camY, camZ), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    result = glm::rotate(result, rot.x / angleFactor, glm::vec3(0, 1, 0));

    return result;
  }

private:
  const float angleFactor = 200;
};


std::string readFile(std::string filename) {
  std::ifstream ifs(filename, std::ios::binary);
  ifs.seekg(0, ifs.end);
  long length = ifs.tellg();
  ifs.seekg(0, ifs.beg);
  char *buffer = (char*)malloc(length);
  ifs.read(buffer, length);
  ifs.close();
  std::string result(buffer);
  free(buffer);
  return result;
}

model loadModel(const std::string &filename) {
  Assimp::Importer importer;

  const aiScene *scene = importer.ReadFile(
      filename, aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                    aiProcess_SortByPType | aiProcess_GenSmoothNormals);

  model result;

  for (int i = 0; i < scene->mMeshes[0]->mNumVertices; i++) {
    aiVector3D v = scene->mMeshes[0]->mVertices[i];
    aiVector3D n = scene->mMeshes[0]->mNormals[i];
    result.vertices.push_back(v.x);
    result.vertices.push_back(v.y);
    result.vertices.push_back(v.z);
    result.vertices.push_back(n.x);
    result.vertices.push_back(n.y);
    result.vertices.push_back(n.z);
  }

  for (int i = 0; i < scene->mMeshes[0]->mNumFaces; i++) {
    aiFace f = scene->mMeshes[0]->mFaces[i];
    for (int j = 0; j < f.mNumIndices; j++) {
      result.indices.push_back(f.mIndices[j]);
    }
  }

  return result;
}

GLuint compileShaders(const char *vertFilename, const char *fragFilename) {
  GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
  std::string vertSource = readFile(vertFilename);
  const char *vertAddr = vertSource.c_str();
  glShaderSource(vertShader, 1, &vertAddr, NULL);
  glCompileShader(vertShader);

  int success;
  char infoLog[512];
  glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertShader, 512, NULL, infoLog);
    printf("Error compiling vertex shader: %s\n", infoLog);
  }

  GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
  std::string fragSource = readFile(fragFilename);
  const char *fragAddr = fragSource.c_str();
  glShaderSource(fragShader, 1, &fragAddr, NULL);
  glCompileShader(fragShader);

  glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragShader, 512, NULL, infoLog);
    printf("Error compiling fragment shader: %s\n", infoLog);
  }

  // Link Shader Program

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertShader);
  glAttachShader(shaderProgram, fragShader);
  glLinkProgram(shaderProgram);

  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    printf("Error linking shader program: %s\n", infoLog);
  }

  glDeleteShader(vertShader);
  glDeleteShader(fragShader);

  return shaderProgram;
}


int main() {
  // Window Setup

  sf::ContextSettings settings;
  settings.depthBits = 24;
  settings.antialiasingLevel = 0;
  settings.majorVersion = 4;
  settings.minorVersion = 6;

  sf::RenderWindow window(sf::VideoMode(1600, 900), "Subsurface Scattering",
                    sf::Style::Default, settings);
  window.setVerticalSyncEnabled(true);

  ImGui::SFML::Init(window);

  // Initialize GLEW

  if (glewInit() != GLEW_OK) {
  }

  GLuint shaderProgram = compileShaders("shaders/vert.glsl", "shaders/frag.glsl");

  model m = loadModel("models/Isotrop-upperjaw.ply");

  arccam arcCam;
  freecam freeCam;
      
  glm::vec3 lightPos(1.2f, 5.0f, 2.0f);

  // MVP

  glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f, 0.01f, 0.01f));

  glm::mat4 view;

  glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)window.getSize().x / window.getSize().y, 0.001f, 1000.0f);

  struct {
    bool wireframe = false;
    bool freecam = false;
  } options;

  sf::Clock deltaClock;

  bool prevMouse = false;

  bool running = true;
  while (running) {
    // Events

    sf::Event event;
    while (window.pollEvent(event)) {
      ImGui::SFML::ProcessEvent(event);

      if (event.type == sf::Event::EventType::Closed) {
        running = false;
      } else if (event.type == sf::Event::EventType::Resized) {
        glViewport(0, 0, event.size.width, event.size.height);
      } else if (event.type == sf::Event::EventType::KeyReleased) {
        using keys = sf::Keyboard;
        switch (event.key.code) {
        case keys::Escape:
          running = false;
          break;
        }
      } else if (event.type == sf::Event::EventType::MouseWheelScrolled) {
        if (! options.freecam) {
          arcCam.radius -= event.mouseWheelScroll.delta / 5.0f;
        }
      }
    }

    // Update

    if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
      window.setMouseCursorVisible(false);

      if (prevMouse) {
        if (options.freecam)
          freeCam.update(window);
        else
          arcCam.update(window);
      }


      sf::Mouse::setPosition(sf::Vector2i(
          window.getSize().x / 2,
          window.getSize().y / 2
        ), window);  
    } else {
      window.setMouseCursorVisible(true);
    }

    prevMouse = sf::Mouse::isButtonPressed(sf::Mouse::Right);

    // Render

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    if (options.wireframe)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glUseProgram(shaderProgram);
    
    if (options.freecam)
      view = freeCam.getViewMatrix();
    else
      view = arcCam.getViewMatrix();
    
    glUniformMatrix4fv(
      glGetUniformLocation(shaderProgram, "model"),
      1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(
      glGetUniformLocation(shaderProgram, "view"),
      1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(
      glGetUniformLocation(shaderProgram, "projection"),
      1, GL_FALSE, glm::value_ptr(proj));

    glUniform3f(
      glGetUniformLocation(shaderProgram, "objectColor"),
      1.0f, 0.5f, 0.31f);
    glUniform3f(
      glGetUniformLocation(shaderProgram, "lightColor"),
      1.0f, 1.0f, 1.0f);
    glUniform3fv(
      glGetUniformLocation(shaderProgram, "lightPos"),
      1, glm::value_ptr(lightPos));

    m.draw();
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    ImGui::SFML::Update(window, deltaClock.restart());

    ImGui::Begin("Options");
    ImGui::Checkbox("Wireframe", &options.wireframe);
    ImGui::Checkbox("Free Cam", &options.freecam);
    if (options.freecam) {
      ImGui::LabelText("Position", "%f %f %f", freeCam.pos.x, freeCam.pos.y, freeCam.pos.z);
      ImGui::LabelText("Rotation", "%f %f", freeCam.rot.x, freeCam.rot.y);
      if (ImGui::Button("Reset")) {
        freeCam.pos = glm::vec3(0, 0, -1);
        freeCam.rot = glm::vec2(0);
      }
    } else {
      ImGui::LabelText("Rotation", "%f %f", arcCam.rot.x, arcCam.rot.y);
      ImGui::InputFloat("Radius", &arcCam.radius);
      if (ImGui::Button("Reset")) {
        arcCam.rot = glm::vec2(0);
        arcCam.radius = 1;
      }
    }
    ImGui::End();

    ImGui::SFML::Render(window);

    window.display();
  }

  return 0;
}
