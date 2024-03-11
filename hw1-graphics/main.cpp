//
//  main.cpp
//  hw1-graphics
//
//  Created by Doğukan Yaprak on 6.03.2024.
//
#include "Angel.h"
#include <iostream>

typedef vec4  color4;
typedef vec4  point4;

const int NumVertices = 36; //(6 faces)(2 triangles/face)(3 vertices/triangle)

point4 points[NumVertices];
color4 colors[NumVertices];

point4 ball_position = point4(-0.5, 0.0, 0.0, 1.0); // Starting at the left side of the box
vec4 ball_velocity = vec4(0.01, 0.01, 0.0, 0.0); // Initial speed to the right
color4 ball_color = color4(1.0, 0.0, 0.0, 1.0); // Red ball

GLfloat cubeScale = 0.2; // Initial scale of the cube
GLfloat scaleStep = 0.01; // Amount by which to change the scale each frame
bool scalingUp = true; // Direction of scaling

void initBall() {
    ball_position = point4(-0.5, 0.0, 0.0, 1.0); // Reset ball position to the left side
    ball_color = color4(1.0, 0.0, 0.0, 1.0); // Reset ball color to red
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
    // Load shaders and use the resulting shader program
    GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );
    glUseProgram( program );
    
    colorcube(); // create the cube in terms of 6 faces each of which is made of two triangles

    // Create a vertex array object
    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );
    
    // Create and initialize a buffer object
    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors), NULL, GL_STATIC_DRAW );
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(points), points );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors );

    // set up vertex arrays
    GLuint vPosition = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( vPosition );
    glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

    GLuint vColor = glGetAttribLocation( program, "vColor" );
    glEnableVertexAttribArray( vColor );
    glVertexAttribPointer( vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(points)) );

    // Retrieve transformation uniform variable locations
    ModelView = glGetUniformLocation( program, "ModelView" );
    Projection = glGetUniformLocation( program, "Projection" );
    
    // Set projection matrix
    mat4  projection;
    projection = Ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0); // Ortho(): user-defined function in mat.h
    glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );
    
    glEnable( GL_DEPTH_TEST );
    glClearColor( 1.0, 1.0, 1.0, 1.0 );
}

//---------------------------------------------------------------------
//
// display
//

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    //  Generate the model-view matrix
    const vec3 displacement( 0.0, 0.0, 0.0 );
    mat4 model_view = Translate(ball_position.x, ball_position.y, ball_position.z) *
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

//---------------------------------------------------------------------
//
// main
//

void updateBallPosition() {
    // Update the cube's rotation
        cubeTheta[Xaxis] += 0.6; // adjust rotation speed as needed
        if (cubeTheta[Xaxis] > 360.0) cubeTheta[Xaxis] -= 360.0;

        cubeTheta[Yaxis] += 0.2; // adjust rotation speed as needed
        if (cubeTheta[Yaxis] > 360.0) cubeTheta[Yaxis] -= 360.0;

        cubeTheta[Zaxis] += 1.0; // adjust rotation speed as needed
        if (cubeTheta[Zaxis] > 360.0) cubeTheta[Zaxis] -= 360.0;

        // Update the cube's position
        ball_position.x += ball_velocity.x;
        ball_position.y += ball_velocity.y;

        // Check for collision with the box walls and bounce
        if (ball_position.x > 0.7 || ball_position.x < -0.7) {
            ball_velocity.x = -ball_velocity.x; // Reverse the x velocity
        }
        if (ball_position.y > 0.7 || ball_position.y < -0.7) {
            ball_velocity.y = -ball_velocity.y; // Reverse the y velocity
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
        currentTime = glfwGetTime();
        if (currentTime - previousTime >= 1/frameRate){
            previousTime = currentTime;
            updateBallPosition();
        }
        
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
