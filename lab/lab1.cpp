//
// Created by florian on 04.03.21.
//

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>

using namespace std;

int windowid;

void renderScene() {
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(1.0, 1.0, 0.0, 1.0);
  glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
  switch (key) {
    case 'a':
      cout << "You just pressed 'a'" << endl;
      break;
    case 27:
      glutDestroyWindow(windowid);
      exit(0);
      break;
    case 32:
      cout << "SPACE" << endl;
      glClear(GL_COLOR_BUFFER_BIT);
      glClearColor(0.0, 1.0, 1.0, 1.0);
      glutSwapBuffers();
      break;
    default:
      break;
  }
  glutPostRedisplay();
}

void onMouseClick(int button, int state, int x, int y) {
  if(button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN) {
    cout << "Middle button clicked at position "
    << "x: " << x << " y: " << y << endl;
  }
}

int main(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
  glutInitWindowPosition(500, 500);
  glutInitWindowSize(800, 600);
  windowid = glutCreateWindow("OpenGL First Windows");

  glutDisplayFunc(renderScene);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(onMouseClick);

  glutMainLoop();
  return 0;
}
