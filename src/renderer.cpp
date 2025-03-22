#include "renderer.hpp"
#include "texture_manager.hpp"
#include <iostream>
#include <math.h>

// Initialize static members
EGLDisplay Renderer::display = EGL_NO_DISPLAY;
EGLContext Renderer::context = EGL_NO_CONTEXT;
EGLSurface Renderer::surface = EGL_NO_SURFACE;
int Renderer::viewportWidth = 480;
int Renderer::viewportHeight = 272;

Renderer::Renderer() {
    // Initialize default values
    initialized = (display != EGL_NO_DISPLAY);
}

Renderer::~Renderer() {
    // Instance destruction doesn't terminate graphics
    // That must be done explicitly with terminateGraphics()
}

// Custom perspective projection function for OpenGL ES 1.0
void Renderer::gluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar) {
    GLfloat frustumHeight = tanf(fovy * M_PI / 360.0f) * zNear;
    GLfloat frustumWidth = frustumHeight * aspect;
    
    glFrustumf(-frustumWidth, frustumWidth, -frustumHeight, frustumHeight, zNear, zFar);
}

bool Renderer::initializeGraphics(int width, int height) {
    // Save viewport dimensions
    viewportWidth = width;
    viewportHeight = height;
    
    // Setup EGL
    EGLint majorVersion, minorVersion;
    EGLConfig config;
    EGLint numConfigs;
    
    EGLint attribList[] = {
        EGL_RED_SIZE,   8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE,  8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_NONE
    };
    
    // Get display
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        std::cerr << "Failed to get EGL display" << std::endl;
        return false;
    }
    
    // Initialize
    if (!eglInitialize(display, &majorVersion, &minorVersion)) {
        std::cerr << "Failed to initialize EGL" << std::endl;
        return false;
    }
    
    // Get config
    if (!eglChooseConfig(display, attribList, &config, 1, &numConfigs)) {
        std::cerr << "Failed to choose EGL config" << std::endl;
        return false;
    }
    
    // Create surface
    surface = eglCreateWindowSurface(display, config, 0, NULL);
    if (surface == EGL_NO_SURFACE) {
        std::cerr << "Failed to create EGL surface" << std::endl;
        return false;
    }
    
    // Create context
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
    if (context == EGL_NO_CONTEXT) {
        std::cerr << "Failed to create EGL context" << std::endl;
        return false;
    }
    
    // Make current
    if (!eglMakeCurrent(display, surface, surface, context)) {
        std::cerr << "Failed to make EGL context current" << std::endl;
        return false;
    }
    
    // Setup viewport
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    // Disable culling by default for maximum compatibility
    glDisable(GL_CULL_FACE);
    
    // Enable texturing
    glEnable(GL_TEXTURE_2D);
    
    return true;
}

void Renderer::terminateGraphics() {
    // Terminate EGL
    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        
        if (context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, context);
            context = EGL_NO_CONTEXT;
        }
        
        if (surface != EGL_NO_SURFACE) {
            eglDestroySurface(display, surface);
            surface = EGL_NO_SURFACE;
        }
        
        eglTerminate(display);
        display = EGL_NO_DISPLAY;
    }
}

void Renderer::swapBuffers() {
    if (display != EGL_NO_DISPLAY && surface != EGL_NO_SURFACE) {
        eglSwapBuffers(display, surface);
    }
}

GLuint Renderer::loadTexture(const ResourceLocation& location) {
    // Delegate to the TextureManager's static method
    return TextureManager::loadTexture(location);
}

GLuint Renderer::loadTexture(const std::string& texturePath) {
    // Delegate to the TextureManager's static method
    return TextureManager::getTexture(texturePath);
}

void Renderer::setClearColor(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
}

void Renderer::clearBuffers() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::beginFrame() {
    clearBuffers();
    glLoadIdentity();
    
    // Apply camera transformation
    glRotatef(camera.rotX, 1.0f, 0.0f, 0.0f);
    glRotatef(camera.rotY, 0.0f, 1.0f, 0.0f);
    glTranslatef(camera.posX, camera.posY, camera.posZ);
}

void Renderer::endFrame() {
    // Ensure we're in a clean state
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Swap buffers
    swapBuffers();
}

void Renderer::setCamera(const Camera& cam) {
    camera = cam;
}

Camera& Renderer::getCamera() {
    return camera;
}

void Renderer::renderModel(const Model& model, float x, float y, float z, float scale_factor) {
    pushMatrix();
    translate(x, y, z);
    
    if (scale_factor != 1.0f) {
        scale(scale_factor, scale_factor, scale_factor);
    }
    
    model.render();
    
    popMatrix();
}

void Renderer::renderColoredCube(float x, float y, float z, float width, float height, float depth,
                                 float r, float g, float b, float a) {
    pushMatrix();
    translate(x, y, z);
    scale(width, height, depth);
    
    enableTexturing(false);
    setColor(r, g, b, a);
    
    // Define cube vertices
    static const GLfloat cubeVerts[] = {
        // Top face
        -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        // Bottom face
        -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f,
        // Front face
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        // Back face
        -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f,
        // Left face
        -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f,
        // Right face
        0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f
    };
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, cubeVerts);
    
    // Indices for drawing the cube
    static const GLubyte cubeIndices[] = {
        0,  1,  2,  0,  2,  3,  // Top face
        4,  5,  6,  4,  6,  7,  // Bottom face
        8,  9,  10, 8,  10, 11, // Front face
        12, 13, 14, 12, 14, 15, // Back face
        16, 17, 18, 16, 18, 19, // Left face
        20, 21, 22, 20, 22, 23  // Right face
    };
    
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, cubeIndices);
    glDisableClientState(GL_VERTEX_ARRAY);
    
    enableTexturing(true);
    resetColor();
    popMatrix();
}

void Renderer::renderColoredPlane(float x, float y, float z, float width, float depth,
                                  float r, float g, float b, float a) {
    pushMatrix();
    translate(x, y, z);
    scale(width, 0.1f, depth);
    
    enableTexturing(false);
    setColor(r, g, b, a);
    
    // Simple plane vertices
    static const GLfloat planeVerts[] = {
        -0.5f, 0.0f, -0.5f,
        0.5f, 0.0f, -0.5f,
        0.5f, 0.0f, 0.5f,
        -0.5f, 0.0f, 0.5f
    };
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, planeVerts);
    
    static const GLubyte planeIndices[] = {0, 1, 2, 0, 2, 3};
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, planeIndices);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    enableTexturing(true);
    resetColor();
    
    popMatrix();
}

void Renderer::renderPoint(float x, float y, float z, float r, float g, float b, float a) {
    enableTexturing(false);
    setColor(r, g, b, a);
    
    glBegin(GL_POINTS);
    glVertex3f(x, y, z);
    glEnd();
    
    resetColor();
    enableTexturing(true);
}

// OpenGL state management helpers
void Renderer::setColor(float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
}

void Renderer::resetColor() {
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void Renderer::enableTexturing(bool enable) {
    if (enable) {
        glEnable(GL_TEXTURE_2D);
    } else {
        glDisable(GL_TEXTURE_2D);
    }
}

void Renderer::pushMatrix() {
    glPushMatrix();
}

void Renderer::popMatrix() {
    glPopMatrix();
}

void Renderer::translate(float x, float y, float z) {
    glTranslatef(x, y, z);
}

void Renderer::rotate(float angle, float x, float y, float z) {
    glRotatef(angle, x, y, z);
}

void Renderer::scale(float x, float y, float z) {
    glScalef(x, y, z);
}