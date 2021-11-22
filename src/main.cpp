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
unsigned int loadCubemap(vector<std::string> faces);

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

    // setting point light
    glm::vec3 lightPos(0.0f, 1.0f, 1.0f);

    // instancing
    unsigned int amount = 500;
    glm::mat4 *modelMatrices;
    modelMatrices = new glm::mat4[amount];
    int k = 25;
    for(int j = 0; j < k; j++) {
        for (int i = 0; i < amount/k; i++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(j * 1.2f, 0.0f, i * 0.5f));
            modelMatrices[j * amount/k + i] = model;
        }
    }

    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, amount*sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

    for(int i = 0; i < aloe_vera.meshes.size(); i++) {
        unsigned int VAO = aloe_vera.meshes[i].VAO;
        glBindVertexArray(VAO);
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2*sizeof(glm::vec4)));
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3*sizeof(glm::vec4)));

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

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        aloeShader.setMat4("projection", projection);
        aloeShader.setMat4("view", view);
        aloeShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        aloeShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
        aloeShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
        aloeShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
        aloeShader.setVec3("pointLight.position", lightPos);
        aloeShader.setVec3("pointLight.ambient", 0.2f, 0.2f, 0.2f);
        aloeShader.setVec3("pointLight.diffuse", 0.5f, 0.5f, 0.5f);
        aloeShader.setVec3("pointLight.specular", 1.0f, 1.0f, 1.0f);
        aloeShader.setFloat("pointLight.constant", 1.0f);
        aloeShader.setFloat("pointLight.linear", 0.09f);
        aloeShader.setFloat("pointLight.quadratic", 0.032f);

        aloeShader.setVec3("lightPos", lightPos);
        aloeShader.setVec3("viewPos", camera.Position);
        aloeShader.setFloat("material.shininess", 32.0f);

        for (int j = 0; j < aloe_vera.meshes.size(); j++) {
            bool flag = false; // checks whether a mesh has a normal map
            for(int i = 0; i < aloe_vera.meshes[j].textures.size(); i++) {
                if(aloe_vera.meshes[j].textures[i].type == "texture_diffuse") {
                    aloeShader.setInt("material.texture_diffuse1", i);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, aloe_vera.meshes[j].textures[i].id);
                } else if(aloe_vera.meshes[j].textures[i].type == "texture_specular") {
                    aloeShader.setInt("material.texture_specular1", i);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, aloe_vera.meshes[j].textures[i].id);
                } else if(aloe_vera.meshes[j].textures[i].type == "texture_normal") {
                    aloeShader.setInt("material.normalMap", 2);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, aloe_vera.meshes[j].textures[i].id);
                    flag = true;
                }
            }
            aloeShader.setBool("flag", flag);
            glBindVertexArray(aloe_vera.meshes[j].VAO);
            glDrawElementsInstanced(GL_TRIANGLES, aloe_vera.meshes[j].indices.size(), GL_UNSIGNED_INT, nullptr, amount);
            glBindVertexArray(0);
        }
        lightSource.use();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(1.0f/20));
        lightSource.setMat4("view", view);
        lightSource.setMat4("projection", projection);
        lightSource.setMat4("model", model);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CW);
        lightBall.Draw(lightSource);

        glDisable(GL_CULL_FACE);

        basic.use();
        basic.setMat4("projection", projection);
        basic.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 2.00f, 0.0f));
        basic.setMat4("model", model);

        basic.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        basic.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
        basic.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
        basic.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
        basic.setVec3("pointLight.position", lightPos);
        basic.setVec3("pointLight.ambient", 0.2f, 0.2f, 0.2f);
        basic.setVec3("pointLight.diffuse", 0.5f, 0.5f, 0.5f);
        basic.setVec3("pointLight.specular", 1.0f, 1.0f, 1.0f);
        basic.setFloat("pointLight.constant", 1.0f);
        basic.setFloat("pointLight.linear", 0.09f);
        basic.setFloat("pointLight.quadratic", 0.032f);

        basic.setVec3("lightPos", lightPos);
        basic.setVec3("viewPos", camera.Position);
        basic.setFloat("material.shininess", 32.0f);
        for (int j = 0; j < room.meshes.size(); j++) {
            bool flag = false; // checks whether a mesh has a normal map
            basic.setBool("parallax", false);
            for(int i = 0; i < room.meshes[j].textures.size(); i++) {
                if(room.meshes[j].textures[i].type == "texture_diffuse") {
                    basic.setInt("material.texture_diffuse1", i);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, room.meshes[j].textures[i].id);
                } else if(room.meshes[j].textures[i].type == "texture_specular") {
                    basic.setInt("material.texture_specular1", i);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, room.meshes[j].textures[i].id);
                } else if(room.meshes[j].textures[i].type == "texture_normal") {
                    basic.setInt("material.normalMap", 2);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, room.meshes[j].textures[i].id);
                    flag = true;
                }
            }
            basic.setBool("flag", flag);
            // knowing our model, if there is a normal map, then we know there is a displacement map
            if(flag) {
                // loading displacement map
                // obj file doesn't recognize displacement maps, so we have to load it here
                basic.setInt("material.depthMap", 3);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, heightMap);
                basic.setBool("parallax", true);
                basic.setFloat("heightScale", heightScale);
            }
            glBindVertexArray(room.meshes[j].VAO);
            glDrawElements(GL_TRIANGLES, room.meshes[j].indices.size(), GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }
        glass.use();
        glass.setMat4("view", view);
        glass.setMat4("projection", projection);
        glass.setMat4("model", model);
        glassDoor.Draw(glass);

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

// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front)
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
