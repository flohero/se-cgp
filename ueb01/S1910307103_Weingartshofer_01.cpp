//
// Created by florian weingartshofer on 31.03.21.
// Stundenaufwand: 21.5h

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <vector>

#include "GL/glew.h"
#include "GL/freeglut.h"

/**
 * Classes and structs
 */

/**
 * Enum for movement for the x and z axis
 */
enum class movement_direction {
  left = 'a',
  right = 'd',
  forward = 'w',
  backward = 's',
  rotate_clockwise = 'e',
  rotate_counterclockwise = 'q',
  jump = 32, // Keycode for spacebar
};

/**
 * Enum for movement on the y axis
 */
enum class vertical_motion {
  standing, jumping, falling
};

/**
 * Define coordinates for glu
 */
struct coord3d {
  float x;
  float y;
  float z;

  explicit coord3d(float x = 0, float y = 0, float z = 0) : x{x}, y{y}, z{z} {}

  friend bool operator!=(const coord3d &left, const coord3d &right) {
    return left.x != right.x || left.y != right.y || left.y != right.y;
  }

  /**
   * Operator to increment the x,y and z values.
   * @param other, the coordinate by which this one should be incremented
   */
  coord3d &operator+=(const coord3d &other) {
    this->x += other.x;
    this->y += other.y;
    this->z += other.z;
    return *this;
  }
};

/**
 * Context for the labyrinth, with camera position
 */
class context {
public:

  context() {
    this->camera_position_.y = ground_level_;
  }

  [[nodiscard]] coord3d &cam() noexcept {
    return this->camera_position_;
  }

  float lx() const noexcept {
    return std::sin(this->horizontal_angle_);
  }

  float lz() const noexcept {
    return -std::cos(this->horizontal_angle_);
  }

  float x() const noexcept {
    return this->lx() * movement_speed_;
  }

  float z() const noexcept {
    return this->lz() * movement_speed_;
  }

  /**
   * Convenience function to increase horizontal angle
   */
  void inc_horizontal_angle() noexcept {
    inc_horizontal_angle_by(turn_factor_);
  }

  /**
 * Convenience function to decrease horizontal angle
 */
  void dec_horizontal_angle() noexcept {
    inc_horizontal_angle_by(-turn_factor_);
  }

  /**
   * increase horizontal angle
   */
  void inc_horizontal_angle_by(const float inc) noexcept {
    this->horizontal_angle_ += inc;
  }

  void inc_vertical_angle_by(const float inc) noexcept {
    this->vertical_angle_ += inc;
  }

  float vertical_angle() const noexcept {
    return this->vertical_angle_;
  }

  void start_jump() noexcept {
    if (this->jp_state_ == vertical_motion::standing) {
      this->jp_state_ = vertical_motion::jumping;
    }
  }

  /**
   * Simulate the motion of jumping for the cam
   * Does nothing if cam is standing
   * If called when currently jumping it will increase the height of a keep_jumping,
   * until a certain height is reached then it will start to fall
   * If called when currently falling the height of the keep_jumping decreases, until ground level is reached,
   * then the jumping state will again be standing
   */
  void keep_jumping() noexcept {
    switch (this->jp_state_) {
      case vertical_motion::jumping:
        if (this->camera_position_.y >= max_jumping_level_) {
          this->jp_state_ = vertical_motion::falling;
        } else {
          this->camera_position_.y += jumping_speed_;
        }
        break;
      case vertical_motion::falling:
        if (this->camera_position_.y <= ground_level_) {
          this->jp_state_ = vertical_motion::standing;
          // Since sometimes a float isn't that precise ;)
          this->camera_position_.y = ground_level_;
        } else {
          this->camera_position_.y -= jumping_speed_;
        }
        break;
      case vertical_motion::standing:
        break;
    }
  }

private:
  coord3d camera_position_;
  float horizontal_angle_ = 0;
  float vertical_angle_ = 0;
  vertical_motion jp_state_ = vertical_motion::standing;

  static const float turn_factor_;
  static const float movement_speed_;
  static const float jumping_speed_;
  static const float ground_level_;
  static const float max_jumping_level_;
};

/**
 * Represents an item, which can be picked up
 */
class portable_object {
public:
  explicit portable_object(coord3d coord) : location_(coord) {}

  void render(context &ctx) {
    move_closer_to_camera(ctx);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor3d(1, 0, 0);
    glPushMatrix();
    glTranslatef(location_.x, location_.y, location_.z);
    this-> angle_ = this->angle_ % 360;
    glRotatef(this->angle_++, 0, 1, 0);
    glRotatef(45, 1, 0, 0);

    glutSolidCube(0.25);
    glPopMatrix();
  }

  void pick() {
    this->follow_cam_ = true;
  }

  bool is_following() const {
    return follow_cam_;
  }

