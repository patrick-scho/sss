#include <stdio.h>

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

const char *vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
  gl_Position = projection * view * model * vec4(pos, 1.0);
  FragPos = vec3(model * vec4(pos, 1));
  Normal = normal;
}
)";

const char *fragmentShaderSource = R"(
#version 330 core

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;

void main()
{
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(lightPos - FragPos);

  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = diff * lightColor;

  float ambientStrength = 0.1;
  vec3 ambient = ambientStrength * lightColor;

  vec3 result = (ambient + diffuse) * objectColor;
  FragColor = vec4(result, 1.0f);
}
)";

std::vector<float> vertices;
std::vector<GLuint> indices;

glm::vec3 lightPos(1.2f, 5.0f, 2.0f);

void load(const std::string &filename, std::vector<float> &vertices,
          std::vector<GLuint> &indices) {
  Assimp::Importer importer;

  const aiScene *scene = importer.ReadFile(
      filename, aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                    aiProcess_SortByPType | aiProcess_GenSmoothNormals);

  for (int i = 0; i < scene->mMeshes[0]->mNumVertices; i++) {
    aiVector3D v = scene->mMeshes[0]->mVertices[i];
    aiVector3D n = scene->mMeshes[0]->mNormals[i];
    vertices.push_back(v.x);
    vertices.push_back(v.y);
    vertices.push_back(v.z);
    vertices.push_back(n.x);
    vertices.push_back(n.y);
    vertices.push_back(n.z);
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

  sf::RenderWindow window(sf::VideoMode(1600, 900), "Subsurface Scattering",
                    sf::Style::Default, settings);
  window.setVerticalSyncEnabled(true);
  window.setMouseCursorGrabbed(true);
  window.setMouseCursorVisible(false);

  ImGui::SFML::Init(window);

  // Initialize GLEW

  if (glewInit() != GLEW_OK) {
  }

  load("models/Isotrop-upperjaw.ply", vertices, indices);

  // Compile Shaders

  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  int success;
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

  // Perspective

  glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f, 0.01f, 0.01f));

  struct {
    float camX = 0;
    float camZ = -5;
    int mouseX = 0;
    int mouseY = 0;
  } arcball;

  struct {
    int mouseX = 0;
    int mouseY = 0;
  } freecam;

  glm::vec3 camPos = glm::vec3(0, 0, -3);
  glm::vec3 camForward = glm::vec3(0, 0, 1);
  glm::vec3 camUp = glm::vec3(0, 1, 0);

  glm::mat4 view =
      glm::lookAt(glm::vec3(arcball.camX, 0.0, arcball.camZ), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

  glm::mat4 proj =
      glm::perspective(glm::radians(45.0f), (float)window.getSize().x / window.getSize().y, 0.001f, 1000.0f);

  struct {
    bool catchMouse = false;
    bool wireframe = false;
    bool freecam = false;
    float radius = 1.0f;
  } options;

  sf::Clock deltaClock;

  bool running = true;
  while (running) {
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
        case keys::C:
          options.catchMouse = !options.catchMouse;
          break;
        case keys::R:
          freecam.mouseX = freecam.mouseY = 0;
          break;
        }
      } else if (event.type == sf::Event::EventType::MouseWheelScrolled) {
        options.radius -= event.mouseWheelScroll.delta / 5.0f;
      }
    }

    int mouseDeltaX = sf::Mouse::getPosition(window).x - window.getSize().x / 2;
    int mouseDeltaY = sf::Mouse::getPosition(window).y - window.getSize().y / 2;

    if (options.catchMouse) {
      sf::Mouse::setPosition(sf::Vector2i(
        window.getSize().x / 2,
        window.getSize().y / 2
      ), window);

      if (options.freecam) {
        freecam.mouseX += mouseDeltaX;
        freecam.mouseY += mouseDeltaY;

        camForward = glm::rotate(glm::vec3(0, 0, 1), freecam.mouseY / 500.0f, glm::vec3(1, 0, 0));
        camForward = glm::rotate(camForward, -freecam.mouseX / 500.0f, glm::vec3(0, 1, 0));

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
          camPos += camForward / 20.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
          camPos -= camForward / 20.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
          glm::vec3 camLeft = glm::rotate(glm::vec3(0, 0, 1), -freecam.mouseX / 500.0f + glm::radians(90.0f), glm::vec3(0, 1, 0));
          camPos += camLeft / 20.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
          glm::vec3 camRight = glm::rotate(glm::vec3(0, 0, 1), -freecam.mouseX / 500.0f - glm::radians(90.0f), glm::vec3(0, 1, 0));
          camPos += camRight / 20.0f;
        }
      } else {
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
          arcball.mouseX += mouseDeltaX;
          arcball.mouseY += mouseDeltaY;
        }
      }
    }



    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    if (options.wireframe)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glUseProgram(shaderProgram);

    //rotate
    //model = glm::rotate(model, glm::radians(0.2f), glm::vec3(0.0f, 1.0f, 0.0f));

    if (options.freecam) {
      view = glm::lookAt(camPos, camPos + camForward, camUp);
    } else {
      float angle = arcball.mouseY / 200.0f;
      if (angle > glm::radians(89.0f)) {
        angle = glm::radians(89.0f);
        arcball.mouseY = angle * 200.0f;
      }
      if (angle < glm::radians(-89.0f)) {
        angle = glm::radians(-89.0f);
        arcball.mouseY = angle * 200.0f;
      }
      arcball.camX = sin(angle) * exp(options.radius);
      arcball.camZ = cos(angle) * exp(options.radius);
      view = glm::lookAt(glm::vec3(0.0, arcball.camX, arcball.camZ), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
      view = glm::rotate(view, arcball.mouseX / 100.0f, glm::vec3(0, 1, 0));
    }

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1,
                       GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE,
                       glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1,
                       GL_FALSE, glm::value_ptr(proj));

    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 0.5f,
                0.31f);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f,
                1.0f);
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    ImGui::SFML::Update(window, deltaClock.restart());

    ImGui::Begin("Options");
    ImGui::LabelText("Cursor Locked", "%d", options.catchMouse);
    ImGui::Checkbox("Wireframe", &options.wireframe);
    ImGui::Checkbox("Free Cam", &options.freecam);
    if (options.freecam) {
      ImGui::LabelText("Position", "%f %f %f", camPos.x, camPos.y, camPos.z);
      ImGui::LabelText("Forward", "%f %f %f", camForward.x, camForward.y, camForward.z);
      ImGui::LabelText("Mouse", "%d %d", freecam.mouseX, freecam.mouseY);
    } else {
      ImGui::LabelText("Rotation", "%f %f", arcball.camX, arcball.camZ);
      ImGui::InputFloat("Radius", &options.radius);
    }
    ImGui::End();

    ImGui::SFML::Render(window);

    window.display();
  }

  return 0;
}