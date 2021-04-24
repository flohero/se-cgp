/**
 * Aufwand: 14h
 *
 * Controls:
 * a: toggle ambient light
 * s: toggle point light left
 * d: toggle point light right
 * f: toggle fog
 * g: toggle spotlight
 * q: rotate spotlight left
 * e: rotate spotlight right
 * x: increase ambient light
 * y: dim ambient light
 */

#include "GL/glew.h"
#include "GL/freeglut.h"
#include <cmath>
#include <utility>
#include <vector>
#include <memory>

constexpr float room_level = -2;
constexpr float room_size = 10;

constexpr float pink[] = {1.000f, 0.753f, 0.796f, 1.0f};
constexpr float gray[] = {.67f, .67f, .67f, 1.0f};
constexpr float green[] = {0.0f, 0.9f, 0.4f};
constexpr float red[] = {1, 0.0f, 0.0f, 1};
constexpr float blue[] = {0.0f, 0.0f, 1.0f, 1.0f};
constexpr float purple[] = {0.5f, 0.0f, 0.5f, 1};
constexpr float red_purple[] = {1, 0.0f, 0.5f, 1};
constexpr float white[] = {1.0f, 1.0f, 1.0f, 1.0f};

constexpr float zero[] = {0, 0, 0};
constexpr float half[] = {0.5, 0.5, 0.5};
constexpr float one[] = {1, 1, 1};
constexpr float shininess_none = 0;
constexpr float shininess_low = 0.1;
constexpr float shininess_mid = 0.5;
constexpr float shininess_high = 1;

constexpr float vertical_camera_top_limit = 0.7;
constexpr float vertical_camera_bot_limit = -1;
constexpr float lookaround_speed = 0.0001;
constexpr float max_angel = 0.5f;

constexpr float spot_light_step = 0.05f;
constexpr float light_intensity_step = 0.01f;

/* Classes */

/**
 * Representation of a 3d Coordinate
 */
struct position {
  float x;
  float y;
  float z;

  explicit position(float x = 0, float y = 0, float z = 0) : x{x}, y{y}, z{z} {}

  friend bool operator!=(const position &left, const position &right) {
    return left.x != right.x || left.y != right.y || left.z != right.z;
  }

  /**
    * Operator to increment the x,y and z values.
    * @param other, the coordinate by which this one should be incremented
    */
  position &operator+=(const position &other) {
    this->x += other.x;
    this->y += other.y;
    this->z += other.z;
    return *this;
  }
};

/**
 * Settings for lights and fog
 */
struct light_settings {
  float spot_light_angel = 0.0f;
  bool ambient_light_enabled = true;
  bool point_light_left_enabled = true;
  bool point_light_right_enabled = true;
  bool spotlight_enabled = true;
  bool fog_enabled = false;
  float ambient_light_intensity = default_light_intensity_;

private:
  constexpr static const float default_light_intensity_ = 0.5f;
};

/**
 * A game object represents a figure which can be rendered
 */
class game_object {
public:
  explicit game_object(position pos) : pos_(pos) {}

  virtual void render() = 0;

protected:
  position pos_;

  virtual void material() {
    glMaterialfv(GL_FRONT, GL_AMBIENT, half);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, one);
    glMaterialfv(GL_FRONT, GL_SPECULAR, one);
    glMaterialfv(GL_FRONT, GL_EMISSION, zero);
    glMaterialf(GL_FRONT, GL_SHININESS, 0);
  }
};

/**
 * The disco room
 */
class disco_room : public game_object {
public:
  disco_room() : game_object(position{room_size / 2, room_level, -room_size / 5}) {}

