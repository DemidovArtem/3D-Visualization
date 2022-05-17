#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <vector>

GLFWwindow *window;
using namespace glm;

std::vector<std::vector<glm::vec3>> getOctahedron(double r) {
    std::vector<std::vector<glm::vec3>> result;
    std::vector<glm::vec3> firstList = {
            glm::vec3(-r / std::sqrt(2), r / std::sqrt(2), 0),
            glm::vec3(r / std::sqrt(2), r / std::sqrt(2), 0),
            glm::vec3(r / std::sqrt(2), -r / std::sqrt(2), 0),
            glm::vec3(-r / std::sqrt(2), -r / std::sqrt(2), 0)
    };
    std::vector<glm::vec3> secondList = {
            glm::vec3(0, 0, -r),
            glm::vec3(0, 0, r)
    };
    for (int i = 0; i <= firstList.size(); ++i) {
        for (int j = 0; j < secondList.size(); ++j) {
            std::vector<glm::vec3> triangle = {
                    firstList[i],
                    firstList[(i + 1) % firstList.size()],
                    secondList[j]
            };
            result.push_back(triangle);
        }
    }
    return result;
}


std::vector<std::vector<glm::vec3>> convertTriangle(std::vector<glm::vec3> &triangle, double r) {
    std::vector<glm::vec3> points;
    for (int i = 0; i < triangle.size(); ++i) {
        points.push_back(triangle[i]);
        auto newPoint = triangle[i] + triangle[(i + 1) % triangle.size()];
        float length = glm::length(newPoint);
        for (int k = 0; k < 3; ++k) {
            newPoint[k] *= r / length;
        }
        points.push_back(newPoint);
    }
    std::vector<std::vector<glm::vec3>> result;
    std::vector<glm::vec3> finalTriangle;
    for (int i = 1; i < 6; i += 2) {
        std::vector<glm::vec3> newTriangle = {
                points[i],
                points[(i + 1) % points.size()],
                points[(i + 2) % points.size()]
        };
        finalTriangle.push_back(points[i]);
        result.push_back(newTriangle);
    }
    result.push_back(finalTriangle);
    return result;
}

std::vector<std::vector<glm::vec3>> convertFigure(std::vector<std::vector<glm::vec3>> &figure, double r) {
    std::vector<std::vector<glm::vec3>> result;
    for (auto &triangle: figure) {
        auto newTriangles = convertTriangle(triangle, r);
        for (auto &newTriangle: newTriangles) {
            result.push_back(newTriangle);
        }
    }
    return result;
}


int main(void) {
    double r = 0.5;
    auto figure = getOctahedron(r);

    // Initialise GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(1024, 768, "Homework 1 - Colored pyramid", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr,
                "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Dark blue background
//    glClearColor(0.07f, 0.25f, 0.67f, 0.0f);
    std::vector<float> backgroundColor = {0.02f, 0.15f, 0.44f};
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);



    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    GLuint programID = LoadShaders("TransformVertexShader.vertexshader", "ColorFragmentShader.fragmentshader");
    GLuint backgroundProgramID = LoadShaders( "BackgroundTransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader" );

    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    GLuint backgroundMatrixID = glGetUniformLocation(backgroundProgramID, "MVP");
