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

/*

TODO:
- Save Depth to fbo
- Stencil Buffer
- LightDist > 1
  - 1 - distanceToBackside in frag_irradiance
- ShadowMap Perspective (no projection?)
- (Implement Gaussian Blur)
- LightDir nicht immer zu 0 0 0

*/

float samplePositions[] = {
  0.000000f,  0.000000f,
  1.633992f,  0.036795f,
  0.177801f,  1.717593f,
  -0.194906f,  0.091094f,
  -0.239737f, -0.220217f,
  -0.003530f, -0.118219f,
  1.320107f, -0.181542f,
  5.970690f,  0.253378f,
  -1.089250f,  4.958349f,
  -4.015465f,  4.156699f,
  -4.063099f, -4.110150f,
  -0.638605f, -6.297663f,
  2.542348f, -3.245901f
};

float sampleWeights[] = {
  0.220441f,  0.487000f, 0.635000f,
  0.076356f,  0.064487f, 0.039097f,
  0.116515f,  0.103222f, 0.064912f,
  0.064844f,  0.086388f, 0.062272f,
  0.131798f,  0.151695f, 0.103676f,
  0.025690f,  0.042728f, 0.033003f,
  0.048593f,  0.064740f, 0.046131f,
  0.048092f,  0.003042f, 0.000400f,
  0.048845f,  0.005406f, 0.001222f,
  0.051322f,  0.006034f, 0.001420f,
  0.061428f,  0.009152f, 0.002511f,
  0.030936f,  0.002868f, 0.000652f,
  0.073580f,  0.023239f, 0.009703f
};

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

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void*)(sizeof(float) * 3));

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

  glm::vec3 getPos() {
    float angle = rot.y / angleFactor;
  
    float camY = sin(angle) * exp(radius);
    float camZ = cos(angle) * exp(radius);

    glm::vec3 result(0.0, camY, camZ);
    return glm::rotate(result, -rot.x / angleFactor, glm::vec3(0, 1, 0));
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
  std::string result, line;
  while (std::getline(ifs, line))
    result += line + "\n";

  return result;
}

model loadModel(const std::string &filename) {
  Assimp::Importer importer;

  const aiScene *scene = importer.ReadFile(
      filename, aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                    aiProcess_SortByPType | aiProcess_GenSmoothNormals |
                    aiProcess_GenUVCoords);

  model result;

  printf("uv channels: %d\n", scene->mMeshes[0]->GetNumUVChannels());

  for (int i = 0; i < scene->mMeshes[0]->mNumVertices; i++) {
    aiVector3D v = scene->mMeshes[0]->mVertices[i];
    aiVector3D n = scene->mMeshes[0]->mNormals[i];
    result.vertices.push_back(v.x * 100);
    result.vertices.push_back(v.y * 100);
    result.vertices.push_back(v.z * 100);
    result.vertices.push_back(n.x * 100);
    result.vertices.push_back(n.y * 100);
    result.vertices.push_back(n.z * 100);
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
    printf("Error compiling vertex shader(%s): %s\n", vertFilename, infoLog);
  }

  GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
  std::string fragSource = readFile(fragFilename);
  const char *fragAddr = fragSource.c_str();
  glShaderSource(fragShader, 1, &fragAddr, NULL);
  glCompileShader(fragShader);

  glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragShader, 512, NULL, infoLog);
    printf("Error compiling fragment shader(%s): %s\n", fragFilename, infoLog);
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

struct framebuffer {
  framebuffer(const char *vertFilename, const char *fragFilename, int width, int height) {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    
    glGenTextures(1, &renderTexture);
    glBindTexture(GL_TEXTURE_2D, renderTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTexture, 0);

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    //glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
      printf("Successfully created framebuffer\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    screenShaderProgram = compileShaders(vertFilename, fragFilename);
    glUseProgram(screenShaderProgram);
    glUniform1i(glGetUniformLocation(screenShaderProgram, "screenTexture"), 0);

    // Screen VAO
    
    glGenBuffers(1, &screenVBO);

    glGenVertexArrays(1, &screenVAO);

    glBindVertexArray(screenVAO);

    float screenVerts[] = {
      -1.0f, +1.0f, +0.0f, +1.0f,
      -1.0f, -1.0f, +0.0f, +0.0f,
      +1.0f, -1.0f, +1.0f, +0.0f,
  
      -1.0f, +1.0f, +0.0f, +1.0f,
      +1.0f, -1.0f, +1.0f, +0.0f,
      +1.0f, +1.0f, +1.0f, +1.0f,
    };

    glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4,
                screenVerts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));

