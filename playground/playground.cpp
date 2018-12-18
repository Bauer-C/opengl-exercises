#include "playground.h"

// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLFW
#include <glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>


// const int num_coordinates = 3;
// const int num_verticesModel = 3*1;
// const int max_models = 1;
// static GLfloat g_vertex_buffer_data[num_coordinates*num_verticesModel*max_models];

int main( void )
{
  //Initialize window
  bool windowInitialized = initializeWindow();
  if (!windowInitialized) return -1;

  //Initialize vertex buffer
  bool vertexbufferInitialized = initializeVertexbuffer();
  if (!vertexbufferInitialized) return -1;

  // Create and compile our GLSL program from the shaders
  programID = LoadShaders("SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader");

  initializeMVPTransformation();

  curr_x = 0;
  curr_y = 0;

	//start animation loop until escape key is pressed
	do{

    updateAnimationLoop();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	
  //Cleanup and close window
  cleanupVertexbuffer();
  glDeleteProgram(programID);
	closeWindow();
  
	return 0;
}
void genModels(std::vector<glm::vec3> * vertices)
{
  //todo
}

void updateAnimationLoop()
{
  // Clear the screen
  glClear(GL_COLOR_BUFFER_BIT);

  // Use our shader
  glUseProgram(programID);

  initializeMVPTransformation();

  // Send our transformation to the currently bound shader, 
  // in the "MVP" uniform
  glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // 1rst attribute buffer : vertices
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glVertexAttribPointer(
    0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
    3,  // size
    GL_FLOAT,           // type
    GL_FALSE,           // normalized?
    0,                  // stride
    (void*)0            // array buffer offset
  );

  // draw unmovable triangle
  glDrawArrays(GL_TRIANGLES, 0, 3); // 3 indices starting at 0 -> 1 triangle

  glm::mat4 transformation = glm::mat4(1.0f);//additional transformation for the model
  if (glfwGetKey(window, GLFW_KEY_W)) curr_y += 0.01;
  else if (glfwGetKey(window, GLFW_KEY_S)) curr_y -= 0.01;
  else if (glfwGetKey(window, GLFW_KEY_A)) curr_x -= 0.01;
  else if (glfwGetKey(window, GLFW_KEY_D)) curr_x += 0.01;
  else if (glfwGetKey(window, GLFW_KEY_R)) curr_angle += 0.01;
 transformation = glm::translate(transformation, glm::vec3(curr_x, curr_y, 0.0f));

  MVP = MVP * transformation;

  glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
  glDrawArrays(GL_TRIANGLES, 3, 3);

  glDisableVertexAttribArray(0);

  // Swap buffers
  glfwSwapBuffers(window);
  glfwPollEvents();
}

bool initializeWindow()
{
  // Initialise GLFW
  if (!glfwInit())
  {
    fprintf(stderr, "Failed to initialize GLFW\n");
    getchar();
    return false;
  }

  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Open a window and create its OpenGL context
  window = glfwCreateWindow(1024, 768, "Tutorial 02 - Red triangle", NULL, NULL);
  if (window == NULL) {
    fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
    getchar();
    glfwTerminate();
    return false;
  }
  glfwMakeContextCurrent(window);

  // Initialize GLEW
  glewExperimental = true; // Needed for core profile
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    getchar();
    glfwTerminate();
    return false;
  }

  // Ensure we can capture the escape key being pressed below
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

  // Dark blue background
  glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
  return true;
}

bool initializeMVPTransformation()
{
  // Get a handle for our "MVP" uniform
  GLuint MatrixIDnew = glGetUniformLocation(programID, "MVP");
  MatrixID = MatrixIDnew;

  
  // Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
  glm::mat4 Projection = glm::perspective(glm::radians(60.0f), 4.0f / 3.0f, 0.1f, 100.0f);
  //glm::mat4 Projection = glm::frustum(-2.0f, 2.0f, -2.0f, 2.0f, -2.0f, 2.0f);
  // Camera matrix
  float distance = 30;
  glm::mat4 View = glm::lookAt(
    glm::vec3(0, 0, -distance), // Camera is at (0,0,-20), in World Space
    glm::vec3(0, 0, 0), // and looks at the origin
    glm::vec3(0, -1, 0)  // Head is down (set to 0,1,0 to look up)
  );
  // Model matrix : an identity matrix (model will be at the origin)
  glm::mat4 Model = glm::mat4(1.0f);

  Model = glm::rotate(Model, curr_angle, glm::vec3(0.0f, 0.0f, 1.0f));
  
  
  // Our ModelViewProjection : multiplication of our 3 matrices
  MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around


  return true;
  
}
void genSquare(std::vector<glm::vec3> * modelVertices, const glm::vec3 &offset)
{
  modelVertices->push_back(glm::vec3(-1.0f,-1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3(-1.0f, 1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f, 1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f, 1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f,-1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3(-1.0f,-1.0f, 0.0f) + offset);
}
bool initializeVertexbuffer()
{
  glGenVertexArrays(1, &VertexArrayID);
  glBindVertexArray(VertexArrayID);

  std::vector<glm::vec3> vertices;
  genSquare(&vertices, glm::vec3(0));

  glGenBuffers(1, &vertexbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(vec3), vertices.data(), GL_STATIC_DRAW);
  return true;
}

bool cleanupVertexbuffer()
{
  // Cleanup VBO
  glDeleteBuffers(1, &vertexbuffer);
  glDeleteVertexArrays(1, &VertexArrayID);
  return true;
}

bool closeWindow()
{
  glfwTerminate();
  return true;
}