//    GLuint Texture = loadBMP_custom("Sky2.bmp");
	GLuint Texture = loadDDS("uvtemplate.DDS");

    // Get a handle for our "myTextureSampler" uniform
    GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");

    // Projection matrix : 45 Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
    glm::mat4 Model = glm::mat4(1.0f);

    float shift = 0.0f;
    double startDr = 0.0005;
    double dr = startDr;
    double dDr = 0.0001;

    int specificationIndex = 0;
    int maxSpecification = 5;

    bool firstPlus = true;
    bool firstMinus = true;

    GLfloat background_g_vertex_buffer_data[] = {
            -1.0f,-1.0f,-1.0f, // Треугольник 1 : начало
            -1.0f,-1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f, // Треугольник 1 : конец
            1.0f, 1.0f,-1.0f, // Треугольник 2 : начало
            -1.0f,-1.0f,-1.0f,
            -1.0f, 1.0f,-1.0f, // Треугольник 2 : конец
            1.0f,-1.0f, 1.0f,
            -1.0f,-1.0f,-1.0f,
            1.0f,-1.0f,-1.0f,
            1.0f, 1.0f,-1.0f,
            1.0f,-1.0f,-1.0f,
            -1.0f,-1.0f,-1.0f,
            -1.0f,-1.0f,-1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f,-1.0f,
            1.0f,-1.0f, 1.0f,
            -1.0f,-1.0f, 1.0f,
            -1.0f,-1.0f,-1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f,-1.0f, 1.0f,
            1.0f,-1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f,-1.0f,-1.0f,
            1.0f, 1.0f,-1.0f,
            1.0f,-1.0f,-1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f,-1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f,-1.0f,
            -1.0f, 1.0f,-1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f,-1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f,-1.0f, 1.0f
    };
    static const GLfloat g_uv_buffer_data[] = {
            0.000059f, 1.0f-0.000004f,
            0.000103f, 1.0f-0.336048f,
            0.335973f, 1.0f-0.335903f,
            1.000023f, 1.0f-0.000013f,
            0.667979f, 1.0f-0.335851f,
            0.999958f, 1.0f-0.336064f,
            0.667979f, 1.0f-0.335851f,
            0.336024f, 1.0f-0.671877f,
            0.667969f, 1.0f-0.671889f,
            1.000023f, 1.0f-0.000013f,
            0.668104f, 1.0f-0.000013f,
            0.667979f, 1.0f-0.335851f,
            0.000059f, 1.0f-0.000004f,
            0.335973f, 1.0f-0.335903f,
            0.336098f, 1.0f-0.000071f,
            0.667979f, 1.0f-0.335851f,
            0.335973f, 1.0f-0.335903f,
            0.336024f, 1.0f-0.671877f,
            1.000004f, 1.0f-0.671847f,
            0.999958f, 1.0f-0.336064f,
            0.667979f, 1.0f-0.335851f,
            0.668104f, 1.0f-0.000013f,
            0.335973f, 1.0f-0.335903f,
            0.667979f, 1.0f-0.335851f,
            0.335973f, 1.0f-0.335903f,
            0.668104f, 1.0f-0.000013f,
            0.336098f, 1.0f-0.000071f,
            0.000103f, 1.0f-0.336048f,
            0.000004f, 1.0f-0.671870f,
            0.336024f, 1.0f-0.671877f,
            0.000103f, 1.0f-0.336048f,
            0.336024f, 1.0f-0.671877f,
            0.335973f, 1.0f-0.335903f,
            0.667969f, 1.0f-0.671889f,
            1.000004f, 1.0f-0.671847f,
            0.667979f, 1.0f-0.335851f
    };

    GLuint uvbuffer;
    glGenBuffers(1, &uvbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_uv_buffer_data), g_uv_buffer_data, GL_STATIC_DRAW);

    float backgroundSize = 5;
    for (int i =0; i < 108; i++){
        background_g_vertex_buffer_data[i] *= backgroundSize;
    }

    do {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLuint VertexArrayID;
        glGenVertexArrays(1, &VertexArrayID);
        glBindVertexArray(VertexArrayID);
        int figSize = int(figure.size()) * 9;
        GLfloat g_vertex_buffer_data[figSize
        + 108
        ];
        int index = 0;
        for (auto &triangle: figure) {
            auto center = triangle[0] + triangle[1] + triangle[2];
            for (int i = 0; i < 3; i++) {
                center[i] = center[i] / (3 * r) * shift;
            }
            for (auto &point: triangle) {
                for (int i = 0; i < 3; i++) {
                    g_vertex_buffer_data[index] = point[i] + center[i];
                    index++;
                }
            }
        }
        for (int i = figSize; i < figSize + 108; ++i){
            g_vertex_buffer_data[i] = background_g_vertex_buffer_data[i - figSize];
        }


        GLuint vertexbuffer;
        glGenBuffers(1, &vertexbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

        GLfloat g_color_buffer_data[figSize
        + 108
        ];
        for (int triangleIndex = 0; triangleIndex < figSize/9; ++triangleIndex) {
            for (int vertexIndex = 0; vertexIndex < 3; ++vertexIndex) {
                for (int colorIndex = 0; colorIndex < 3; ++colorIndex) {
                    int finalIndex = triangleIndex * 9 + vertexIndex * 3 + colorIndex;
                    if (colorIndex == 0) {
                        g_color_buffer_data[finalIndex] = 1.0;
                    } else {
                        if (colorIndex == 1) {
                            if (vertexIndex == 0) {
                                g_color_buffer_data[finalIndex] = 0.67;
                            } else {
                                if (vertexIndex == 1) {
                                    g_color_buffer_data[finalIndex] = 0.87;
                                } else {
                                    g_color_buffer_data[finalIndex] = 0.36;
                                }
                            }
                        }
                    }
                }
            }
        }

        for (int i = figSize; i < figSize + 108; ++i){
            g_color_buffer_data[i] = backgroundColor[(i - figSize) % 3];
        }


        GLuint colorbuffer;
        glGenBuffers(1, &colorbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);


        glm::mat4 View = glm::lookAt(
                glm::vec3(-1, -1, -1),
                glm::vec3(0, 0, 0), // and looks at the origin
                glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
        );
        glm::mat4 MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around

        dr = std::min(std::max(dr, -2 * startDr), 2 * startDr);
        shift += dr;
        shift = std::max(std::min(double(shift), r), 0.0);
        // Use our shader
        glUseProgram(programID);

        // Send our transformation to the currently bound shader,
        // in the "MVP" uniform
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

//        glUseProgram(backgroundProgramID);

        // Send our transformation to the currently bound shader,
        // in the "MVP" uniform
//        glUniformMatrix4fv(backgroundMatrixID, 1, GL_FALSE, &MVP[0][0]);

        // Bind our texture in Texture Unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture);
        // Set our "myTextureSampler" sampler to use Texture Unit 0
        glUniform1i(TextureID, 0);

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(
                0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void *) 0            // array buffer offset
        );

        // 2nd attribute buffer : colors
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
        glVertexAttribPointer(
                1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                3,                                // size
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void *) 0                          // array buffer offset
        );

        // Draw the triangles !
        glDrawArrays(GL_TRIANGLES, 0, figSize * 3
        + 108/3
        );

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
            if (firstPlus) {
                firstPlus = false;
                if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
                    specificationIndex += 1;
                    if (specificationIndex > maxSpecification) {
                        specificationIndex = maxSpecification;
                    } else {
                        figure = convertFigure(figure, r);
                    }
                } else {
                    dr += dDr;
                }

            }
        } else {
            firstPlus = true;
            if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
                if (firstMinus) {
                    firstMinus = false;
                    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
                        specificationIndex -= 1;
                        if (specificationIndex < 0) {
                            specificationIndex = 0;
                        } else {
                            figure = getOctahedron(r);
                            for (int i = 0; i < specificationIndex; ++i) {
                                figure = convertFigure(figure, r);
                            }
                        }
                    } else {
                        dr -= dDr;
                    }
                }
            } else {
                firstMinus = true;
            }
        }
        glDeleteBuffers(1, &vertexbuffer);
        glDeleteVertexArrays(1, &VertexArrayID);
        glDeleteBuffers(1, &colorbuffer);

    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
             glfwWindowShouldClose(window) == 0);

    glDeleteProgram(programID);
    glDeleteBuffers(1, &uvbuffer);

    glfwTerminate();
    return 0;
}