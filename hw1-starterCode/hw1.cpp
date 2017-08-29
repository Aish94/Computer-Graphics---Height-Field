/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields

  Student username: averghes
*/

#include <iostream>
#include <stdlib.h>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iomanip>

#include "openGLHeader.h"
#include "glutHeader.h"
#include "imageIO.h"
#include "openGLMatrix.h"
#include "basicPipelineProgram.h"

#ifdef WIN32
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#ifdef WIN32
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

//change transformation or shape on keypress
typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;
typedef enum { POINTS, WIREFRAME, SOLID_TRIANGLES } SHAPE;
SHAPE shape = SOLID_TRIANGLES;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };
GLfloat theta[3] = {0.0, 0.0, 0.0};

//vertex data arrays
float* triangles_positions;
float* points_positions;
float* lines_positions;
GLuint size_of_points_positions;
GLuint size_of_lines_positions;
GLuint size_of_triangles_positions;
int points_num_of_vertrices, lines_num_of_vertrices, triangles_num_of_vertrices;

//screenshot params
int screenshot_iteration = 0;
int prev_time = 0;
int time_now;


OpenGLMatrix* matrix;
BasicPipelineProgram * pipelineProgram;
GLuint points_vao,lines_vao,triangles_vao;
GLuint points_vbo,lines_vbo,triangles_vbo;
GLuint program;

int windowWidth = 1280;
int windowHeight = 720;
int image_width;
int image_height;
char windowTitle[512] = "CSCI 420 homework I";

ImageIO * heightmapImage;

// write a screenshot to the specified filename
void saveScreenshot(string filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  string path = "screenshots/"+filename;  //save in screenshot folder
  if (screenshotImg.save(path.c_str(), ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}


void bindProgram()
{
    //use shader
    pipelineProgram->Bind();

    // get a handle to the modelViewMatrix shader variable
    GLint h_modelViewMatrix = glGetUniformLocation(program, "modelViewMatrix");
    float m[16];
    matrix -> SetMatrixMode(OpenGLMatrix::ModelView);
    matrix->GetMatrix(m);  // column-major
    GLboolean isRowMajor = GL_FALSE;
    glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);  //update shader's model view matrix
    
    // get a handle to the projectionMatrix shader variable
    GLint h_projectionMatrix = glGetUniformLocation(program, "projectionMatrix");
    float p[16]; // column-major
    matrix -> SetMatrixMode(OpenGLMatrix::Projection);
    matrix->GetMatrix(p);
    isRowMajor = GL_FALSE;
    glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, p); //update shader's projection matrix
}

//render solid triangles
void renderTriangles()
{
  glBindVertexArray(triangles_vao); // bind the VAO
    GLint first = 0;
    GLsizei count = triangles_num_of_vertrices;
  glDrawArrays(GL_TRIANGLE_STRIP, first, count);
  glBindVertexArray(0); // unbind the VAO
}

//render points
void renderPoints()
{
  glBindVertexArray(points_vao); // bind the VAO
    GLint first = 0;
    GLsizei count = points_num_of_vertrices;
  glDrawArrays(GL_POINTS, first, count);
  glBindVertexArray(0); // unbind the VAO
}

//render lines(wireframe)
void renderWireframe()
{
  glBindVertexArray(lines_vao); // bind the VAO
    GLint first = 0;
    GLsizei count = lines_num_of_vertrices;
  glDrawArrays(GL_LINES, first, count);
  glBindVertexArray(0); // unbind the VAO
}

//Update matrices with all transformations performed
void transformations()
{
  //Model View
  matrix -> SetMatrixMode(OpenGLMatrix::ModelView);
  matrix -> LoadIdentity(); 
  //View
  matrix->LookAt(1, 1, 1, 0, 0, 0, 0, 1.0, 0); //(raised, to the right, behind)camera looking at origin
  //Scale
  matrix->Scale(landScale[0], landScale[1], landScale[2]);
  //Rotate
  if(landRotate[0] != 0)
    matrix->Rotate(theta[0],landRotate[0],0.0f,0.0f);
  if(landRotate[1] != 0)
    matrix->Rotate(theta[1],0.0f,landRotate[1],0.0f);
  if(landRotate[2] != 0)
    matrix->Rotate(theta[2],0.0f,0.0f,landRotate[2]);
  //Translate
  matrix->Translate(landTranslate[0],landTranslate[1],landTranslate[2]);

  //Projections
  matrix -> SetMatrixMode(OpenGLMatrix::Projection);
  matrix -> LoadIdentity();
  matrix -> Perspective(45.0, 1280/720, 0.1, 100.0);
}

//The initial transformation values
void initMatrices()
{
    matrix = new OpenGLMatrix();

    //Scale image to NDC
    landScale[0] = 1.0f/(GLfloat)image_width;
    landScale[1] = 1.0f/255.0f;
    landScale[2] = 1.0f/(GLfloat)image_height;

    //Push center of image to origin
    landTranslate[0] = -image_width/2;
    landTranslate[1] = 0.0f;
    landTranslate[2] = image_height/2;

    //No rotations
    landRotate[0] = 0.0f;
    landRotate[1] = 0.0f;
    landRotate[2] = 0.0f;

    theta[0] = 0.0f;
    theta[1] = 0.0f;
    theta[2] = 0.0f;
}