  coord3d location() {
    return location_;
  }

private:
  coord3d location_;
  bool follow_cam_ = false;
  int angle_ = 0;
  static const float ball_speed_;

  void move_closer_to_camera(context &ctx) {
    if (follow_cam_) {
      location_.x += ctx.cam().x < location_.x ? -ball_speed_ : ball_speed_;
      location_.z += ctx.cam().z < location_.z ? -ball_speed_ : ball_speed_;
    }
  }
};

/**
 * Statics
 */

// These should only be available to the context class
const float context::turn_factor_ = 0.05;
const float context::movement_speed_ = 0.3;
const float context::jumping_speed_ = 0.004;
const float context::ground_level_ = 1.5;
const float context::max_jumping_level_ = context::ground_level_ + 1;

const float portable_object::ball_speed_ = 0.005;

constexpr float field_size = 5.0f;
constexpr float lookaround_speed = 0.001;
constexpr float vertical_camera_top_limit = 0.7;
constexpr float vertical_camera_bot_limit = -1;

constexpr int labyrinth_width = 11;

constexpr int escape_key = 27;


/**
 * Global variables
 */

context ctx;
int windowid;
std::vector<portable_object> portable_objects;

bool labyrinth[labyrinth_width][labyrinth_width] = {
    {false, false, false, false, false, false, false, false, false, false, false},
    {true,  true,  true,  true,  true,  true,  true,  true,  false, true,  false},
    {false, false, false, false, false, false, false, true,  false, true,  false},
    {false, true,  true,  true,  true,  true,  false, true,  false, true,  false},
    {false, true,  false, true,  false, false, false, true,  false, true,  false},
    {false, true,  false, true,  false, true,  true,  true,  false, true,  false},
    {false, true,  false, false, false, true,  false, false, false, true,  false},
    {false, true,  false, true,  true,  true,  false, true,  true,  true,  false},
    {false, true,  false, true,  false, false, false, true,  false, true,  false},
    {false, true,  true,  true,  true,  true,  true,  true,  false, true,  false},
    {false, false, false, false, false, false, false, false, false, false, false},
};

/**
 * Helper functions
 */

void position_view() {
  auto cam = ctx.cam();
  gluLookAt(cam.x, cam.y, cam.z,
            cam.x + ctx.lx(), cam.y + ctx.vertical_angle(), cam.z + ctx.lz(),
            0.0f, cam.y, 0.0f);
}

/**
 * Move the camera according to the movement direction
 * @param mv which direction to move
 */
void movement(movement_direction mv) {
  coord3d old_cam = ctx.cam();
  switch (mv) {
    case movement_direction::left:
      ctx.cam() += coord3d{ctx.z(), 0, -ctx.x()};
      break;
    case movement_direction::right:
      ctx.cam() += coord3d{-ctx.z(), 0, ctx.x()};
      break;
    case movement_direction::forward:
      ctx.cam() += coord3d{ctx.x(), 0, ctx.z()};
      break;
    case movement_direction::backward:
      ctx.cam() += coord3d{-ctx.x(), 0, -ctx.z()};
      break;
    case movement_direction::rotate_clockwise:
      ctx.inc_horizontal_angle();
      break;
    case movement_direction::rotate_counterclockwise:
      ctx.dec_horizontal_angle();
      break;
    case movement_direction::jump:
      ctx.start_jump();
      break;
  }

  // Easy hitbox
  int x_axis_pos = (int) (ctx.cam().x) / (int) field_size;
  int z_axis_pos = (int) (ctx.cam().z) / (int) field_size;
  if (!labyrinth[z_axis_pos][x_axis_pos]
      || ctx.cam().x < 0
      || ctx.cam().z < 0) {
    ctx.cam() = old_cam;
  }
}

/**
 * Callbacks
 */

void reshapeFunc(int x, int y) {
  if (y == 0 || x == 0) return;  //Nothing is visible then, so return

  glMatrixMode(GL_PROJECTION); //Set a new projection matrix
  glLoadIdentity();
  //Angle of view: 40 degrees
  //Near clipping plane distance: 0.5
  //Far clipping plane distance: 20.0

  gluPerspective(40.0, (GLdouble) x / (GLdouble) y, 0.5, 40.0);
  glViewport(0, 0, x, y);  //Use the whole window for rendering

}

