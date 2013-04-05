/*
 *	Data Visualization for Distributed Denial of Service Attacks
 *		Research by John Russo
 *		Advised by Dr. Scott W. Harrison
 *		
 *		St. John Fisher College
 *		Science Scholar Program
 *		Spring 2013													
 *
 *		Portions of this project are modified from examples
 *		from the book OpenGL Programming which is distributed
 *		from Wikibooks. It is licensed for redistribution and
 *		modification under the GNU Free Documentation License.
 *
 *
 *																	*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL\glew.h>
#include <GL\glut.h>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL\shader_utils.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

GLuint program;
GLint attribute_coord2d;
GLint uniform_vertex_transform;
GLint uniform_texture_transform;
GLuint texture_id;
GLint uniform_mytexture;

float offset_x = 0.0;
float offset_y = 0.0;
float scale = 1.5;

bool interpolate = false;
bool clamp = false;
bool rotate = false;

GLuint vbo[2];

struct point {
		GLfloat x;
		GLfloat y; };

int init_resources()
{
  int vertex_texture_units;
  glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &vertex_texture_units);

  if(!vertex_texture_units) 
  {
    fprintf(stderr, "Your graphics cards does not support texture lookups in the vertex shader!\n");
    return 0;
  }

  GLint link_ok = GL_FALSE;

  GLuint vs, fs;

  if ((vs = create_shader("graph.v.glsl", GL_VERTEX_SHADER))   == 0) return 0;	
  if ((fs = create_shader("graph.f.glsl", GL_FRAGMENT_SHADER)) == 0) return 0;

  program = glCreateProgram();	
  glAttachShader(program, vs);	
  glAttachShader(program, fs);
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
	
  if (!link_ok) 
  {
    fprintf(stderr, "glLinkProgram:");
    print_log(program);	
    return 0;
  }

  const char* attribute_name;
  attribute_name = "coord2d";
  attribute_coord2d = glGetAttribLocation(program, attribute_name);
  if (attribute_coord2d == -1) 
  {
    fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
    return 0;
  }

  const char* uniform_name;

  uniform_name = "vertex_transform";
  uniform_vertex_transform = glGetUniformLocation(program, uniform_name);

  if (uniform_vertex_transform == -1) 
  {
    fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
    return 0;	
  }

  uniform_name = "texture_transform";	
  uniform_texture_transform = glGetUniformLocation(program, uniform_name);

  if (uniform_texture_transform == -1) 
  {
    fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
    return 0;	
  }

  // Create our datapoints, store it as bytes
	
  #define N 256

  GLbyte graph[N][N];
  int a, b, c, temp;
  float total_1;

  using std::ifstream;

  using std::istringstream;
  istringstream iss;

  using std::string;
  string line;

  ifstream myfile;
  myfile.open("pingflood.g3d.nz");

  if(!myfile)
  {
	  fprintf(stderr, "Error: file could not be opened");
	  return 0;
  }

  int j = 0; // x-cordinate
  int k = 0; // y-cordinate

  total_1 = 0;

  /* Initializes all data points to value 0 */
  for(int g = 0; g < N; g++)
  {
	 for(int h = 0; h < N; h++)
	 {
		 graph[g][h] = 0;
	 }
  }

  /* Reads data file and stores values as data points on graph.
	 All middle range values are divided by 1000 in order to give more
	 attention to outlier values */
  while( std::getline (myfile,line) )
  {
	   istringstream iss(line);
	   iss >> a;
       iss >> b;
	   iss >> total_1;

	   if(total_1 < 50000 && total_1 < 100000)
	   {
			total_1 = total_1 / 1000;
	   }

	   if( k < 256 )
	   {
		   graph[j][k] = total_1;
		   k++;
	   }
	   else
	   {
		   j++;
		   k = 0;
		   graph[j][k] = total_1;
	   }
  }
  
  myfile.close(); 
  
  /* Upload the texture with our datapoints */

  glActiveTexture(GL_TEXTURE0);	
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexImage2D(

     GL_TEXTURE_2D,      // target
     0,                  // level, 0 = base, no minimap,
     GL_LUMINANCE,       // internalformat
     N,                  // width
     N,                  // height
     0,                  // border, always 0 in OpenGL ES
     GL_LUMINANCE,       // format
     GL_UNSIGNED_BYTE,   // type
     graph
  );

  // Create two vertex buffer objects

  glGenBuffers(2, vbo);

  // Create an array for 101 * 101 vertices
	
  point vertices[116][116];

  for(int i = 0; i < 116; i++) 
  {	
    for(int j = 0; j < 116; j++) 
	{
      vertices[i][j].x = (j - 50) / 50.0;
      vertices[i][j].y = (i - 50) / 50.0;
    }
  }
  
  // Tell OpenGL to copy our array to the buffer objects
	
  glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);

  // Create an array of indices into the vertex array that traces both horizontal and vertical lines

  GLushort indices[115 * 116 * 4];

  int i = 0;

  for(int y = 0; y < 116; y++) 
  {
    for(int x = 0; x < 115; x++) 
	{
      indices[i++] = y * 116 + x;	
      indices[i++] = y * 116 + x + 1;	
    }
  }

  for(int x = 0; x < 116; x++) 
  {
	for(int y = 0; y < 115; y++) 
	{
      indices[i++] = y * 116 + x;
      indices[i++] = (y + 1) * 116 + x;	
    }
  }
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof indices, indices, GL_STATIC_DRAW);
  return 1;
}

