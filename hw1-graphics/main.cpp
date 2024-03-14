#include "Angel.h"
#include <iostream>
#include <vector>

typedef vec4  color4;
typedef vec4  point4;

int cube_or_ball = 0;

const int NumVertices = 36; //(6 faces)(2 triangles/face)(3 vertices/triangle)

point4 points[NumVertices];
color4 colors[NumVertices];

GLuint cubeVAO, cubeVBO;
GLuint sphereVAO, sphereVBO;

point4 cube_position = point4(-0.5, 0.0, 0.0, 1.0); // Starting at the left side of the box
vec4 cube_velocity = vec4(0.01, 0.01, 0.0, 0.0); // Initial speed to the right
color4 cube_color = color4(1.0, 0.0, 0.0, 1.0); // Red ball

point4 sphere_position = point4(0.5, 0.0, 0.0, 1.0); // Initial position of the sphere
vec4 sphere_velocity = vec4(-0.01, 0.02, 0.0, 0.0); // Velocity of the sphere


GLfloat cubeScale = 0.2; // Initial scale of the cube
GLfloat scaleStep = 0.01; // Amount by which to change the scale each frame
bool scalingUp = true; // Direction of scaling

// Sphere data
std::vector<point4> sphere_points;
std::vector<color4> sphere_colors;

// Sphere subdivision
int NumTimesToSubdivide = 5;
int NumSphereVertices = 0; // Updated during sphere generation

void initBall() {
    cube_position = point4(-0.5, 0.0, 0.0, 1.0); // Reset ball position to the left side
    cube_color = color4(1.0, 0.0, 0.0, 1.0); // Reset ball color to red
    // ... Set other properties if needed
}

// Vertices of a unit cube centered at origin, sides aligned with axes
point4 vertices[8] = {
    point4( -0.5, -0.5,  0.5, 1.0 ),
    point4( -0.5,  0.5,  0.5, 1.0 ),
    point4(  0.5,  0.5,  0.5, 1.0 ),
    point4(  0.5, -0.5,  0.5, 1.0 ),
    point4( -0.5, -0.5, -0.5, 1.0 ),
    point4( -0.5,  0.5, -0.5, 1.0 ),
    point4(  0.5,  0.5, -0.5, 1.0 ),
    point4(  0.5, -0.5, -0.5, 1.0 )
};

// RGBA olors
color4 vertex_colors[8] = {
    color4( 0.0, 0.0, 0.0, 1.0 ),  // black
    color4( 1.0, 0.0, 0.0, 1.0 ),  // red
    color4( 1.0, 1.0, 0.0, 1.0 ),  // yellow
    color4( 0.0, 1.0, 0.0, 1.0 ),  // green
    color4( 0.0, 0.0, 1.0, 1.0 ),  // blue
    color4( 1.0, 0.0, 1.0, 1.0 ),  // magenta
    color4( 1.0, 1.0, 1.0, 1.0 ),  // white
    color4( 0.0, 1.0, 1.0, 1.0 )   // cyan
};

void triangle(const point4& a, const point4& b, const point4& c) {
    sphere_colors.push_back(cube_color); // Use cube_color or any other color you prefer
    sphere_points.push_back(a);
    sphere_points.push_back(b);
    sphere_points.push_back(c);
    NumSphereVertices += 3;
}

point4 unit(const point4& p) {
    float len = sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    return point4(p.x / len, p.y / len, p.z / len, 1.0);
}

void divide_triangle(const point4& a, const point4& b, const point4& c, int count) {
    if (count > 0) {
        point4 v1 = unit(a + b);
        point4 v2 = unit(a + c);
        point4 v3 = unit(b + c);
        divide_triangle(a, v1, v2, count - 1);
        divide_triangle(c, v2, v3, count - 1);
        divide_triangle(b, v3, v1, count - 1);
        divide_triangle(v1, v3, v2, count - 1);
    } else {
        triangle(a, b, c);
    }
}

void tetrahedron(int count) {
    point4 v[4] = {
        vec4(0.0, 0.0, 1.0, 1.0),
        vec4(0.0, 0.942809, -0.333333, 1.0),
        vec4(-0.816497, -0.471405, -0.333333, 1.0),
        vec4(0.816497, -0.471405, -0.333333, 1.0)
    };

    divide_triangle(v[0], v[1], v[2], count);
    divide_triangle(v[3], v[2], v[1], count);
    divide_triangle(v[0], v[3], v[1], count);
    divide_triangle(v[0], v[2], v[3], count);
}