  void render() override {
    glPushMatrix();
    material();
    // floor
    glTranslatef(0, pos_.y, -room_size / 2);
    glScaled(room_size, 0.01, room_size);
    // Have to use a sphere, otherwise the spotlight won't work
    glutSolidSphere(1, 100, 100);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(pos_.x, pos_.y, pos_.z);

    wall_material();
    // wall north
    glBegin(GL_QUADS);
    glVertex3d(0, 0, -room_size);
    glVertex3d(0, height_, -room_size);
    glVertex3d(-room_size, height_, -room_size);
    glVertex3d(-room_size, 0, -room_size);
    glEnd();

    // wall east
    glBegin(GL_QUADS);
    glVertex3d(0, 0, room_size);
    glVertex3d(0, height_, room_size);
    glVertex3d(0, height_, -room_size);
    glVertex3d(0, 0, -room_size);
    glEnd();

    // wall west
    glBegin(GL_QUADS);
    glVertex3d(-room_size, 0, room_size);
    glVertex3d(-room_size, height_, room_size);
    glVertex3d(-room_size, height_, -room_size);
    glVertex3d(-room_size, 0, -room_size);
    glEnd();
    glPopMatrix();
  }

private:
  constexpr static const float height_ = 5;

  /**
   * Dedicated material for the walls
   */
  static void wall_material() {
    GLfloat emission[] = {0.0f, 0.0f, 0.0f, 0.0f};
    glMaterialfv(GL_FRONT, GL_AMBIENT, gray);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, gray);
    glMaterialfv(GL_FRONT, GL_SPECULAR, gray);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess_high);
    glMaterialfv(GL_FRONT, GL_EMISSION, emission);
  }

protected:
  void material() override {
    glMaterialfv(GL_FRONT, GL_AMBIENT, purple);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, purple);
    glMaterialfv(GL_FRONT, GL_SPECULAR, purple);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess_mid);
    glMaterialfv(GL_FRONT, GL_EMISSION, zero);
  }
};

/**
 * A physical representation of the spotlight
 */
class light_cone : public game_object {
public:
  explicit light_cone(std::shared_ptr<light_settings> sett) :
      game_object(position(0, 2, -room_size / 2)),
      sett_(std::move(sett)) {}

  void render() override {
    glPushMatrix();
    material();
    glTranslatef(pos_.x, pos_.y, pos_.z);
    glRotatef(-90, 1, 0, 0);
    glRotatef(sett_->spot_light_angel * start_angel, 0, 1, 0);
    glutSolidCone(cone_base_, cone_height_, 100, 100);
    glPopMatrix();
  }

protected:
  void material() override {
    glMaterialfv(GL_FRONT, GL_AMBIENT, green);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, green);
    glMaterialfv(GL_FRONT, GL_SPECULAR, half);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess_high);
    glMaterialfv(GL_FRONT, GL_EMISSION, green);
  }

private:
  std::shared_ptr<light_settings> sett_;
  constexpr static const float cone_base_ = 0.1f;
  constexpr static const float cone_height_ = 0.3f;
  constexpr static const float start_angel = -60.0f;
};

/**
 * a rotating disco ball
 */
class disco_ball : public game_object {
public:
  explicit disco_ball(float x, const float *color) : game_object(position(x, ball_height_, -room_size / 2)), color_(color) {}

  void render() override {
    glPushMatrix();
    glShadeModel(GL_FLAT);
    material();
    glTranslatef(pos_.x, pos_.y, pos_.z);
    angel = (angel + 1) % 360;
    glRotatef((float) angel, 0, 1, 0);
    glutSolidSphere(ball_radius_, 10, 10);
    glShadeModel(GL_SMOOTH);
    glPopMatrix();
  }

protected:
  void material() override {
    glMaterialfv(GL_FRONT, GL_AMBIENT, color_);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, color_);
    glMaterialfv(GL_FRONT, GL_SPECULAR, half);
    glMaterialfv(GL_FRONT, GL_EMISSION, zero);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess_high);
  }

private:
  const float *color_;
  int angel = 0;
  constexpr static const float ball_height_ = 1.5f;
  constexpr static const float ball_radius_ = 0.3f;
};

/**
 * a floor and a dj console
 */