void displayFunc()
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    transformations();  //update matrices
    bindProgram();  //bind shader variables
    //render according to shape mode
    switch(shape)
    {
      case POINTS:
        renderPoints();
        break;
      case WIREFRAME:
        renderWireframe();
        break;
      case SOLID_TRIANGLES:
        renderTriangles();
        break;
    }
    
    glutSwapBuffers(); //double - buffered
}

void idleFunc()
{
  if(screenshot_iteration < 300)  //max of 300 screenshot
  {
    time_now = glutGet(GLUT_ELAPSED_TIME);  //number of milliseconds since the call to glutInit
    if(time_now - prev_time > 66) //15fps take screenshot if 66ms has passed since last capture
    {
      stringstream ss;
      ss << std::setw(3) << std::setfill('0') << screenshot_iteration;  //add leading zeros
      string iter = ss.str();
      prev_time = time_now;

      saveScreenshot(iter + ".jpg");  //take a screenshot
      screenshot_iteration++;
    }
  }
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  //update Projection matrix
  GLfloat aspect = (GLfloat)windowWidth/(GLfloat)windowHeight;
  matrix -> SetMatrixMode(OpenGLMatrix::Projection);
  matrix -> LoadIdentity();
  matrix -> Perspective(45.0, aspect, 0.1, 100.0);
  matrix -> SetMatrixMode(OpenGLMatrix::ModelView);
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { (x - mousePos[0]), (y - mousePos[1]) };
  matrix -> SetMatrixMode(OpenGLMatrix::ModelView);
  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.1f;
        landTranslate[1] -= mousePosDelta[1] * 0.1f;
      }
      if (rightMouseButton)
      {
        // control z translation via the right mouse button
        landTranslate[2] += mousePosDelta[1] * 0.1f;
      }
      break;
    // rotate the landscape
    case ROTATE:
      if (leftMouseButton)  //Rotate about y
      {
        theta[1] += mousePosDelta[0] * 0.1f;
        landRotate[1] = 1.0;
      }
      if (rightMouseButton) //Rotate about x
      {
        theta[0] += mousePosDelta[1] * 0.1f;
        landRotate[0] = 1.0;
      }
      break;

    // scale the landscape
    case SCALE:
      if (leftMouseButton)
      {
        // control x & y scaling via the left mouse button
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.001f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.001f;
      }
      if (rightMouseButton)
      {
        // control z scaling via the right mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.001f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;

  glutPostRedisplay();
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton and rightMouseButton variables
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN); //MAC: control+trackpad
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      // Properly de-allocate all resources
      glDeleteVertexArrays(1, &points_vao);
      glDeleteBuffers(1, &points_vbo);
      glDeleteVertexArrays(1, &lines_vao);
      glDeleteBuffers(1, &lines_vbo);
      glDeleteVertexArrays(1, &triangles_vao);
      glDeleteBuffers(1, &triangles_vbo);
      exit(0); // exit the program
    break;

    //change Shape Mode
    case 'p':
      shape = POINTS;
      glutPostRedisplay();  
    break;

    case 'w':
      shape = WIREFRAME;
      glutPostRedisplay();  
    break;

    case 't':
      shape = SOLID_TRIANGLES;
      glutPostRedisplay();  
    break;

    //Change Transformation Mode
    case 'r':
      controlState = ROTATE;
    break;

    case 's':
      controlState = SCALE;
    break;

    case 'm':
      controlState = TRANSLATE;
    break;

    case 'n':     //Reset transformations
      initMatrices();
      glutPostRedisplay();
      break;

    case 'x':
      // take a screenshot
      saveScreenshot("screenshot.jpg");
    break;
  }
}

void initAttributes()
{
  GLuint loc = glGetAttribLocation(program,"position");
  glEnableVertexAttribArray(loc);
  const void * offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset);
}

//initialize VBO & VAO
void initVBO()
{
    //Points
    glGenVertexArrays(1, &points_vao);
    glBindVertexArray(points_vao); // bind the VAO
    
    glGenBuffers(1, &points_vbo);
    glBindBuffer(GL_ARRAY_BUFFER,points_vbo);
    glBufferData(GL_ARRAY_BUFFER,size_of_points_positions,points_positions,GL_STATIC_DRAW);
    initAttributes();

    glBindVertexArray(0); // Unbind VAO


     //Lines
    glGenVertexArrays(1, &lines_vao);
    glBindVertexArray(lines_vao); // bind the VAO
    
    glGenBuffers(1, &lines_vbo);
    glBindBuffer(GL_ARRAY_BUFFER,lines_vbo);
    glBufferData(GL_ARRAY_BUFFER,size_of_lines_positions,lines_positions,GL_STATIC_DRAW);
    initAttributes();

    glBindVertexArray(0); // Unbind VAO

     //Triangles
     glGenVertexArrays(1, &triangles_vao);
    glBindVertexArray(triangles_vao); // bind the VAO
    
    glGenBuffers(1, &triangles_vbo);
    glBindBuffer(GL_ARRAY_BUFFER,triangles_vbo);
    glBufferData(GL_ARRAY_BUFFER,size_of_triangles_positions,triangles_positions,GL_STATIC_DRAW);
    initAttributes();

    glBindVertexArray(0); // Unbind VAO    
}