void keyboard(unsigned char key, int x, int y) {
  switch (key) {
    // These are movement keys and are processed in another function
    case 'q':
    case 'e':
    case 'a':
    case 'd':
    case 'w':
    case 's':
    case static_cast<unsigned char>(movement_direction::jump):
      movement(static_cast<movement_direction>(key));
      break;
    case escape_key: // Escape key
      glutDestroyWindow(windowid);
      exit(0);
      break; // Unreachable code
    case 'f': {
      int x_axis_pos = (int) (ctx.cam().x) / (int) field_size;
      int z_axis_pos = (int) (ctx.cam().z) / (int) field_size;
      for (auto &po : portable_objects) {
        int po_x = (int) (po.location().x) / (int) field_size;
        int po_z = (int) (po.location().z) / (int) field_size;
        if (x_axis_pos == po_x && z_axis_pos == po_z) {
          po.pick();
        }
      }
      break;
    }
    default:
      break;
  }
  glutPostRedisplay();
}

void mouse_motion(int x, int y) {

  int vertical_center = glutGet(GLUT_WINDOW_HEIGHT) / 2;
  int horizontal_center = glutGet(GLUT_WINDOW_WIDTH) / 2;

  // horizontal horizontal_angle
  ctx.inc_horizontal_angle_by(static_cast<float>(x - horizontal_center) * lookaround_speed);
  // vertical horizontal_angle
  float diff = -((float) y - (float) vertical_center) * lookaround_speed;
  float new_vertical_angle = diff + ctx.vertical_angle();

  // Limit vertical camera movement
  if (new_vertical_angle < vertical_camera_top_limit
      && new_vertical_angle >= vertical_camera_bot_limit) {
    ctx.inc_vertical_angle_by(diff);
  }

  // Have to check for an area,
  // since the glutWarpPointer function will fire mouse_motion again
  const int max_mouse_center_offset = 10;
  if (x <= horizontal_center - max_mouse_center_offset
      || x >= horizontal_center + max_mouse_center_offset
      || y <= vertical_center - max_mouse_center_offset
      || y >= vertical_center + max_mouse_center_offset) {
    // keep mouse in center of the window
    glutWarpPointer(horizontal_center, vertical_center);
  }

  glutPostRedisplay();
}

void render_labyrinth() {
  // render ground
  glPushMatrix();
  glColor3d(0.67, 0.67, 0.67); // Gray
  glBegin(GL_QUADS);

  float total_labyrinth_width = labyrinth_width * field_size;

  glVertex3d(0, 0, 0);
  glVertex3d(total_labyrinth_width, 0, 0);
  glVertex3d(total_labyrinth_width, 0, total_labyrinth_width);
  glVertex3d(0, 0, total_labyrinth_width);
  glEnd();
  glPopMatrix();
  // render walls
  int z = 0;
  for (int i = 0; i < labyrinth_width; i++) {
    for (int j = 0; j < labyrinth_width; j++) {
      if (!labyrinth[i][j]) {
        glPushMatrix();

        // Without the offset the center and not the edge of the cube will be where I want it
        // It would be off about half the field size
        const float offset = field_size / 2;
        glTranslatef(static_cast<float>(j) * field_size + offset,
                     0,
                     static_cast<float>(i) * field_size + offset);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glColor3d(0.0, 0.9, 0.0); // green
        glutSolidCube(field_size);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glColor3d(0, 0, 0); // black
        glutSolidCube(field_size * 0.99);

        glPopMatrix();
      }
    }
  }

}

void render_room_floor() {
  glPushMatrix();
  glColor3d(0, 0, 0.67); // Blue
  glBegin(GL_QUADS);
  glVertex3d(0, 0.01, field_size);
  glVertex3d(field_size, 0.01, field_size);
  glVertex3d(field_size, 0.01, field_size + field_size);
  glVertex3d(0, 0.01, field_size + field_size);
  glEnd();
  glPopMatrix();

  glPushMatrix();
  glColor3d(0.67, 0.67, 0.67); // Gray
  glBegin(GL_QUADS);
  glVertex3d(0, field_size, field_size);
  glVertex3d(field_size, field_size, field_size);
  glVertex3d(field_size, field_size, field_size + field_size);
  glVertex3d(0, field_size, field_size + field_size);
  glEnd();
  glPopMatrix();
}

void render_room_walls() {
  glPushMatrix();
  // North Wall
  glColor3d(0, 0.67, 0.67); // Cyan
  glBegin(GL_QUADS);
  glVertex3d(0, 0, field_size);
  glVertex3d(field_size, 0, field_size);
  glVertex3d(field_size, field_size, field_size);
  glVertex3d(0, field_size, field_size);
  glEnd();

  // West Wall
  glColor3d(0.67, 0.67, 0); // Yellow
  glBegin(GL_QUADS);
  glVertex3d(0, 0, field_size);
  glVertex3d(0, 0, field_size + field_size);
  glVertex3d(0, field_size, field_size + field_size);
  glVertex3d(0, field_size, field_size);
  glEnd();

  // South Wall
  glColor3d(0.67, 0, 0.67); // Purple
  glBegin(GL_QUADS);
  glVertex3d(0, 0, field_size + field_size);
  glVertex3d(field_size, 0, field_size + field_size);
  glVertex3d(field_size, field_size, field_size + field_size);
  glVertex3d(0, field_size, field_size + field_size);
  glEnd();
  glPopMatrix();
}