void display()
{
  glUseProgram(program);
  glUniform1i(uniform_mytexture, 0);
  glm::mat4 model;

  if(rotate)
    model = glm::rotate(glm::mat4(1.0f), float(glutGet(GLUT_ELAPSED_TIME) / 100.0), glm::vec3(0.0f, 0.0f, 1.0f));
  else
    model = glm::mat4(1.0f);  

  glm::mat4 view = glm::lookAt(glm::vec3(0.0, -2.0, 2.0), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
  glm::mat4 projection = glm::perspective(45.0f, 1.0f*640/480, 0.1f, 10.0f);
  glm::mat4 vertex_transform = projection * view * model;

  glm::mat4 texture_transform = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, 1)), glm::vec3(offset_x, offset_y, 0));

  glUniformMatrix4fv(uniform_vertex_transform, 1, GL_FALSE, glm::value_ptr(vertex_transform));

  glUniformMatrix4fv(uniform_texture_transform, 1, GL_FALSE, glm::value_ptr(texture_transform));

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);

  /* Set texture wrapping mode */

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);

  /* Set texture interpolation mode */
	
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolate ? GL_LINEAR : GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolate ? GL_LINEAR : GL_NEAREST);

  /* Draw the grid using the indices to our vertices using our vertex buffer objects */

  glEnableVertexAttribArray(attribute_coord2d);
  glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
  glVertexAttribPointer(attribute_coord2d, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
  glDrawElements(GL_LINES, 115 * 116 * 4, GL_UNSIGNED_SHORT, 0);

  /* Stop using the vertex buffer object */

  glDisableVertexAttribArray(attribute_coord2d);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glutSwapBuffers();
}

/* Allows user to navigate or rotate the graph in order to get
	a better look at the data */
void special(int key, int x, int y)
{
  switch(key) 
  {
    case GLUT_KEY_F1:
      rotate = !rotate;
      break;

    case GLUT_KEY_LEFT:
      offset_x -= 0.03;
      break;

    case GLUT_KEY_RIGHT:
      offset_x += 0.03;
      break;

    case GLUT_KEY_UP:
      offset_y += 0.03;
      break;

    case GLUT_KEY_DOWN:
      offset_y -= 0.03;
      break;
	
    case GLUT_KEY_HOME:
      offset_x = 0.0;
      offset_y = 0.0;
      scale = 1.5;	
      break;
  }
  
  glutPostRedisplay();
}

void free_resources()
{
  glDeleteProgram(program);
}

int main(int argc, char* argv[]) 
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE);
  glutInitWindowSize(1100,1000);
  glutCreateWindow("pingflood.g3d.nz");
  GLenum glew_status = glewInit();

  if (GLEW_OK != glew_status) 
  {
    fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
    return 1;
  }

  if (!GLEW_VERSION_2_0) 
  {
    fprintf(stderr, "No support for OpenGL 2.0 found\n");
    return 1;
  }

  GLint max_units;
  glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &max_units);

  if(max_units < 1) 
  {
	  fprintf(stderr, "Your GPU does not have any vertex texture image units\n");
	  return 1;
  }

  if (init_resources()) 
  {
    glutDisplayFunc(display);
    glutIdleFunc(display);
    glutSpecialFunc(special);
    glutMainLoop();
  }
  free_resources();
  return 0;
}