//set up and compile shaders
void initPipelineProgram()
{
    pipelineProgram = new BasicPipelineProgram();
    pipelineProgram->Init("../openGLHelper-starterCode"); //Links & compiles shaders
    program = pipelineProgram->GetProgramHandle();
}

//Get vertex Data from image
void getVertexData()
{
  //width & height
  image_width = heightmapImage -> getWidth();
  image_height = heightmapImage -> getHeight();

  //No. of vertices
  triangles_num_of_vertrices = (image_width * image_height) + ((image_height-2) * image_width) + ((image_height-2)*2);
  points_num_of_vertrices = (image_width * image_height);
  lines_num_of_vertrices = ((image_width * image_height) + (image_height*(image_width-2))) + (image_width * image_height) + (image_width*(image_height-2));
  
  //Initialize arrays
  triangles_positions = new float[triangles_num_of_vertrices * 3];
  points_positions = new float[points_num_of_vertrices * 3];
  lines_positions = new float[lines_num_of_vertrices * 3];

  //Size of arrays in bytes
  size_of_points_positions = sizeof(float) * points_num_of_vertrices * 3;
  size_of_triangles_positions = sizeof(float) * triangles_num_of_vertrices * 3;
  size_of_lines_positions = sizeof(float) * lines_num_of_vertrices * 3;

  int p_index = 0; //poisition array index
  unsigned char pixel;
  //Triangles
  //For each row in image
  for(int i = 0; i < image_height - 1 ; i++)
  {
      //For each column in the image
      for(int j = 0; j < image_width; j++)
      {
          pixel = heightmapImage->getPixel(i,j,1);
          triangles_positions[p_index++] = j;
          triangles_positions[p_index++] = int(pixel);
          triangles_positions[p_index++] = -i;

          pixel = heightmapImage->getPixel(i+1,j,1);
          triangles_positions[p_index++] = j;
          triangles_positions[p_index++] = int(pixel);
          triangles_positions[p_index++] = -1*(i+1);
      }

      //after each row add a degenerate triangle 
      if(i < image_height -2)
      {
        int j = image_width-1;
        pixel = heightmapImage->getPixel(i+1,j,1);
        triangles_positions[p_index++] = j;
        triangles_positions[p_index++] = int(pixel);
        triangles_positions[p_index++] = -1*(i+1);

        pixel = heightmapImage->getPixel(i+1,0,1);
        triangles_positions[p_index++] = 0;
        triangles_positions[p_index++] = int(pixel);
        triangles_positions[p_index++] = -1*(i+1);
    }
  }

  //Points
  p_index = 0;
  for(int i = 0; i < image_height; i++)
  {
    for(int j = 0; j < image_width; j++)
    {
          unsigned char pixel = heightmapImage->getPixel(i,j,1);
          points_positions[p_index++] = j;
          points_positions[p_index++] = int(pixel);
          points_positions[p_index++] = -i;
    }
  }

  
  //Wireframe
  p_index = 0;
  //Horizontal Lines
  for(int i = 0; i < image_height; i++)
  {

    for(int j = 0; j < image_width-1; j++)
    {
          unsigned char pixel = heightmapImage->getPixel(i,j,1);
          lines_positions[p_index++] = j;
          lines_positions[p_index++] = int(pixel);
          lines_positions[p_index++] = -i;

          //repeat next point except for boundary points
            pixel = heightmapImage->getPixel(i,j+1,1);
            lines_positions[p_index++] = j+1;
            lines_positions[p_index++] = int(pixel);
            lines_positions[p_index++] = -i;
    }
  }
  //Vertical Lines
  for(int j = 0; j < image_width; j++)
  {

    for(int i = 0; i < image_height-1; i++)
    {
          unsigned char pixel = heightmapImage->getPixel(i,j,1);
          lines_positions[p_index++] = j;
          lines_positions[p_index++] = int(pixel);
          lines_positions[p_index++] = -i;

          //repeat next point except for boundary points
            pixel = heightmapImage->getPixel(i+1,j,1);
            lines_positions[p_index++] = j;
            lines_positions[p_index++] = int(pixel);
            lines_positions[p_index++] = -(i+1);
  }
}
  
}

void initScene(int argc, char *argv[])
{
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); //Black background
  glEnable(GL_DEPTH_TEST);
  initPipelineProgram();

  // load the image from a jpeg disk file to main memory
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }
  getVertexData();
  initVBO();
  initMatrices();
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  // tells glut to use a particular display function to redraw 
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}