void render_room_table() {
  glColor3d(0.545, 0.271, 0.075); // Brown
  // Base
  glPushMatrix();
  glTranslatef(0.5, 0.25, field_size + 0.5);
  glRotatef(90, 1, 0, 0);
  glutSolidCylinder(0.05, 0.6, 100, 100);
  glPopMatrix();

  // Plate
  glPushMatrix();
  glTranslatef(0.5, 0.4, field_size + 0.5);
  glRotatef(90, 1, 0, 0);
  glutSolidCylinder(0.4, 0.1, 100, 100);
  glPopMatrix();
}

void render_room_hourglass() {
  // Base
  glColor3d(0.855, 0.647, 0.125);
  glPushMatrix();
  glTranslatef(0.55, 0.4, field_size + 0.5);
  glRotatef(-90, 1, 0, 0);
  glutSolidCone(0.1, 0.2, 100, 100);
  glPopMatrix();

  // Top
  glColor3d(0.529, 0.808, 0.980);
  glPushMatrix();
  glTranslatef(0.55, 0.7, field_size + 0.5);
  glRotatef(90, 1, 0, 0);
  glutSolidCone(0.1, 0.2, 100, 100);
  glPopMatrix();
}

void render_room_balloon() {
  glPushMatrix();
  glColor3d(1, 0, 0); // Red
  glTranslatef(4, 1.5, field_size + 0.7);
  glScalef(1, 1.2, 1);
  glutSolidSphere(0.2, 100, 100);
  glPopMatrix();

  // String
  glPushMatrix();
  glColor3d(1, 1, 1); // White
  glTranslatef(4, 0.4, field_size + 0.7);
  glBegin(GL_QUAD_STRIP);
  // Wobbly line
  for (int i = 0; i < 10; i++) {
    if (i % 2 == 0) {
      glVertex3d(0, 0 + 0.1 * i, 0);
      glVertex3d(0.01, 0 + 0.1 * i, 0);
      glVertex3d(0.02, 0.1 + 0.1 * i, 0);
      glVertex3d(0.01, 0.1 + 0.1 * i, 0);
    } else {
      glVertex3d(0.01, 0 + 0.1 * i, 0);
      glVertex3d(0.02, 0 + 0.1 * i, 0);
      glVertex3d(0.01, 0.1 + 0.1 * i, 0);
      glVertex3d(0, 0.1 + 0.1 * i, 0);
    }
  }
  glEnd();
  glPopMatrix();
}

void render_room() {
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  // The room itself
  render_room_floor();
  render_room_walls();

  // Objects
  render_room_table();
  render_room_hourglass();
  render_room_balloon();

}

void render_portable_objects() {
  std::vector<portable_object> new_objects;
  for (auto &po : portable_objects) {
    po.render(ctx);
    if (!po.is_following() ||
        (po.location().x - ctx.cam().x >= 0.1
         || po.location().z - ctx.cam().z >= 0.1)) {
      new_objects.push_back(po);
    } else {
      std::cout << "Despawned object" << std::endl;
    }
  }
  portable_objects = new_objects;
}

void renderScene() {
  glMatrixMode(GL_MODELVIEW);
  glClear(GL_DEPTH_BUFFER_BIT);
  glClear(GL_COLOR_BUFFER_BIT);
  // For overlapping objects
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);
  glDepthRange(0.0f, 1.0f);
  glClearDepth(1.0f);

  ctx.keep_jumping();

  glLoadIdentity();

  position_view();

  render_room();
  render_labyrinth();
  render_portable_objects();
  glutSwapBuffers();
}

void init_portable_objects() {
  for (int i = 0; i < labyrinth_width; i++) {
    for (int j = 0; j < labyrinth_width; j++) {
      if (rand() % 5 == 0 && labyrinth[j, i]) {
        portable_objects.emplace_back(coord3d{(float) j * field_size + field_size / 2,
                                              1,
                                              (float) i * field_size + field_size / 2});
      }
    }
  }
}

int main(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  srand(time(nullptr));
  glutInitWindowPosition(500, 500);
  glutInitWindowSize(800, 600);
  windowid = glutCreateWindow("Labyrinth");
  glutSetCursor(GLUT_CURSOR_NONE);

  ctx.cam().x = field_size / 2;
  ctx.cam().z = field_size + field_size / 2;
  init_portable_objects();

  glutReshapeFunc(reshapeFunc);
  glutPassiveMotionFunc(mouse_motion);
  glutKeyboardFunc(keyboard);

  glutDisplayFunc(renderScene);

  glutIdleFunc(renderScene);

  glutMainLoop();
  return EXIT_SUCCESS;
}
