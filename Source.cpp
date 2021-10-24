#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnOpengl/camera.h> // Camera class

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Fehir_M7_project"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao[5];         // Handle for the vertex array object
        GLuint vbo[5];         // Handle for the vertex buffer object
        GLuint nVertices[5];    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh gMesh;
    // Texture ids
    GLuint uTextureSponge;
    GLuint uTextureFloor;
    GLuint uTextureBroomHead;
    GLuint uTextureBroomHandle;
    GLuint uTextureBucket;
    // Shader program
    GLuint gCubeProgramId;
    GLuint gLampProgramId;

    // camera
    Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;
    glm::vec3 gCameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 gCameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 gCameraUp = glm::vec3(0.0f, 1.0f, 0.0f);


    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // Subject position and scale
    glm::vec3 gCubePosition(0.0f, 0.0f, 0.0f);
    glm::vec3 gCubeScale(2.0f);

    // Cube and light color
    glm::vec3 gObjectColor(1.0f, 0.2f, 0.0f);
    glm::vec3 gLightColor(1.0f, 1.0f, 0.6f);//set lamp color

    // Light position and scale
    glm::vec3 gLightPosition(1.5f, 0.5f, 3.0f);
    glm::vec3 gLightScale(0.3f);
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code*/
const GLchar* cubeVertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec2 textureCoordinate;
layout(location = 2) in vec3 normal; // VAP position 2 for normals


out vec2 vertexTextureCoordinate;
out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

    vertexTextureCoordinate = textureCoordinate;

    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
}
);


/* Fragment Shader Source Code*/
const GLchar* cubeFragmentShaderSource = GLSL(440,
    in vec2 vertexTextureCoordinate;
in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position

out vec4 fragmentColor;

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;

uniform sampler2D uTexture;

void main()
{
    fragmentColor = texture(uTexture, vertexTextureCoordinate); // Sends texture to the GPU for rendering
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

        //Calculate Ambient lighting*/
    float ambientStrength = 0.5f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

    //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color

    //Calculate Specular lighting*/
    float specularIntensity = 0.9f; // Set specular light strength
    float highlightSize = 16.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;


    // Calculate phong result
    vec3 objectColor = texture(uTexture, vertexTextureCoordinate).xyz;
    vec3 phong = (ambient + diffuse + specular) * objectColor;
    fragmentColor = vec4(phong, 1.0f); // Send lighting results to GPU
}
);

/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

        //Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f, 1.0f, 1.0f, 1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object

    // Create the shader program
    if (!UCreateShaderProgram(cubeVertexShaderSource, cubeFragmentShaderSource, gCubeProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Load texture
    const char* texFilename = "sponge.png";
    if (!UCreateTexture(texFilename, uTextureSponge))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gCubeProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gCubeProgramId, "uTexture"), 0);

    texFilename = "floor.png";
    if (!UCreateTexture(texFilename, uTextureFloor))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gCubeProgramId);
    // We set the texture as texture unit 1
    glUniform1i(glGetUniformLocation(gCubeProgramId, "uTexture"), 1);

    texFilename = "broomHead.png";
    if (!UCreateTexture(texFilename, uTextureBroomHead))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gCubeProgramId);
    // We set the texture as texture unit 2
    glUniform1i(glGetUniformLocation(gCubeProgramId, "uTexture"), 2);

    texFilename = "broomHandle.png";
    if (!UCreateTexture(texFilename, uTextureBroomHandle))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gCubeProgramId);
    // We set the texture as texture unit 3
    glUniform1i(glGetUniformLocation(gCubeProgramId, "uTexture"), 3);

    texFilename = "bucket.png";
    if (!UCreateTexture(texFilename, uTextureBucket))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gCubeProgramId);
    // We set the texture as texture unit 4
    glUniform1i(glGetUniformLocation(gCubeProgramId, "uTexture"), 4);


    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);

    // Release texture
    UDestroyTexture(uTextureSponge);
    UDestroyTexture(uTextureFloor);
    UDestroyTexture(uTextureBroomHead);
    UDestroyTexture(uTextureBroomHandle);
    UDestroyTexture(uTextureBucket);


    // Release shader program
    UDestroyShaderProgram(gCubeProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;
    float cameraOffset = cameraSpeed * gDeltaTime;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}


