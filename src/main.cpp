#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

float heightScale = 0.001;
const float speed = 1.0f;

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    // glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // depth testing
    glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LESS);
    // blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    Model aloe_vera("resources/objects/aloe_vera_plant/aloevera.obj");
    Model lightBall("resources/objects/ball/ball.obj");
    Model room("resources/objects/room/untitled.obj");
    unsigned int heightMap = loadTexture(string("resources/objects/room/displacement.png").c_str());
    Model glassDoor("resources/objects/room/glass.obj");

    // instantiation of shaders

    Shader aloeShader("resources/shaders/aloe_vera.vs", "resources/shaders/aloe_vera.fs");
    Shader lightSource("resources/shaders/light_source.vs", "resources/shaders/light_source.fs");
    Shader basic("resources/shaders/basic.vs", "resources/shaders/basic.fs");
    Shader glass("resources/shaders/glass.vs", "resources/shaders/glass.fs");
    Shader simple("resources/shaders/simple.vs", "resources/shaders/simple.fs");
    Shader plant("resources/shaders/plant.vs", "resources/shaders/simple.fs");

    // setting point light
    glm::vec3 lightPos(1.0f, 1.0f, 1.0f);
    glm::vec3 spotlights[] = {
            glm::vec3(-6.0f, 1.3f, 2.0f),
            glm::vec3(-4.0f, 0.5f, -3.0f),
            glm::vec3(7.0f, 1.2f, 6.0f)
    };

    // instancing
    unsigned int amount = 90;
    glm::mat4 *modelMatrices;
    modelMatrices = new glm::mat4[amount];
    int k = 9;
    for (int j = 0; j < k; j++) {
        for (int i = 0; i < amount / k; i++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(j * 1.2f, 0.0f, i * 0.5f));
            modelMatrices[j * amount / k + i] = model;
        }
    }

    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

    for (int i = 0; i < aloe_vera.meshes.size(); i++) {
        unsigned int VAO = aloe_vera.meshes[i].VAO;
        glBindVertexArray(VAO);
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *) 0);
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *) (sizeof(glm::vec4)));
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *) (2 * sizeof(glm::vec4)));
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *) (3 * sizeof(glm::vec4)));

        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);
        glVertexAttribDivisor(7, 1);
        glVertexAttribDivisor(8, 1);

        glBindVertexArray(0);
    }


    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // setting values for aloeVera shader
        aloeShader.use();

        aloeShader.setVec3("viewPosition", camera.Position);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f,
                                                100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        aloeShader.setMat4("projection", projection);
        aloeShader.setMat4("view", view);
        aloeShader.setVec3("pointLight.position", lightPos);
        aloeShader.setVec3("pointLight.ambient", 0.2f, 0.2f, 0.2f);
        aloeShader.setVec3("pointLight.diffuse", 0.5f, 0.5f, 0.5f);
        aloeShader.setVec3("pointLight.specular", 1.0f, 1.0f, 1.0f);
        aloeShader.setFloat("pointLight.constant", 1.0f);
        aloeShader.setFloat("pointLight.linear", 0.09f);
        aloeShader.setFloat("pointLight.quadratic", 0.032f);

        aloeShader.setVec3("lightPos[" + to_string(0) + "]", lightPos);
        aloeShader.setVec3("viewPos", camera.Position);
        aloeShader.setFloat("material.shininess", 32.0f);

        for (int i = 0; i < spotlights->length(); i++) {
            aloeShader.setVec3("lightPos[" + to_string(i + 1) + "]", spotlights[i]);
            // our spotlights will follow our movement
            aloeShader.setVec3("lightDirs[" + to_string(i) + "]", camera.Position - spotlights[i]);
            aloeShader.setVec3("spotlights[" + to_string(i) + ".ambient", 0.5f, 0.5f, 0.5f);
            aloeShader.setVec3("spotlights[" + to_string(i) + ".diffuse", 1.0f, 1.0f, 1.0f);
            aloeShader.setVec3("spotlights[" + to_string(i) + "].specular", 1.0f, 1.0f, 1.0f);
            aloeShader.setFloat("spotlights[" + to_string(i) + "].constant", 1.0f);
            aloeShader.setFloat("spotlights[" + to_string(i) + "].linear", 0.09f);
            aloeShader.setFloat("spotlights[" + to_string(i) + "].quadratic", 0.032f);
            aloeShader.setVec3("spotlights[" + to_string(i) + "].position", spotlights[i]);
            aloeShader.setVec3("spotlights[" + to_string(i) + "].direction", camera.Position - spotlights[i]);
            aloeShader.setFloat("spotlights[" + to_string(i) + "].cutOff", glm::cos(glm::radians(20.5f)));
            aloeShader.setFloat("spotlights[" + to_string(i) + "].outerCutOff", glm::cos(glm::radians(25.5f)));
        }

        // unfortunately, face culling doesn't work well on this model
        for (int i = 0; i < aloe_vera.meshes[0].textures.size(); i++) {
            if (aloe_vera.meshes[0].textures[i].type == "texture_diffuse") {
                aloeShader.setInt("material.texture_diffuse1", i);
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, aloe_vera.meshes[0].textures[i].id);
            } else if (aloe_vera.meshes[0].textures[i].type == "texture_specular") {
                aloeShader.setInt("material.texture_specular1", i);
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, aloe_vera.meshes[0].textures[i].id);
            } else if (aloe_vera.meshes[0].textures[i].type == "texture_normal") {
                aloeShader.setInt("material.normalMap", i);
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, aloe_vera.meshes[0].textures[i].id);
            }
        }
        glBindVertexArray(aloe_vera.meshes[0].VAO);
        glDrawElementsInstanced(GL_TRIANGLES, aloe_vera.meshes[0].indices.size(), GL_UNSIGNED_INT, nullptr, amount);
        glBindVertexArray(0);

        plant.use();
        plant.setMat4("projection", projection);
        plant.setMat4("view", view);

        plant.setVec3("pointLight.position", lightPos);
        plant.setVec3("pointLight.ambient", 0.2f, 0.2f, 0.2f);
        plant.setVec3("pointLight.diffuse", 0.5f, 0.5f, 0.5f);
        plant.setVec3("pointLight.specular", 1.0f, 1.0f, 1.0f);
        plant.setFloat("pointLight.constant", 1.0f);
        plant.setFloat("pointLight.linear", 0.09f);
        plant.setFloat("pointLight.quadratic", 0.032f);

        simple.setVec3("lightPos", lightPos);
        for (int i = 0; i < spotlights->length(); i++) {
            plant.setVec3("lightPos[" + to_string(i + 1) + "]", spotlights[i]);
            plant.setVec3("lightDirs[" + to_string(i) + "]", camera.Position - spotlights[i]);
            plant.setVec3("spotlights[" + to_string(i) + ".ambient", 0.5f, 0.5f, 0.5f);
            plant.setVec3("spotlights[" + to_string(i) + ".diffuse", 1.0f, 1.0f, 1.0f);
            plant.setVec3("spotlights[" + to_string(i) + "].specular", 1.0f, 1.0f, 1.0f);
            plant.setFloat("spotlights[" + to_string(i) + "].constant", 1.0f);
            plant.setFloat("spotlights[" + to_string(i) + "].linear", 0.09f);
            plant.setFloat("spotlights[" + to_string(i) + "].quadratic", 0.032f);
            plant.setVec3("spotlights[" + to_string(i) + "].position", spotlights[i]);
            plant.setVec3("spotlights[" + to_string(i) + "].direction", camera.Position - spotlights[i]);
            plant.setFloat("spotlights[" + to_string(i) + "].cutOff", glm::cos(glm::radians(12.5f)));
            plant.setFloat("spotlights[" + to_string(i) + "].outerCutOff", glm::cos(glm::radians(15.0f)));
        }

            for (int i = 0; i < aloe_vera.meshes[1].textures.size(); i++) {
                if (aloe_vera.meshes[1].textures[i].type == "texture_diffuse") {
                    aloeShader.setInt("material.texture_diffuse1", i);
                    glActiveTexture(GL_TEXTURE0 + i);
                    glBindTexture(GL_TEXTURE_2D, aloe_vera.meshes[1].textures[i].id);
                } else if (aloe_vera.meshes[1].textures[i].type == "texture_specular") {
                    aloeShader.setInt("material.texture_specular1", i);
                    glActiveTexture(GL_TEXTURE0 + i);
                    glBindTexture(GL_TEXTURE_2D, aloe_vera.meshes[1].textures[i].id);
                } else if (aloe_vera.meshes[1].textures[i].type == "texture_normal") {
                    aloeShader.setInt("material.normalMap", i);
                    glActiveTexture(GL_TEXTURE0 + i);
                    glBindTexture(GL_TEXTURE_2D, aloe_vera.meshes[1].textures[i].id);
                }
            }
            glBindVertexArray(aloe_vera.meshes[1].VAO);
            glDrawElementsInstanced(GL_TRIANGLES, aloe_vera.meshes[1].indices.size(), GL_UNSIGNED_INT, nullptr, amount);
            glBindVertexArray(0);

            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CW);
            lightSource.use();
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, lightPos);
            model = glm::scale(model, glm::vec3(1.0f / 20));
            lightSource.setMat4("view", view);
            lightSource.setMat4("projection", projection);
            lightSource.setMat4("model", model);
            lightBall.Draw(lightSource);

            glDisable(GL_CULL_FACE);

            // there's no need to cull faces on our room, as it is made out of 6 planes

            basic.use();
            basic.setMat4("projection", projection);
            basic.setMat4("view", view);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 2.00f, 0.0f));
            basic.setMat4("model", model);

            basic.setVec3("pointLight.position", lightPos);
            basic.setVec3("pointLight.ambient", 0.2f, 0.2f, 0.2f);
            basic.setVec3("pointLight.diffuse", 0.5f, 0.5f, 0.5f);
            basic.setVec3("pointLight.specular", 1.0f, 1.0f, 1.0f);
            basic.setFloat("pointLight.constant", 1.0f);
            basic.setFloat("pointLight.linear", 0.09f);
            basic.setFloat("pointLight.quadratic", 0.032f);

            basic.setVec3("lightPos", lightPos);
            for (int i = 0; i < spotlights->length(); i++) {
                basic.setVec3("lightPos[" + to_string(i + 1) + "]", spotlights[i]);
                basic.setVec3("lightDirs[" + to_string(i) + "]", camera.Position - spotlights[i]);
                basic.setVec3("spotlights[" + to_string(i) + ".ambient", 0.5f, 0.5f, 0.5f);
                basic.setVec3("spotlights[" + to_string(i) + ".diffuse", 1.0f, 1.0f, 1.0f);
                basic.setVec3("spotlights[" + to_string(i) + "].specular", 1.0f, 1.0f, 1.0f);
                basic.setFloat("spotlights[" + to_string(i) + "].constant", 1.0f);
                basic.setFloat("spotlights[" + to_string(i) + "].linear", 0.09f);
                basic.setFloat("spotlights[" + to_string(i) + "].quadratic", 0.032f);
                basic.setVec3("spotlights[" + to_string(i) + "].position", spotlights[i]);
                basic.setVec3("spotlights[" + to_string(i) + "].direction", camera.Position - spotlights[i]);
                basic.setFloat("spotlights[" + to_string(i) + "].cutOff", glm::cos(glm::radians(12.5f)));
                basic.setFloat("spotlights[" + to_string(i) + "].outerCutOff", glm::cos(glm::radians(15.0f)));
            }

            basic.setVec3("viewPos", camera.Position);
            basic.setFloat("material.shininess", 32.0f);
            for (int j = 0; j < 4; j++) {
                for (int i = 0; i < room.meshes[j].textures.size(); i++) {
                    if (room.meshes[j].textures[i].type == "texture_diffuse") {
                        basic.setInt("material.texture_diffuse1", i);
                        glActiveTexture(GL_TEXTURE0 + i);
                        glBindTexture(GL_TEXTURE_2D, room.meshes[j].textures[i].id);
                    } else if (room.meshes[j].textures[i].type == "texture_specular") {
                        basic.setInt("material.texture_specular1", i);
                        glActiveTexture(GL_TEXTURE0 + i);
                        glBindTexture(GL_TEXTURE_2D, room.meshes[j].textures[i].id);
                    } else if (room.meshes[j].textures[i].type == "texture_normal") {
                        basic.setInt("material.normalMap", i);
                        glActiveTexture(GL_TEXTURE0 + i);
                        glBindTexture(GL_TEXTURE_2D, room.meshes[j].textures[i].id);
                    }
                }
                basic.setBool("flag", true);
                // knowing our model, if there is a normal map, then we know there is a displacement map
                // loading displacement map
                // obj file doesn't recognize displacement maps, so we have to load it here
                basic.setInt("material.depthMap", room.meshes[j].textures.size());
                glActiveTexture(GL_TEXTURE0 + room.meshes[j].textures.size());
                glBindTexture(GL_TEXTURE_2D, heightMap);
                basic.setBool("parallax", true);
                basic.setFloat("heightScale", heightScale);
                glBindVertexArray(room.meshes[j].VAO);
                glDrawElements(GL_TRIANGLES, room.meshes[j].indices.size(), GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
            }

            simple.use();
            simple.setMat4("projection", projection);
            simple.setMat4("view", view);
            simple.setMat4("model", model);

            simple.setVec3("pointLight.position", lightPos);
            simple.setVec3("pointLight.ambient", 0.2f, 0.2f, 0.2f);
            simple.setVec3("pointLight.diffuse", 0.5f, 0.5f, 0.5f);
            simple.setVec3("pointLight.specular", 1.0f, 1.0f, 1.0f);
            simple.setFloat("pointLight.constant", 1.0f);
            simple.setFloat("pointLight.linear", 0.09f);
            simple.setFloat("pointLight.quadratic", 0.032f);

            simple.setVec3("lightPos", lightPos);
            for (int i = 0; i < spotlights->length(); i++) {
                simple.setVec3("lightPos[" + to_string(i + 1) + "]", spotlights[i]);
                simple.setVec3("lightDirs[" + to_string(i) + "]", camera.Position - spotlights[i]);
                simple.setVec3("spotlights[" + to_string(i) + ".ambient", 0.5f, 0.5f, 0.5f);
                simple.setVec3("spotlights[" + to_string(i) + ".diffuse", 1.0f, 1.0f, 1.0f);
                simple.setVec3("spotlights[" + to_string(i) + "].specular", 1.0f, 1.0f, 1.0f);
                simple.setFloat("spotlights[" + to_string(i) + "].constant", 1.0f);
                simple.setFloat("spotlights[" + to_string(i) + "].linear", 0.09f);
                simple.setFloat("spotlights[" + to_string(i) + "].quadratic", 0.032f);
                simple.setVec3("spotlights[" + to_string(i) + "].position", spotlights[i]);
                simple.setVec3("spotlights[" + to_string(i) + "].direction", camera.Position - spotlights[i]);
                simple.setFloat("spotlights[" + to_string(i) + "].cutOff", glm::cos(glm::radians(12.5f)));
                simple.setFloat("spotlights[" + to_string(i) + "].outerCutOff", glm::cos(glm::radians(15.0f)));
            }

            for (int j = 4; j < 6; j++) {

                for (int i = 0; i < room.meshes[j].textures.size(); i++) {
                    if (room.meshes[j].textures[i].type == "texture_diffuse") {
                        basic.setInt("material.texture_diffuse1", i);
                        glActiveTexture(GL_TEXTURE0 + i);
                        glBindTexture(GL_TEXTURE_2D, room.meshes[j].textures[i].id);
                    } else if (room.meshes[j].textures[i].type == "texture_specular") {
                        basic.setInt("material.texture_specular1", i);
                        glActiveTexture(GL_TEXTURE0 + i);
                        glBindTexture(GL_TEXTURE_2D, room.meshes[j].textures[i].id);
                    } else if (room.meshes[j].textures[i].type == "texture_normal") {
                        basic.setInt("material.normalMap", i);
                        glActiveTexture(GL_TEXTURE0 + i);
                        glBindTexture(GL_TEXTURE_2D, room.meshes[j].textures[i].id);
                    }
                }
                glBindVertexArray(room.meshes[j].VAO);
                glDrawElements(GL_TRIANGLES, room.meshes[j].indices.size(), GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
            }

/*
        simple.use();
        simple.setMat4("projection", projection);
        simple.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 2.00f, 0.0f));
        simple.setMat4("model", model);

        simple.setVec3("pointLight.position", lightPos);
        simple.setVec3("pointLight.ambient", 0.2f, 0.2f, 0.2f);
        simple.setVec3("pointLight.diffuse", 0.5f, 0.5f, 0.5f);
        simple.setVec3("pointLight.specular", 1.0f, 1.0f, 1.0f);
        simple.setFloat("pointLight.constant", 1.0f);
        simple.setFloat("pointLight.linear", 0.09f);
        simple.setFloat("pointLight.quadratic", 0.032f);

        simple.setVec3("lightPos", lightPos);
        for(int i = 0; i < spotlights->length(); i++) {
            simple.setVec3("lightPos[" + to_string(i + 1) + "]", spotlights[i]);
            simple.setVec3("lightDirs[" + to_string(i) + "]", camera.Position - spotlights[i]);
            simple.setVec3("spotlights[" + to_string(i) + ".ambient", 0.5f, 0.5f, 0.5f);
            simple.setVec3("spotlights[" + to_string(i) + ".diffuse", 1.0f, 1.0f, 1.0f);
            simple.setVec3("spotlights[" + to_string(i) + "].specular", 1.0f, 1.0f, 1.0f);
            simple.setFloat("spotlights[" + to_string(i) + "].constant", 1.0f);
            simple.setFloat("spotlights[" + to_string(i) + "].linear", 0.09f);
            simple.setFloat("spotlights[" + to_string(i) + "].quadratic", 0.032f);
            simple.setVec3("spotlights[" + to_string(i) + "].position", spotlights[i]);
            simple.setVec3("spotlights[" + to_string(i) + "].direction", camera.Position - spotlights[i]);
            simple.setFloat("spotlights[" + to_string(i) + "].cutOff", glm::cos(glm::radians(12.5f)));
            simple.setFloat("spotlights[" + to_string(i) + "].outerCutOff", glm::cos(glm::radians(15.0f)));
        }
        room.Draw(simple);
        */
            glass.use();
            glass.setMat4("view", view);
            glass.setMat4("projection", projection);
            glass.setMat4("model", model);
            glassDoor.Draw(glass);
            // moving our point light
            model = glm::mat4(1.0f);
            model = glm::rotate(model, speed * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));
            lightPos = glm::vec3(model * glm::vec4(lightPos, 1.0f));

            for (int i = 0; i < spotlights->length(); i++) {
                lightSource.use();
                model = glm::mat4(1.0f);
                model = glm::translate(model, spotlights[i]);
                model = glm::scale(model, glm::vec3(1.0f / 20));
                lightSource.setMat4("model", model);
                lightBall.Draw(lightSource);
            }

            // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
            // -------------------------------------------------------------------------------
            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        // glfw: terminate, clearing all previously allocated GLFW resources.
        // ------------------------------------------------------------------
        glfwTerminate();
        return 0;
    }

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
