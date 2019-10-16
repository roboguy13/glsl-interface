#include <GL/glew.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_keycode.h>


#if __APPLE__
#       include <OpenGL/GL.h>
#       include <OpenGL/GLu.h>
#else
#       include <GL/GL.h>
#       include <GL/GLu.h>
#endif

#include <string>
#include <iostream>
#include <fstream>

using std::cerr;
using std::endl;
using std::string;
using std::ifstream;

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;

const int SHIFT_PIXELS = 4;

const float SHIFT_AMOUNT = 30;
const float ZOOM_MULT = 0.9;

float shiftXBy(float zoom, float x, float shift) {
  return shift + ((x * SHIFT_PIXELS) * zoom / SCREEN_WIDTH);
}

float shiftYBy(float zoom, float y, float shift) {
  return shift + ((y * SHIFT_PIXELS) * zoom / SCREEN_HEIGHT);
}

bool sdlInit(SDL_Window** window, SDL_GLContext* context) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    cerr << "Error: SDL_Init" << endl;
    return false;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  *window = SDL_CreateWindow("Burning Ship", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

  if (!*window) {
    cerr << "Error: SDL_CreateWindow" << endl;
    return false;
  }

  *context = SDL_GL_CreateContext(*window);

  if (!context) {
    cerr << "Error: SDL_CreateContext" << endl;
    return false;
  }

  return true;
}

bool compileShader(string type, GLuint shader, const char* str) {
  glShaderSource(shader, 1, &str, NULL);
  glCompileShader(shader);

  GLint compileStatus;

  glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

  if (!compileStatus) {
    cerr << "Compilation error in " << type << " shader:" << endl;
    int length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    char* log = new char[length];
    glGetShaderInfoLog(shader, length, NULL, log);
    cerr << log << endl;
    return false;
  }
  return true;
}

bool glInit(string fsh, string vsh, GLint* programID, GLuint* vbo, GLuint* ibo) {
  *programID = glCreateProgram();

  ifstream fshStream(fsh);
  if (!fshStream) {
    cerr << "Cannot load fragment shader: " << fsh << endl;
    return false;
  }

  ifstream vshStream(vsh);
  if (!vshStream) {
    cerr << "Cannot load vertex shader: " << vsh << endl;
    return false;
  }

  std::string fshStr((std::istreambuf_iterator<char>(fshStream)), 
    std::istreambuf_iterator<char>());

  std::string vshStr((std::istreambuf_iterator<char>(vshStream)), 
    std::istreambuf_iterator<char>());


  fshStream.close();
  vshStream.close();

  const char* fshCStr = fshStr.c_str();
  const char* vshCStr = vshStr.c_str();

  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

  if (!compileShader("vertex", vertexShader, vshCStr)) return false;

  glAttachShader(*programID, vertexShader);

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

  if (!compileShader("fragment", fragmentShader, fshCStr)) return false;

  glAttachShader(*programID, fragmentShader);

  return true;
}



int main(int argc, char* argv[]) {

  SDL_Window* window;
  SDL_GLContext context;

  if (!sdlInit(&window, &context)) {
    return 1;
  }

  std::cout << "GL version: " << glGetString(GL_VERSION) << endl;

  GLenum glewError = glewInit();
  if (glewError != GLEW_OK) {
    cerr << "GLEW error: " << glewGetErrorString(glewError) << endl;
    return 1;
  }

  GLint programID;
  GLuint vbo, ibo;
  GLuint vao;

  glInit("../glsl/BurningShip_fsh.glsl", "../glsl/BurningShip_vsh.glsl", &programID, &vbo, &ibo);

  glLinkProgram(programID);
  glUseProgram(programID);


  GLint whichFractalLoc = glGetUniformLocation(programID, "whichFractal");
  GLint zoomLoc = glGetUniformLocation(programID, "zoom");
  GLint currShiftLoc = glGetUniformLocation(programID, "currShift");
  GLint oneLoc = glGetUniformLocation(programID, "one");

  int whichFractal = 0;
  float zoom = 1;
  float currShift[2] = { 0, 0 };

  glUniform1f(whichFractalLoc, (float)whichFractal);
  glUniform1f(zoomLoc, zoom);
  glUniform2f(currShiftLoc, currShift[0], currShift[1]);
  glUniform1f(oneLoc, 1);


  glClearColor(0.f, 0.f, 0.f, 1.f);

  GLfloat vertexData[] =
  { -1, 1,
    1, 1,
    -1, -1,
    -1, -1,
    1, 1,
    1, -1
  };

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);



  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, 2 * 6 * sizeof(GLfloat), vertexData, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  glBindAttribLocation(programID, 0, "position");

  GLint linkStatus;
  glGetProgramiv(programID, GL_LINK_STATUS, &linkStatus);

  if (!linkStatus) {
    cerr << "Error linking GL program" << endl;
    return 1;
  }

  GLint vertexPos2DLocation = glGetAttribLocation(programID, "LVertexPos2D");

  bool done = false;
  SDL_Event event;

  while (!done) {
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        done = true;
        break;
      } else if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
          case SDLK_w:
            currShift[1] = shiftYBy(zoom, -SHIFT_AMOUNT, currShift[1]);
            break;
          case SDLK_s:
            currShift[1] = shiftYBy(zoom,  SHIFT_AMOUNT, currShift[1]);
            break;
          case SDLK_a:
            currShift[0] = shiftXBy(zoom, -SHIFT_AMOUNT, currShift[0]);
            break;
          case SDLK_d:
            currShift[0] = shiftXBy(zoom,  SHIFT_AMOUNT, currShift[0]);
            break;
          case SDLK_z:
            zoom *= ZOOM_MULT;
            break;
          case SDLK_x:
            zoom /= ZOOM_MULT;
            break;
          case SDLK_t:
            whichFractal = !whichFractal;
            break;
        }
      }
    }

    glUniform1f(whichFractalLoc, (float)whichFractal);
    glUniform1f(zoomLoc, zoom);
    glUniform2f(currShiftLoc, currShift[0], currShift[1]);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 6);
  
    SDL_GL_SwapWindow(window);
  }

  glDisableVertexAttribArray(vertexPos2DLocation);
  glUseProgram(NULL);

  return 0;
}