// Array of rotation angles (in degrees) for each coordinate axis
enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
int      Axis = Xaxis;
GLfloat  cubeTheta[NumAxes] = { 0.0, 0.0, 0.0 };

// Model-view and projection matrices uniform location
GLuint  ModelView, Projection;

//----------------------------------------------------------------------------

// quad generates two triangles for each face and assigns colors to the vertices
int Index = 0;

void quad( int a, int b, int c, int d )
{
    colors[Index] = vertex_colors[a]; points[Index] = vertices[a]; Index++;
    colors[Index] = vertex_colors[b]; points[Index] = vertices[b]; Index++;
    colors[Index] = vertex_colors[c]; points[Index] = vertices[c]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[a]; Index++;
    colors[Index] = vertex_colors[c]; points[Index] = vertices[c]; Index++;
    colors[Index] = vertex_colors[d]; points[Index] = vertices[d]; Index++;
}

//----------------------------------------------------------------------------

// generate 12 triangles: 36 vertices and 36 colors

void colorcube()
{
    quad( 1, 0, 3, 2 );
    quad( 2, 3, 7, 6 );
    quad( 3, 0, 4, 7 );
    quad( 6, 5, 1, 2 );
    quad( 4, 5, 6, 7 );
    quad( 5, 4, 0, 1 );
}

//---------------------------------------------------------------------
//
// init
//

void init()
{
    // Load and use shader program
        GLuint program = InitShader("vshader.glsl", "fshader.glsl");
        glUseProgram(program);

        // Retrieve transformation uniform variable locations
        ModelView = glGetUniformLocation(program, "ModelView");
        Projection = glGetUniformLocation(program, "Projection");
        GLuint vPosition = glGetAttribLocation(program, "vPosition");
        GLuint vColor = glGetAttribLocation(program, "vColor");

        // Set projection matrix
        mat4 projection = Ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
        glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

        glEnable(GL_DEPTH_TEST);
        glClearColor(1.0, 1.0, 1.0, 1.0);

        // Initialize cube
        colorcube(); // Assumes this populates 'points' and 'colors'

        glGenVertexArrays(1, &cubeVAO);
        glBindVertexArray(cubeVAO);

        glGenBuffers(1, &cubeVBO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);

        glEnableVertexAttribArray(vPosition);
        glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        glEnableVertexAttribArray(vColor);
        glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(points)));

        // Initialize sphere
        tetrahedron(NumTimesToSubdivide); // Fills 'sphere_points' and 'sphere_colors'

        glGenVertexArrays(1, &sphereVAO);
        glBindVertexArray(sphereVAO);

        glGenBuffers(1, &sphereVBO);
        glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
        glBufferData(GL_ARRAY_BUFFER, sphere_points.size() * sizeof(point4) + sphere_colors.size() * sizeof(color4), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sphere_points.size() * sizeof(point4), sphere_points.data());
        glBufferSubData(GL_ARRAY_BUFFER, sphere_points.size() * sizeof(point4), sphere_colors.size() * sizeof(color4), sphere_colors.data());

        glEnableVertexAttribArray(vPosition);
        glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        glEnableVertexAttribArray(vColor);
        glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sphere_points.size() * sizeof(point4)));
    
    glEnable( GL_DEPTH_TEST );
    glClearColor( 1.0, 1.0, 1.0, 1.0 );
    
    
}