    glBindVertexArray(0);
  }
  ~framebuffer() {
    glDeleteFramebuffers(1, &fbo);
  }
  
  GLuint fbo;
  GLuint renderTexture;
  GLuint rbo;
  GLuint screenShaderProgram;
  GLuint screenVBO;
  GLuint screenVAO;
};

int main() {
  // Window Setup

  const int width = 1600, height = 900;

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

  GLuint shaderProgramShadowmap = compileShaders("shaders/vert_shadowmap.glsl", "shaders/frag_shadowmap.glsl");
  GLuint shaderProgramIrradiance = compileShaders("shaders/vert_irradiance.glsl", "shaders/frag_irradiance.glsl");

  //model m = loadModel("models/Isotrop-upperjaw.ply");
  model m = loadModel("models/african_head/african_head.obj");

  arccam arcCam;
  freecam freeCam;

  // MVP

  glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f, 0.01f, 0.01f));

  glm::mat4 view, lightView;

  glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)window.getSize().x / window.getSize().y, 0.001f, 1000.0f);
  glm::mat4 lightProj = glm::perspective(glm::radians(90.0f), (float)window.getSize().x / window.getSize().y, 0.001f, 1000.0f);

  // Framebuffer
  framebuffer fb_shadowmap("shaders/fbo_vert.glsl", "shaders/fbo_frag.glsl", width, height);
  framebuffer fb_irradiance("shaders/fbo_vert.glsl", "shaders/fbo_frag.glsl", width, height);

  // Config

  const struct {
    bool wireframe = false;
    bool freecam = false;
    int renderState = 2;
    float color[3] = { 0.7f, 0.4f, 0.4f };
    glm::vec3 lightPos = glm::vec3(0.0f, 0.04f, -0.08f);
    float transmittanceScale = 0.005f;
    float powBase = 2.718;
    float powFactor = 1;
  } DefaultOptions;

  auto options = DefaultOptions;

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

    // Render Shadowmap

    glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
    glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);
    glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);

    glBindFramebuffer(GL_FRAMEBUFFER, fb_shadowmap.fbo);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    if (options.wireframe)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glUseProgram(shaderProgramShadowmap);
    
    if (options.freecam)
      view = freeCam.getViewMatrix();
    else
      view = arcCam.getViewMatrix();
    
    lightView = glm::lookAt(options.lightPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    
    glUniformMatrix4fv(
      glGetUniformLocation(shaderProgramShadowmap, "model"),
      1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(
      glGetUniformLocation(shaderProgramShadowmap, "lightView"),
      1, GL_FALSE, glm::value_ptr(lightView));
    glUniformMatrix4fv(
      glGetUniformLocation(shaderProgramShadowmap, "projection"),
      1, GL_FALSE, glm::value_ptr(lightProj));

    glUniform3fv(
      glGetUniformLocation(shaderProgramShadowmap, "lightPos"),
      1, glm::value_ptr(options.lightPos));

    m.draw();

    // Render irradiance

    glBindFramebuffer(GL_FRAMEBUFFER, fb_irradiance.fbo);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    if (options.wireframe)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glUseProgram(shaderProgramIrradiance);
    
    glUniformMatrix4fv(
      glGetUniformLocation(shaderProgramIrradiance, "model"),
      1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(
      glGetUniformLocation(shaderProgramIrradiance, "view"),
      1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(
      glGetUniformLocation(shaderProgramIrradiance, "lightView"),
      1, GL_FALSE, glm::value_ptr(lightView));
    glUniformMatrix4fv(
      glGetUniformLocation(shaderProgramIrradiance, "lightViewInv"),
      1, GL_FALSE, glm::value_ptr(glm::inverse(lightView)));
    glUniformMatrix4fv(
      glGetUniformLocation(shaderProgramIrradiance, "projection"),
      1, GL_FALSE, glm::value_ptr(proj));
    glUniformMatrix4fv(
      glGetUniformLocation(shaderProgramIrradiance, "lightProjection"),
      1, GL_FALSE, glm::value_ptr(lightProj));
    glUniform1i(glGetUniformLocation(shaderProgramIrradiance, "screenWidth"), window.getSize().x);
    glUniform1i(glGetUniformLocation(shaderProgramIrradiance, "screenHeight"), window.getSize().y);
    glUniform2fv(glGetUniformLocation(shaderProgramIrradiance, "samplePositions"), 13, samplePositions);
    glUniform3fv(glGetUniformLocation(shaderProgramIrradiance, "sampleWeights"), 13, sampleWeights);

    glUniform1f(
      glGetUniformLocation(shaderProgramIrradiance, "transmittanceScale"),
      options.transmittanceScale);
    glUniform1i(
      glGetUniformLocation(shaderProgramIrradiance, "renderState"),
      options.renderState);
    glUniform1f(
      glGetUniformLocation(shaderProgramIrradiance, "powBase"),
      options.powBase);
    glUniform1f(
      glGetUniformLocation(shaderProgramIrradiance, "powFactor"),
      options.powFactor);

    glUniform3fv(
      glGetUniformLocation(shaderProgramIrradiance, "objectColor"),
      1, options.color);
    glUniform3f(
      glGetUniformLocation(shaderProgramIrradiance, "lightColor"),
      1.0f, 1.0f, 1.0f);
    glUniform3fv(
      glGetUniformLocation(shaderProgramIrradiance, "lightPos"),
      1, glm::value_ptr(options.lightPos));
    glUniform3fv(
      glGetUniformLocation(shaderProgramIrradiance, "viewPos"),
      1, glm::value_ptr(options.freecam ? freeCam.pos : arcCam.getPos()));
      
    glUniform1i(glGetUniformLocation(shaderProgramIrradiance, "shadowmapTexture"), 0);
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, fb_shadowmap.renderTexture);

    m.draw();

    // Render fbo to screen
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(fb_irradiance.screenShaderProgram);

    glUniform1i(glGetUniformLocation(fb_irradiance.screenShaderProgram, "screenWidth"), window.getSize().x);
    glUniform1i(glGetUniformLocation(fb_irradiance.screenShaderProgram, "screenHeight"), window.getSize().y);
    glUniform1i(glGetUniformLocation(fb_irradiance.screenShaderProgram, "renderState"), options.renderState);
    glUniform2fv(glGetUniformLocation(fb_irradiance.screenShaderProgram, "samplePositions"), 13, samplePositions);
    glUniform3fv(glGetUniformLocation(fb_irradiance.screenShaderProgram, "sampleWeights"), 13, sampleWeights);

    glBindVertexArray(fb_irradiance.screenVAO);
    glUniform1i(glGetUniformLocation(fb_irradiance.screenShaderProgram, "shadowmapTexture"), 0);
    glUniform1i(glGetUniformLocation(fb_irradiance.screenShaderProgram, "irradianceTexture"), 1);
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, fb_shadowmap.renderTexture);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, fb_irradiance.renderTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    ImGui::SFML::Update(window, deltaClock.restart());

    ImGui::Begin("Options");
    ImGui::Checkbox("Wireframe", &options.wireframe);
    ImGui::Checkbox("Free Cam", &options.freecam);
    ImGui::InputInt("Render State", &options.renderState);
    ImGui::DragFloat3("Color", options.color, 0.01, 0, 1);
    ImGui::DragFloat("Transmittance Scale", &options.transmittanceScale, 0.0001f, 0, 0.3);
    ImGui::DragFloat("Pow Base", &options.powBase, 0.01f, 0, 4);
    ImGui::DragFloat("Pow Factor", &options.powFactor, 0.01f, 0, 3);
    ImGui::DragFloat3("Light Pos", glm::value_ptr(options.lightPos), 0.01, -5, 5);
    if (options.freecam) {
      ImGui::LabelText("Position", "%f %f %f", freeCam.pos.x, freeCam.pos.y, freeCam.pos.z);
      ImGui::LabelText("Rotation", "%f %f", freeCam.rot.x, freeCam.rot.y);
      if (ImGui::Button("Reset")) {
        freeCam.pos = glm::vec3(0, 0, -1);
        freeCam.rot = glm::vec2(0);
        options = DefaultOptions;
      }
    } else {
      ImGui::LabelText("Rotation", "%f %f", arcCam.rot.x, arcCam.rot.y);
      ImGui::DragFloat("Radius", &arcCam.radius, 0.01f, -1.0f, 1.0f);
      if (ImGui::Button("Reset")) {
        arcCam.rot = glm::vec2(0);
        arcCam.radius = 1;
        options = DefaultOptions;
      }
    }
    ImGui::End();

    ImGui::SFML::Render(window);

    window.display();
  }

  return 0;
}
