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
int model_index;
bool model_touched_ground;
static const int NUM_VERTICES_PER_SIDE = 6;
static const int NUM_VERTICES_PER_MODEL = 6*NUM_VERTICES_PER_SIDE;
std::vector<glm::vec3> vertices;
std::vector<glm::vec3> normals;

// used when generating model vertices
int static const SAVEDMODELSMAX = 100;
float savedModelsOffset[SAVEDMODELSMAX];
float savedModelsRotation[SAVEDMODELSMAX];
int savedModels[SAVEDMODELSMAX]; /// whih representative model forwhich object on screen by index
float static const DISTANCE = 20.0f;
float static const UPPER_BOUND = -DISTANCE/2;
float static const LOWER_BOUND = +DISTANCE/2;

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
  curr_y = UPPER_BOUND;
  curr_angle_z = 0;
  model_touched_ground = true; // to generate a model at start
  model_index = 0; // 0 = 1 model

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
void genModels(std::vector<glm::vec3> * vertices, std::vector<glm::vec3> * normals)
{
  genQube(vertices);

  // front side
  for(int i = 0; i < NUM_VERTICES_PER_SIDE; i++){
    normals->push_back(glm::vec3( 0.0f, 0.0f,-1.0f));
  }
  // back side
  for(int i = 0; i < NUM_VERTICES_PER_SIDE; i++){
    normals->push_back(glm::vec3( 0.0f, 0.0f, 1.0f));
  }
  // right sice
  for(int i = 0; i < NUM_VERTICES_PER_SIDE; i++){
    normals->push_back(glm::vec3( 1.0f, 0.0f, 0.0f));
  }
  // top side
  for(int i = 0; i < NUM_VERTICES_PER_SIDE; i++){
    normals->push_back(glm::vec3( 0.0f,-1.0f, 0.0f));
  }
  // right sice
  for(int i = 0; i < NUM_VERTICES_PER_SIDE; i++){
    normals->push_back(glm::vec3(-1.0f,-1.0f, 0.0f));
  }
  // bottom side
  for(int i = 0; i < NUM_VERTICES_PER_SIDE; i++){
    normals->push_back(glm::vec3( 0.0f, 1.0f, 0.0f));
  }
}