// Functioned called to render a frame
void URender()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Scales the object by 2
    glm::mat4 scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
    // 2. Rotates shape slightly to approximate photo angle 
    glm::mat4 rotation = glm::rotate(0.6f, glm::vec3(0.6, -1.0f, 0.0f));
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a perspective projection
    glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Set the shader to be used
    glUseProgram(gCubeProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gCubeProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gCubeProgramId, "view");
    GLint projLoc = glGetUniformLocation(gCubeProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    GLint uTextureLoc = glGetUniformLocation(gCubeProgramId, "uTexture");
    GLint lightColorLoc = glGetUniformLocation(gCubeProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gCubeProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gCubeProgramId, "viewPosition");

    
    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
    
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao[0]);
    
    // bind textures on corresponding texture units
    glUniform1i(uTextureLoc, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, uTextureSponge);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[0]);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao[1]);

    // bind textures on corresponding texture units
    glUniform1i(uTextureLoc, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, uTextureFloor);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[1]);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao[2]);

    // bind textures on corresponding texture units
    glUniform1i(uTextureLoc, 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, uTextureBroomHead);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[2]);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao[3]);

    // bind textures on corresponding texture units
    glUniform1i(uTextureLoc, 3);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, uTextureBroomHandle);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[3]);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao[4]);

    // bind textures on corresponding texture units
    glUniform1i(uTextureLoc, 4);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, uTextureBucket);

    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[4]);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // LAMP: draw lamp
    //----------------
    glUseProgram(gLampProgramId);

    //Transform the smaller cube used as a visual que for the light source
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to the Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[0]);
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[1]);
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[2]);
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[3]);
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[4]);


    // Deactivate the Vertex Array Object and shader program
    glBindVertexArray(0);
    glUseProgram(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{
    //Sponge vertex data
    GLfloat verts0[] = {
        //Positions          //Texture Coordinates            //Normals
       -0.6f, 0.0f, 0.2f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f, 0.0f, 0.2f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f, 0.1f, 0.2f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f, 0.1f, 0.2f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.6f, 0.1f, 0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.6f, 0.0f, 0.2f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,

       -0.6f, 0.0f,  0.4f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f, 0.0f,  0.4f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f, 0.1f,  0.4f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f, 0.1f,  0.4f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.6f, 0.1f,  0.4f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.6f, 0.0f,  0.4f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,

       -0.6f, 0.1f,  0.4f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.6f, 0.1f, 0.2f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.6f, 0.0f, 0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.6f, 0.0f, 0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.6f, 0.0f,  0.4f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.6f, 0.1f,  0.4f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,

       -0.2f, 0.1f,  0.4f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f, 0.1f, 0.2f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f, 0.0f, 0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f, 0.0f, 0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f, 0.0f,  0.4f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f, 0.1f,  0.4f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,

       -0.6f, 0.0f, 0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f, 0.0f, 0.2f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f, 0.0f,  0.4f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f, 0.0f,  0.4f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.6f, 0.0f,  0.4f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.6f, 0.0f, 0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,

       -0.6f,  0.0f, 0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f,  0.0f, 0.2f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f,  0.0f,  0.4f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.2f,  0.0f,  0.4f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.6f,  0.0f,  0.4f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.6f,  0.0f, 0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
    };

    // 3d Plane Vertex data
    GLfloat verts1[] = {
        //Positions          //Texture Coordinates
       -1.0, 0.0f, 1.0f, 0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       1.0, 0.0f, 1.0f, 0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -1.0, 0.0f, -1.0f, 1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,

       1.0, 0.0f, -1.0f, 1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -1.0, 0.0f, -1.0f, 1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       1.0, 0.0f, 1.0f, 0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
    };

    // Broom head Vertex data
    GLfloat verts2[] = {
        //Positions          //Texture Coordinates
       -1.0f, 0.0f, -0.2f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       0.0f, 0.0f, -0.2f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       0.0f, 0.2f, -0.2f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       0.0f, 0.2f, -0.2f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -1.0f, 0.2f, -0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -1.0f, 0.0f, -0.2f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,

       -1.0f, 0.0f,  0.0f,  0.0f, 0.0f,                  0.0f, 1.0f, 0.0f,
       0.0f, 0.0f,  0.0f,  1.0f, 0.0f,                  0.0f, 1.0f, 0.0f,
       0.0f, 0.2f,  0.0f,  1.0f, 1.0f,                  0.0f, 1.0f, 0.0f,
       0.0f, 0.2f,  0.0f,  1.0f, 1.0f,                  0.0f, 1.0f, 0.0f,
       0.0f, 0.2f,  0.0f,  0.0f, 1.0f,                  0.0f, 1.0f, 0.0f,
       -1.0f, 0.0f,  0.0f,  0.0f, 0.0f,                  0.0f, 1.0f, 0.0f,

       -1.0f, 0.2f,  0.0f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -1.0f, 0.2f, -0.2f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -1.0f, 0.0f, -0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -1.0f, 0.0f, -0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -1.0f, 0.0f,  0.0f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -1.0f, 0.2f,  0.0f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,

       0.0f, 0.2f,  0.0f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       0.0f, 0.2f, -0.2f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       0.0f, 0.0f, -0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       0.0f, 0.0f, -0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       0.0f, 0.0f,  0.0f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       0.0f, 0.2f,  0.0f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,

       -1.0f, 0.0f, -0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       0.0f, 0.0f, -0.2f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       0.0f, 0.0f,  0.0f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       0.0f, 0.0f,  0.0f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       0.0f, 0.0f,  0.0f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -1.0f, 0.0f, -0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,

       -1.0f,  0.0f, -0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       0.0f,  0.0f, -0.2f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       0.0f,  0.0f,  0.0f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       0.0f,  0.0f,  0.0f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -1.0f,  0.0f, -0.2f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
    };

    // Broom Handle Vertex data
    GLfloat verts3[] = {
        //Positions          //Texture Coordinates
       -0.5f, 0.2f, -0.1f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.4f, 0.2f, -0.1f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.4f, 1.0f, -0.1f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.4f, 1.0f, -0.1f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.5f, 1.0f, -0.1f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.5f, 0.2f, -0.1f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,

       -0.5f, 0.2f,  -0.15f,  0.0f, 0.0f,                  0.0f, 1.0f, 0.0f,
       -0.4f, 0.2f,  -0.15f,  1.0f, 0.0f,                  0.0f, 1.0f, 0.0f,
       -0.4f, 1.0f,  -0.15f,  1.0f, 1.0f,                  0.0f, 1.0f, 0.0f,
       -0.4f, 1.0f,  -0.15f,  1.0f, 1.0f,                  0.0f, 1.0f, 0.0f,
       -0.4f, 1.0f,  -0.15f,  0.0f, 1.0f,                  0.0f, 1.0f, 0.0f,
       -0.5f, 0.2f,  -0.15f,  0.0f, 0.0f,                  0.0f, 1.0f, 0.0f,

       -0.5f, 1.0f,  -0.15f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.5f, 1.0f, -0.1f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.5f, 0.2f, -0.1f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.5f, 0.2f, -0.1f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.5f, 0.2f,  -0.15f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.5f, 1.0f,  -0.15f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,

       -0.4f, 1.0f,  -0.15f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.4f, 1.0f, -0.1f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.4f, 0.2f, -0.1f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.4f, 0.2f, -0.1f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.4f, 0.2f,  -0.15f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.4f, 1.0f,  -0.15f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,

       -0.5f, 0.2f, -0.1f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.4f, 0.2f, -0.1f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.4f, 0.2f,  -0.15f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.4f, 0.2f,  -0.15f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.4f, 0.2f,  -0.15f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.5f, 0.2f, -0.1f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,

       -0.5f,  0.2f, -0.1f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.4f,  0.2f, -0.1f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
       -0.4f,  0.2f,  -0.15f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.4f,  0.2f,  -0.15f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.5f,  0.2f,  -0.15f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
       -0.5f,  0.2f, -0.1f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
    };

    //Bucket vertex data
    GLfloat verts4[] = {
        //Positions          //Texture Coordinates            //Normals
        0.2f, 0.0f, 0.3f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
        0.6f, 0.0f, 0.3f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
        0.6f, 0.4f, 0.3f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
        0.6f, 0.4f, 0.3f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
        0.2f, 0.4f, 0.3f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
        0.2f, 0.0f, 0.3f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,

        0.2f, 0.0f,  -0.1f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
        0.6f, 0.0f,  -0.1f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
        0.6f, 0.4f,  -0.1f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
        0.6f, 0.4f,  -0.1f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
        0.2f, 0.4f,  -0.1f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
        0.2f, 0.0f,  -0.1f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,

        0.2f, 0.4f,  -0.1f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
        0.2f, 0.4f, 0.3f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
        0.2f, 0.0f, 0.3f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
        0.2f, 0.0f, 0.3f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
        0.2f, 0.0f,  -0.1f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
        0.2f, 0.4f,  -0.1f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,

        0.6f, 0.4f,  -0.1f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
        0.6f, 0.4f, 0.3f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
        0.6f, 0.0f, 0.3f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
        0.6f, 0.0f, 0.3f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
        0.6f, 0.0f,  -0.1f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
        0.6f, 0.4f,  -0.1f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,

        0.2f, 0.0f, 0.3f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
        0.6f, 0.0f, 0.3f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
        0.6f, 0.0f,  -0.1f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
        0.6f, 0.0f,  -0.1f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
        0.2f, 0.0f,  -0.1f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
        0.2f, 0.0f, 0.3f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,

        0.2f,  0.0f, 0.3f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
        0.6f,  0.0f, 0.3f,  1.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
        0.6f,  0.0f,  -0.1f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
        0.6f,  0.0f,  -0.1f,  1.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
        0.2f,  0.0f,  -0.1f,  0.0f, 0.0f,                  0.0f, 0.0f, 1.0f,
        0.2f,  0.0f, 0.3f,  0.0f, 1.0f,                  0.0f, 0.0f, 1.0f,
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerUV = 2;
    const GLuint floatsPerNormal = 3;

    // Strides between vertex coordinates
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerUV + floatsPerNormal);

    mesh.nVertices[0] = sizeof(verts0) / (sizeof(verts0[0]) * (floatsPerVertex + floatsPerUV + floatsPerNormal));

    glGenVertexArrays(1, &mesh.vao[0]); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao[0]);

    // Create VBO
    glGenBuffers(5, mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts0), verts0, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU



    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(2, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);


    mesh.nVertices[1] = sizeof(verts1) / (sizeof(verts1[0]) * (floatsPerVertex + floatsPerUV + floatsPerNormal));

    glGenVertexArrays(1, &mesh.vao[1]); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao[1]);

    // Create VBO
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[1]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts1), verts1, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU



    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(2, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    mesh.nVertices[2] = sizeof(verts2) / (sizeof(verts2[0]) * (floatsPerVertex + floatsPerUV + floatsPerNormal));

    glGenVertexArrays(1, &mesh.vao[2]); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao[2]);

    // Create VBO
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[2]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts2), verts2, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU



    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(2, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    mesh.nVertices[3] = sizeof(verts3) / (sizeof(verts3[0]) * (floatsPerVertex + floatsPerUV + floatsPerNormal));

    glGenVertexArrays(1, &mesh.vao[3]); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao[3]);

    // Create VBO
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[3]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts3), verts3, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU



    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(2, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);

    mesh.nVertices[4] = sizeof(verts4) / (sizeof(verts4[0]) * (floatsPerVertex + floatsPerUV + floatsPerNormal));

    glGenVertexArrays(1, &mesh.vao[4]); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao[4]);

    // Create VBO
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[4]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts4), verts4, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU



    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(2, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)* floatsPerVertex));
    glEnableVertexAttribArray(1);
}


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(5, mesh.vao);
    glDeleteBuffers(5, mesh.vbo);
}


/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}