void display1(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glBindVertexArray(cubeVAO);
    //  Generate the model-view matrix
    const vec3 displacement( 0.0, 0.0, 0.0 );
    mat4 model_view = Translate(cube_position.x, cube_position.y, cube_position.z) *
                          RotateX(cubeTheta[Xaxis]) *
                          RotateY(cubeTheta[Yaxis]) *
                          RotateZ(cubeTheta[Zaxis]) *
                          Scale(cubeScale, cubeScale, cubeScale); // Apply dynamic scaling

    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
    
    
    glDrawArrays( GL_TRIANGLES, 0, NumVertices );
    
    
    glFlush();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch( key ) {
        case GLFW_KEY_ESCAPE: case GLFW_KEY_Q:
            exit( EXIT_SUCCESS );
            break;
        case GLFW_KEY_X:
            cube_or_ball=0;
        case GLFW_KEY_A:
            cube_or_ball=1;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if ( action == GLFW_PRESS ) {
        switch( button ) {
            case GLFW_MOUSE_BUTTON_RIGHT:    Axis = Xaxis;  break;
            case GLFW_MOUSE_BUTTON_MIDDLE:  Axis = Yaxis;  break;
            case GLFW_MOUSE_BUTTON_LEFT:   Axis = Zaxis;  break;
        }
    }
}


void display2(){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(sphereVAO);
    // Generate the model-view matrix for the sphere
    mat4 model_view = Translate(cube_position.x, cube_position.y, cube_position.z) *
                        RotateX(cubeTheta[Xaxis]) *
                        RotateY(cubeTheta[Yaxis]) *
                        RotateZ(cubeTheta[Zaxis]) *
                        Scale(cubeScale, cubeScale, cubeScale);

    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);

    
    glDrawArrays(GL_TRIANGLES, 0, NumSphereVertices);

    glBindVertexArray(0);
    glFlush();
}

//---------------------------------------------------------------------
//
// main
//

void updateCubePosition() {
    // Update the cube's rotation
        cubeTheta[Xaxis] += 0.6; // adjust rotation speed as needed
        if (cubeTheta[Xaxis] > 360.0) cubeTheta[Xaxis] -= 360.0;

        cubeTheta[Yaxis] += 0.2; // adjust rotation speed as needed
        if (cubeTheta[Yaxis] > 360.0) cubeTheta[Yaxis] -= 360.0;

        cubeTheta[Zaxis] += 1.0; // adjust rotation speed as needed
        if (cubeTheta[Zaxis] > 360.0) cubeTheta[Zaxis] -= 360.0;

        // Update the cube's position
        cube_position.x += cube_velocity.x;
        cube_position.y += cube_velocity.y;

        // Check for collision with the box walls and bounce
        if (cube_position.x > 0.7 || cube_position.x < -0.7) {
            cube_velocity.x = -cube_velocity.x; // Reverse the x velocity
        }
        if (cube_position.y > 0.7 || cube_position.y < -0.7) {
            cube_velocity.y = -cube_velocity.y; // Reverse the y velocity
        }
        
        if (scalingUp) {
            cubeScale += scaleStep;
            if (cubeScale > 1.0) { // Set a maximum scale limit
                cubeScale = 1.0;
                scalingUp = false; // Start scaling down
            }
        } else {
            cubeScale -= scaleStep;
            if (cubeScale < 0.1) { // Set a minimum scale limit
                cubeScale = 0.1;
                scalingUp = true; // Start scaling up
            }
        }
        // No need to update the VBO for rotation, as it is done in the shader
}

void updateBallPosition() {
    // Update the sphere's position
    sphere_position.x += sphere_velocity.x;
    sphere_position.y += sphere_velocity.y;

    // Check for collision with the left or right sides of the window
    if (sphere_position.x > 1.0 || sphere_position.x < -1.0) {
        sphere_velocity.x = -sphere_velocity.x; // Reverse the x velocity
    }

    // Check for collision with the top or bottom of the window
    if (sphere_position.y > 1.0 || sphere_position.y < -1.0) {
        sphere_velocity.y = -sphere_velocity.y; // Reverse the y velocity
    }
}



int main()
{
    if (!glfwInit())
            exit(EXIT_FAILURE);
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    
    GLFWwindow* window = glfwCreateWindow(512, 512, "Spin Cube", NULL, NULL);
    glfwMakeContextCurrent(window);
    
    if (!window)
        {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    
    init();
    initBall();
    
    double frameRate = 60, currentTime, previousTime = 0.0;
    while (!glfwWindowShouldClose(window))
    {
//        currentTime = glfwGetTime();
//        if (currentTime - previousTime >= 1/frameRate & cube_or_ball==0){
//            previousTime = currentTime;
//            updateBallPosition();
//        }
        updateCubePosition(); // Existing cube update
        updateBallPosition(); // Update sphere position

        if (cube_or_ball == 0) {
            display1(); // Cube display
        }
        else if (cube_or_ball == 1) {
            display2(); // Sphere display
        }
        
        glfwSetKeyCallback(window, key_callback);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