class dj_booth : public game_object {
public:
  explicit dj_booth(std::shared_ptr<light_settings> sett) :
      game_object(position{size_ / 2, booth_level_, size_ / 2}),
      sett_(std::move(sett)) {}

  void render() override {
    glPushMatrix();
    material();
    // floor
    glTranslatef(pos_.x, pos_.y, pos_.z);
    glBegin(GL_QUADS);
    glVertex3d(0, 0, 0);
    glVertex3d(-size_, 0, 0);
    glVertex3d(-size_, 0, -size_);
    glVertex3d(0, 0, -size_);
    glEnd();

    // console for lights etc
    glBegin(GL_QUADS);
    glVertex3d(0, 0, 0);
    glVertex3d(0, size_, 0);
    glVertex3d(0, size_, -size_);
    glVertex3d(0, 0, -size_);
    glEnd();
    glPopMatrix();

    // ambient light
    glPushMatrix();
    text_material();
    glTranslatef(pos_.x - 0.1f, pos_.y + 2, -1);
    glScalef(text_scale_, text_scale_, text_scale_); // Have to scale down, since text is huge
    glRotatef(-90, 0, 1, 0);
    glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, 'a');
    glPopMatrix();

    glPushMatrix();
    glMaterialfv(GL_FRONT, GL_EMISSION, sett_->ambient_light_enabled ? green : red);
    glTranslatef(pos_.x, pos_.y + 2.05f, -0.7);
    glutSolidCube(0.1);
    glPopMatrix();

    // point light left
    glPushMatrix();
    text_material();
    glTranslatef(pos_.x - 0.1f, pos_.y + 1.7f, -1);
    glScalef(text_scale_, text_scale_, text_scale_);
    glRotatef(-90, 0, 1, 0);
    glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, 's');
    glPopMatrix();

    glPushMatrix();
    glMaterialfv(GL_FRONT, GL_EMISSION, sett_->point_light_left_enabled ? green : red);
    glTranslatef(pos_.x, pos_.y + 1.75f, -0.7);
    glutSolidCube(0.1);
    glPopMatrix();

    // point light right
    glPushMatrix();
    text_material();
    glTranslatef(pos_.x - 0.1f, pos_.y + 1.4f, -1);
    glScalef(text_scale_, text_scale_, text_scale_);
    glRotatef(-90, 0, 1, 0);
    glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, 'd');
    glPopMatrix();

    glPushMatrix();
    glMaterialfv(GL_FRONT, GL_EMISSION, sett_->point_light_right_enabled ? green : red);
    glTranslatef(pos_.x, pos_.y + 1.45f, -0.7);
    glutSolidCube(0.1);
    glPopMatrix();

    // fog
    glPushMatrix();
    text_material();
    glTranslatef(pos_.x - 0.1f, pos_.y + 1.1f, -1);
    glScalef(text_scale_, text_scale_, text_scale_);
    glRotatef(-90, 0, 1, 0);
    glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, 'f');
    glPopMatrix();

    glPushMatrix();
    glMaterialfv(GL_FRONT, GL_EMISSION, sett_->fog_enabled ? green : red);
    glTranslatef(pos_.x, pos_.y + 1.15f, -0.7);
    glutSolidCube(0.1);
    glPopMatrix();

    // spotlight
    glPushMatrix();
    text_material();
    glTranslatef(pos_.x - 0.1f, pos_.y + 0.8f, -1);
    glScalef(text_scale_, text_scale_, text_scale_);
    glRotatef(-90, 0, 1, 0);
    glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, 'g');
    glPopMatrix();

    glPushMatrix();
    glMaterialfv(GL_FRONT, GL_EMISSION, sett_->spotlight_enabled ? green : red);
    glTranslatef(pos_.x, pos_.y + 0.85f, -0.7);
    glutSolidCube(0.1);
    glPopMatrix();

    // dim ambient light
    glPushMatrix();
    text_material();
    glTranslatef(pos_.x - 0.1f, pos_.y + 2, -0.4);
    glScalef(text_scale_, text_scale_, text_scale_);
    glRotatef(-90, 0, 1, 0);
    glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, 'y');
    glPopMatrix();

    glPushMatrix();
    text_material();
    glTranslatef(pos_.x - 0.1f, pos_.y + 2, 0.9f);
    glScalef(text_scale_, text_scale_, text_scale_);
    glRotatef(-90, 0, 1, 0);
    glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, 'x');
    glPopMatrix();

    glPushMatrix();
    glMaterialfv(GL_FRONT, GL_EMISSION, blue);
    glTranslatef(pos_.x, pos_.y + 2.05f, -0.1f + sett_->ambient_light_intensity);
    glutSolidCube(0.1);
    glPopMatrix();

    // rotate spotlight
    glPushMatrix();
    text_material();
    glTranslatef(pos_.x - 0.1f, pos_.y + 1.7f, -0.4);
    glScalef(text_scale_, text_scale_, text_scale_);
    glRotatef(-90, 0, 1, 0);
    glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, 'q');
    glPopMatrix();

    glPushMatrix();
    text_material();
    glTranslatef(pos_.x - 0.1f, pos_.y + 1.7f, 0.9f);
    glScalef(text_scale_, text_scale_, text_scale_);
    glRotatef(-90, 0, 1, 0);
    glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, 'e');
    glPopMatrix();

    glPushMatrix();
    glMaterialfv(GL_FRONT, GL_EMISSION, blue);
    glTranslatef(pos_.x, pos_.y + 1.75f, 0.4f + sett_->spot_light_angel);
    glutSolidCube(0.1);
    glPopMatrix();
  }

