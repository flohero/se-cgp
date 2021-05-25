//
// Created by florian on 26.04.21.
//
#include "GL/glew.h"
#include "GL/freeglut.h"
#include <iostream>

using namespace std;


// Create checkerboard texture
#define checkImageWidth 64
#define checkImageHeight 64

static GLubyte checkImage[checkImageHeight][checkImageWidth][4];
static GLuint texName;


// Data read from the header of the BMP file
unsigned char header[54]; // Each BMP file begins by a 54-bytes header
unsigned int dataPos;     // Position in the file where the actual data begins
unsigned int imageWidth, imageHeight;
unsigned int imageSize;   // = width*height*3
// Actual RGB data
unsigned char * imageData;

std::string str = "marbles.bmp";

char *filenameandpath = &str[0];

// GLUT Window ID
int windowid;

int loadBMP_custom(char *imagepath) {
  FILE * file;
  file = fopen(imagepath, "rb"); // Open the file
  if (!file) {
    cout << "Image could not be opened" << endl;
    return 0;
  }

  if (fread(header, 1, 54, file) != 54) { // If not 54 bytes read : problem
    cout << "Not a correct BMP file" << endl;
    return 0;
  }

  if (header[0] != 'B' || header[1] != 'M') {
    cout << "Not a correct BMP file" << endl;
    return 0;
  }

  // Read ints from the byte array
  dataPos = *(int*)&(header[0x0A]);
  imageSize = *(int*)&(header[0x22]);
  imageWidth = *(int*)&(header[0x12]);
  imageHeight = *(int*)&(header[0x16]);
  // Some BMP files are misformatted, guess missing information
  // 3 : one byte for each Red, Green and Blue component
  if (imageSize == 0)    imageSize = imageWidth * imageHeight * 3;
  if (dataPos == 0)      dataPos = 54; // The BMP header is done that way
  cout << "imageSize:" << imageSize <<  endl;
  cout << "dataPos:" << dataPos << endl;
  imageData = new unsigned char[imageSize]; // Create a buffer
  fread(imageData, 1, imageSize, file); // Read the actual data from the file into the buffer

  fclose(file); //Everything is in memory now, the file can be closed
  return 0;
}


void makeCheckImage(){
  int i, j, c;

  for (i = 0; i < checkImageHeight; i++) {
    for (j = 0; j < checkImageWidth; j++) {
      c = ((((i & 0x8) == 0) ^ (((j & 0x8)) == 0))) * 255;
      checkImage[i][j][0] = (GLubyte)c;
      checkImage[i][j][1] = (GLubyte)c;
      checkImage[i][j][2] = (GLubyte)c;
      checkImage[i][j][3] = (GLubyte)255;
    }
  }
}

void initTextures(){
  makeCheckImage();


  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, &texName);
  glBindTexture(GL_TEXTURE_2D, texName);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, checkImageWidth, checkImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkImage);

}

void display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

  glBindTexture(GL_TEXTURE_2D, texName);
  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0); glVertex3f(-2.0, -1.0, 0.0);
  glTexCoord2f(0.0, 1.0); glVertex3f(-2.0, 1.0, 0.0);
  glTexCoord2f(1.0, 1.0); glVertex3f(0.0, 1.0, 0.0);
  glTexCoord2f(1.0, 0.0); glVertex3f(0.0, -1.0, 0.0);

  glTexCoord2f(0.0, 0.0); glVertex3f(1.0, -1.0, 0.0);
  glTexCoord2f(0.0, 1.0); glVertex3f(1.0, 1.0, 0.0);
  glTexCoord2f(1.0, 1.0); glVertex3f(2.41421, 1.0, -1.41421);
  glTexCoord2f(1.0, 0.0); glVertex3f(2.41421, -1.0, -1.41421);
  glEnd();

  glFlush();
  glDisable(GL_TEXTURE_2D);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glTranslatef(0.0, 0.0, -4.5);
  glutSwapBuffers();
}

/*-[Keyboard Callback]-------------------------------------------------------*/
void keyboard(unsigned char key, int x, int y) {
  switch (key) {
    case 'a': // lowercase character 'a'
      cout << "You just pressed 'a'" << endl;

      break;
    case 'd': // lowercase character 'd'
      cout << "You just pressed 'd'" << endl;

      break;
    case 'w': // lowercase character 'w'
      cout << "You just pressed 'w'" << endl;

      break;
    case 's': // lowercase character 's'
      cout << "You just pressed 's'" << endl;

      break;
    case 27: // Escape key
      glutDestroyWindow(windowid);
      exit(0);
      break;
  }
  glutPostRedisplay();
}

/*-[MouseClick Callback]-----------------------------------------------------*/
void onMouseClick(int button, int state, int x, int y) {
  if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN) {
    cout << "Middle button clicked at position "
         << "x: " << x << " y: " << y << endl;
  }
}

/*-[Reshape Callback]--------------------------------------------------------*/
void reshapeFunc(int x, int y) {
  if (y == 0 || x == 0) return;  //Nothing is visible then, so return

  glMatrixMode(GL_PROJECTION); //Set a new projection matrix
  glLoadIdentity();
  //Angle of view: 40 degrees
  //Near clipping plane distance: 0.5
  //Far clipping plane distance: 20.0

  gluPerspective(40.0, (GLdouble)x / (GLdouble)y, 0.5, 40.0);
  glViewport(0, 0, x, y);  //Use the whole window for rendering
}

void idleFunc(void) {

}


int main(int argc, char **argv) {

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowPosition(500, 500); //determines the initial position of the window
  glutInitWindowSize(800, 600);	  //determines the size of the window
  windowid = glutCreateWindow("Our Fourth OpenGL Window"); // create and name window

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_DEPTH_TEST);

  // register callbacks
  initTextures();

  glutDisplayFunc(display);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(onMouseClick);
  glutReshapeFunc(reshapeFunc);
  glutIdleFunc(idleFunc);

  // GLUT Full Screen
  //glutFullScreen();

  glutMainLoop(); // start the main loop of GLUT
  return 0;
}