int temp = 0; // DEBUG
void updateAnimationLoop()
{
  // Clear the screen
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Use our shader
  glUseProgram(programID);

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

  
  glm::mat4 Rotation;
  glm::mat4 transformation; //initialize matrix for transformations for the model
  // draw **inactive** (unmovable) triangle
  for(int i = 0; i < model_index; i++){
    Rotation = glm::mat4(1.0f);
    Rotation = glm::rotate(Rotation, savedModelsRotation[i], glm::vec3(0.0f, 0.0f, 1.0f));
    transformation = glm::mat4(1.0f);
    transformation = glm::translate(transformation, glm::vec3(savedModelsOffset[i], LOWER_BOUND, 0.0f));
    initializeMVPTransformation();
    MVP = MVP * transformation * Rotation;
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

    glDrawArrays(GL_TRIANGLES, 0, NUM_VERTICES_PER_MODEL); // 6 indices starting at 0 -> 1 square
    // DEBUG
    if (temp != model_index){
      printf("model: %d, rotation: %f, offset: %f\n", i, savedModelsRotation[i], savedModelsOffset[i]);
    }
  }
  temp = model_index; // DEBUG

  curr_y += 0.005; //gravity
  // upper bound check
  if (curr_y > LOWER_BOUND){
    // Save rotation and offset for each "fallen" cube
    savedModelsOffset[model_index] = curr_x;
    savedModelsRotation[model_index] = curr_angle_z;
    // Reset variables
    curr_y = UPPER_BOUND;
    curr_x = 0;
    curr_angle_z = 0;
    cleanupVertexbuffer();
    initializeVertexbuffer();

    // Check overflow
    model_index++;
    if (model_index > SAVEDMODELSMAX){
        exit(1); // exit because too many models saved
    }
  }

  // draw **active** (movable) model
  transformation = glm::mat4(1.0f);
  if (glfwGetKey(window, GLFW_KEY_W)) curr_y -= 0.01;
  else if (glfwGetKey(window, GLFW_KEY_S)) curr_y += 0.01;
  else if (glfwGetKey(window, GLFW_KEY_A)) curr_x -= 0.01;
  else if (glfwGetKey(window, GLFW_KEY_D)) curr_x += 0.01;
  else if (glfwGetKey(window, GLFW_KEY_R)) curr_angle_z += 0.01;

  // Rotation matrix : rotates **active model** according to *curr_angle_z*
  
  Rotation = glm::mat4(1.0f);
  Rotation = glm::rotate(Rotation, curr_angle_z, glm::vec3(0.0f, 0.0f, 1.0f));

  // Tansformation matrix : translates **active model** according to *curr_x* and *curr_y*
  transformation = glm::translate(transformation, glm::vec3(curr_x, curr_y, 0.0f));
  initializeMVPTransformation();
  MVP = MVP * transformation * Rotation;

  glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
  
  // draw **active** (movable) triangle
  glDrawArrays(GL_TRIANGLES, 0, NUM_VERTICES_PER_MODEL);

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
  window = glfwCreateWindow(1024, 768, "Showcase", NULL, NULL);
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

  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  // Accept fragment if it closer to the camera than the former one
  glDepthFunc(GL_LESS);
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
  glm::mat4 View = glm::lookAt(
    glm::vec3(0, 0, -DISTANCE), // Camera is at ( 5, 5,-20), in World Space
    glm::vec3(0, 0, 0), // and looks at the origin
    glm::vec3(0, -1, 0)  // Head is down (set to 0,1,0 to look up)
  );
  // Model matrix : an identity matrix (model will be at the origin)
  glm::mat4 Model = glm::mat4(1.0f);  
  
  // Our ModelViewProjection : multiplication of our 3 matrices
  MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around


  return true;
  
}
void genQube(std::vector<glm::vec3> * modelVertices, const glm::vec3 &offset)
{
  // front side
  //with normalized
  modelVertices->push_back(glm::vec3(-1.0f,-1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3(-1.0f, 1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f, 1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f, 1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f,-1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3(-1.0f,-1.0f, 0.0f) + offset);
  
  // back side
  //with normalized
  modelVertices->push_back(glm::vec3(-1.0f,-1.0f, 2.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f, 1.0f, 2.0f) + offset);
  modelVertices->push_back(glm::vec3(-1.0f, 1.0f, 2.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f, 1.0f, 2.0f) + offset);
  modelVertices->push_back(glm::vec3(-1.0f,-1.0f, 2.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f,-1.0f, 2.0f) + offset);
  
  // right sice
  //with normalized
  modelVertices->push_back(glm::vec3( 1.0f,-1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f, 1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f, 1.0f, 2.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f, 1.0f, 2.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f,-1.0f, 2.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f,-1.0f, 0.0f) + offset);
  
  // top side
  //with normalized
  modelVertices->push_back(glm::vec3(-1.0f,-1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f,-1.0f, 2.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f,-1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3(-1.0f,-1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3(-1.0f,-1.0f, 2.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f,-1.0f, 2.0f) + offset);

  // right sice
  //with normalized
  modelVertices->push_back(glm::vec3(-1.0f,-1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3(-1.0f, 1.0f, 2.0f) + offset);
  modelVertices->push_back(glm::vec3(-1.0f, 1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3(-1.0f, 1.0f, 2.0f) + offset);
  modelVertices->push_back(glm::vec3(-1.0f,-1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3(-1.0f,-1.0f, 2.0f) + offset);
  
  // bottom side
  //with normalized
  modelVertices->push_back(glm::vec3(-1.0f, 1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f, 1.0f, 2.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f, 1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3(-1.0f, 1.0f, 0.0f) + offset);
  modelVertices->push_back(glm::vec3(-1.0f, 1.0f, 2.0f) + offset);
  modelVertices->push_back(glm::vec3( 1.0f, 1.0f, 2.0f) + offset);
}
void genQube(std::vector<glm::vec3> * modelVertices)
{
  genQube (modelVertices, glm::vec3(0.0f, 0.0f, 0.0f));
}
bool initializeVertexbuffer()
{
  glGenVertexArrays(1, &VertexArrayID);
  glBindVertexArray(VertexArrayID);

  // for(int i=0; i<model_index; i++){
  //   // generate model from saved position
  //   // genQube(&vertices, glm::vec3(savedModelsOffset[i], 0.0f, 0.0f));
  //   genQube(&vertices, glm::vec3(0.0f, 0.0f, 0.0f));
  // }
  // generate movable object
  genModels(&vertices, &normals);
  
  // Fill Vertex Buffer
  glGenBuffers(1, &vertexbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(vec3), vertices.data(), GL_STATIC_DRAW);
  vertices.clear();

  // Fill Color Buffer
  static GLfloat g_color_buffer_data[12*3*3];
  for (int v = 0; v < NUM_VERTICES_PER_MODEL ; v++){
      g_color_buffer_data[3*v+0] = 1; // red
      g_color_buffer_data[3*v+1] = 0; // green
      g_color_buffer_data[3*v+2] = 0; // blue
  }
  glGenBuffers(1, &colorbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

  // Fill Normal Buffer
  GLuint normalbuffer;
  glGenBuffers(1, &normalbuffer);
  glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
  glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);

  // 3rd attribute buffer : normals
  glEnableVertexAttribArray(2);
  glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
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