protected:
  void material() override {
    glMaterialfv(GL_FRONT, GL_AMBIENT, half);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, half);
    glMaterialfv(GL_FRONT, GL_SPECULAR, one);
    glMaterialfv(GL_FRONT, GL_EMISSION, half);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess_high);
  }

  static void text_material() {
    glMaterialfv(GL_FRONT, GL_AMBIENT, zero);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, zero);
    glMaterialfv(GL_FRONT, GL_SPECULAR, zero);
    glMaterialfv(GL_FRONT, GL_EMISSION, zero);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess_none);
  }

private:
  std::shared_ptr<light_settings> sett_;
  constexpr static const float size_ = 4;
  constexpr static const float booth_level_ = -1;
  constexpr static float text_scale_ = 0.002f;

};

/**
 * How the guests should move
 */
enum class movement {
  jump = 0,
  left_right = 1,
  front_back = 2,
  jump_and_left_right = 3,
};

class guest : public game_object {
public:
  guest(float x, float z) : game_object(position(x, 0, z)) {
    movement_ = (movement) (rand() % 4);
  }

  void render() override {
    move();
    glPushMatrix();
    glMaterialfv(GL_FRONT, GL_EMISSION, zero);
    glMaterialfv(GL_FRONT, GL_AMBIENT, pink);
    glMaterialfv(GL_FRONT, GL_SPECULAR, one);
    glMaterialf(GL_FRONT, GL_SHININESS, shininess_low);
    glTranslatef(pos_.x, pos_.y, pos_.z);
    glutSolidSphere(0.2, 30, 30);
    glPopMatrix();
    glPushMatrix();
    material();

    glTranslatef(pos_.x, pos_.y - 1, pos_.z);
    glRotated(-90, 1, 0, 0);
    glutSolidCone(0.3, 1, 30, 30);
    glPopMatrix();
    glMaterialfv(GL_FRONT, GL_EMISSION, zero);
  }

private:
  bool up_ = true;
  bool left_ = true;
  float counter = 0;
  movement movement_;
  float movement_speed_ = 0.001f * ((rand() % 5) + 1);
  int material_ = rand() % 5;


  void move() {
    switch (movement_) {
      case movement::left_right:
        left_right();
        break;
      case movement::front_back:
        front_back();
        break;
      case movement::jump_and_left_right:
        left_right();
      case movement::jump:
        jump();
        break;
    }
  }

  void left_right() {
    if (left_) {
      pos_.x -= movement_speed_;
      counter++;
      left_ = counter <= 200;
    } else {
      pos_.x += movement_speed_;
      counter--;
      left_ = counter < -200;
    }
  }

  void front_back() {
    if (left_) {
      pos_.z -= movement_speed_;
      counter++;
      left_ = counter <= 700;
    } else {
      pos_.z += movement_speed_;
      counter--;
      left_ = counter < -700;
    }
  }

  void jump() {
    if (up_) {
      pos_.y += movement_speed_;
      if (pos_.y > 0.5) {
        up_ = false;
      }
    } else {
      pos_.y -= movement_speed_;
      if (pos_.y <= 0) {
        up_ = true;
      }
    }
  }

protected:

  /**
   * Different Materials for the guests
   */
  void material() override {
    glMaterialfv(GL_FRONT, GL_EMISSION, zero);
    switch (material_) {
      case 0:
        glMaterialfv(GL_FRONT, GL_AMBIENT, pink);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, pink);
        glMaterialfv(GL_FRONT, GL_SPECULAR, one);
        glMaterialf(GL_FRONT, GL_SHININESS, shininess_mid);
        break;
      case 1:
        glMaterialfv(GL_FRONT, GL_AMBIENT, green);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, green);
        glMaterialfv(GL_FRONT, GL_SPECULAR, half);
        glMaterialf(GL_FRONT, GL_SHININESS, shininess_high);
        glMaterialfv(GL_FRONT, GL_EMISSION, green);
        break;
      case 2:
        glMaterialfv(GL_FRONT, GL_AMBIENT, red);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, red);
        glMaterialfv(GL_FRONT, GL_SPECULAR, half);
        glMaterialf(GL_FRONT, GL_SHININESS, shininess_high);
        break;
      case 3:
        glMaterialfv(GL_FRONT, GL_AMBIENT, purple);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, purple);
        glMaterialfv(GL_FRONT, GL_SPECULAR, half);
        glMaterialf(GL_FRONT, GL_SHININESS, shininess_mid);
        break;
      case 4:
        glMaterialfv(GL_FRONT, GL_AMBIENT, red_purple);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, red_purple);
        glMaterialfv(GL_FRONT, GL_SPECULAR, half);
        glMaterialfv(GL_FRONT, GL_EMISSION, zero);
        glMaterialf(GL_FRONT, GL_SHININESS, shininess_low);
        break;
    }
  }
};

using game_object_ptr = std::shared_ptr<game_object>;

/**
 * Represents the current state of the game/disco
 * Contains all game objects and manages the light
 */
class game_state {
public:
  game_state() = default;

  explicit game_state(std::shared_ptr<light_settings> sett) : sett_(std::move(sett)) {};

  int windowid() const {
    return this->windowid_;
  }

  void windowid(int id) {
    this->windowid_ = id;
  }

  std::shared_ptr<light_settings> setting() {
    return sett_;
  }

  void inc_horizontal_angle_by(const float inc) noexcept {
    this->horizontal_angle_ += inc;
  }

  void inc_vertical_angle_by(const float inc) noexcept {
    this->vertical_angle_ += inc;
  }

  float vertical_angle() const noexcept {
    return this->vertical_angle_;
  }

  float lx() const noexcept {
    return std::sin(this->horizontal_angle_);
  }

  float lz() const noexcept {
    return -std::cos(this->horizontal_angle_);
  }

  void position_view() const {
    gluLookAt(camera_position_.x, camera_position_.y, camera_position_.z,
              camera_position_.x + lx(), camera_position_.y + vertical_angle(), camera_position_.z + lz(),
              0.0f, camera_position_.y, 0.0f);
  }

  void add_game_object(const game_object_ptr &go) {
    game_objects.push_back(go);
  }

  /**
   * Render all game objects
   */
  void render() {
    for (const auto &go: game_objects) {
      go->render();
    }
  }

  void render_lights() {
    ambient_light();
    point_light_left();
    point_light_right();
    spotlight();
    fog();
  }

  void toggle_ambient_light() {
    sett_->ambient_light_enabled = !sett_->ambient_light_enabled;
    ambient_light();
  }

  void toggle_point_light_left() {
    sett_->point_light_left_enabled = !sett_->point_light_left_enabled;
    point_light_left();
  }

  void toggle_point_light_right() {
    sett_->point_light_right_enabled = !sett_->point_light_right_enabled;
    point_light_right();
  }

  void toggle_spotlight() {
    sett_->spotlight_enabled = !sett_->spotlight_enabled;
    spotlight();
  }

  void toggle_fog() {
    sett_->fog_enabled = !sett_->fog_enabled;
    fog();
  }

private:
// GLUT Window ID
  int windowid_ = -1;
  float horizontal_angle_ = 0;
  float vertical_angle_ = 0;
  position camera_position_{0, 0.7, 0};
  std::vector<game_object_ptr> game_objects;

  std::shared_ptr<light_settings> sett_;

  constexpr static const float fog_density = 0.2;

  void ambient_light() const {
    float intensity = sett_->ambient_light_intensity;
    if (!sett_->ambient_light_enabled) {
      intensity = 0.0f;
    }
    GLfloat lmodel_ambient[] = {intensity, intensity, intensity, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
  }

  void point_light_left() const {
    GLfloat light_pos[] = {-room_size / 2 + 1, 3, -room_size / 2, 1.0f}; // Left side of the room
    glLightfv(GL_LIGHT0, GL_DIFFUSE, red);
    glLightfv(GL_LIGHT0, GL_SPECULAR, red);
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.15f);
    if (sett_->point_light_left_enabled) {
      glEnable(GL_LIGHT0);
    } else {
      glDisable(GL_LIGHT0);
    }
  }

  void point_light_right() const {
    GLfloat light_pos[] = {room_size / 2 - 1, 3, -room_size / 2, 1.0f}; // Right side of the room
    glLightfv(GL_LIGHT1, GL_DIFFUSE, blue);
    glLightfv(GL_LIGHT1, GL_SPECULAR, blue);
    glLightfv(GL_LIGHT1, GL_POSITION, light_pos);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.15f);
    if (sett_->point_light_right_enabled) {
      glEnable(GL_LIGHT1);
    } else {
      glDisable(GL_LIGHT1);
    }
  }

  void spotlight() {
    GLfloat light_pos[] = {0.0f, 4.0f, -room_size / 2, 1.0f};

    GLfloat spot_direction[] = {sett_->spot_light_angel, -1.0f, 0.0f};

    glLightfv(GL_LIGHT2, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, spot_direction);
    glLightfv(GL_LIGHT2, GL_SPECULAR, white);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, white);
    glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 20.0f);
    glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 0.03f);
    if (sett_->spotlight_enabled) {
      glEnable(GL_LIGHT2);
    } else {
      glDisable(GL_LIGHT2);
    }
  }

  void fog() const {
    glFogi(GL_FOG_MODE, GL_EXP);
    glFogfv(GL_FOG_COLOR, gray);
    glFogf(GL_FOG_DENSITY, fog_density);
    glHint(GL_FOG_HINT, GL_DONT_CARE);
    glFogf(GL_FOG_START, 2.0f);
    glFogf(GL_FOG_END, 4.0f);
    if (sett_->fog_enabled) {
      glEnable(GL_FOG);
    } else {
      glDisable(GL_FOG);
    }
  }
};


/* Game State */
game_state state;

/*-[Keyboard Callback]-------------------------------------------------------*/
void keyboard(unsigned char key, int x, int y) {
  switch (key) {
    case 'a':
      state.toggle_ambient_light();
      break;
    case 's':
      state.toggle_point_light_left();
      break;
    case 'd':
      state.toggle_point_light_right();
      break;
    case 'f':
      state.toggle_fog();
      break;
    case 'g':
      state.toggle_spotlight();
      break;
    case 'q': {
      auto sett = state.setting();
      if (sett->spot_light_angel > -max_angel)
        sett->spot_light_angel -= spot_light_step;
      break;
    }
    case 'e': {
      auto sett = state.setting();
      if (sett->spot_light_angel < max_angel)
        sett->spot_light_angel += spot_light_step;
      break;
    }
    case 'x': {
      auto sett = state.setting();
      if (sett->ambient_light_intensity < 1)
        sett->ambient_light_intensity += light_intensity_step;
      break;
    }
    case 'y': {
      auto sett = state.setting();
      if (sett->ambient_light_intensity > 0)
        sett->ambient_light_intensity -= light_intensity_step;
      break;
    }
    case 27: // Escape key
      glutDestroyWindow(state.windowid());
      exit(0);
      break;
    default:
      break;
  }
  glutPostRedisplay();
}

void mouse_motion(int x, int y) {

  int vertical_center = glutGet(GLUT_WINDOW_HEIGHT) / 2;
  int horizontal_center = glutGet(GLUT_WINDOW_WIDTH) / 2;

  // horizontal horizontal_angle
  state.inc_horizontal_angle_by(static_cast<float>(x - horizontal_center) * lookaround_speed);
  // vertical horizontal_angle
  float diff = -((float) y - (float) vertical_center) * lookaround_speed;
  float new_vertical_angle = diff + state.vertical_angle();

  // Limit vertical camera movement
  if (new_vertical_angle < vertical_camera_top_limit
      && new_vertical_angle >= vertical_camera_bot_limit) {
    state.inc_vertical_angle_by(diff);
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

/*-[Reshape Callback]--------------------------------------------------------*/
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

void render_scene() {
  glMatrixMode(GL_MODELVIEW);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // For overlapping objects
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);
  glDepthRange(0.0f, 1.0f);
  glClearDepth(1.0f);

  glClearColor(0.0, 0.0, 0.0, 0.0); // Original Black

  glLoadIdentity();
  state.position_view();
  state.render_lights();
  state.render();
  glutSwapBuffers();
}

/**
 * Fill the disco with guests and equipment
 */
void init_disco() {
  state.add_game_object(std::make_shared<disco_room>());
  state.add_game_object(std::make_shared<dj_booth>(state.setting()));
  state.add_game_object(std::make_shared<light_cone>(state.setting()));
  state.add_game_object(std::make_shared<disco_ball>(-room_size / 2 + 1, white));
  state.add_game_object(std::make_shared<disco_ball>(room_size / 2 - 1, white));
  for (int i = 0; i < 5; i++) {
    state.add_game_object(std::make_shared<guest>(
        (float) i - room_size / 4,
        -room_size / 2 + (i % 2 == 0 ? 1.0 : -1.0)
    ));
  }
}

/**
 * Initialize lights
 */
void init_light_sources() {
  state.render_lights();
  glEnable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);
}

int main(int argc, char **argv) {
  srand(time(nullptr));

  auto sett = std::make_shared<light_settings>();
  state = game_state(sett);

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowPosition(500, 500); //determines the initial position of the window
  glutInitWindowSize(800, 600);    //determines the size of the window

  state.windowid(glutCreateWindow("Disco"));

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glutSetCursor(GLUT_CURSOR_NONE);
  init_light_sources();

  init_disco();

  // register callbacks
  glutKeyboardFunc(keyboard);
  glutPassiveMotionFunc(mouse_motion);

  glutDisplayFunc(render_scene);
  glutReshapeFunc(reshapeFunc);
  glutIdleFunc(render_scene);

  glutMainLoop(); // start the main loop of GLUT
  return 0;
}
