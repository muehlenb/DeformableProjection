// Include OpenGL3.3 Core functions:
#include <glad/glad.h>

#include "src/math/Vec4.h"

namespace DebugDraw {
    void drawLine(Vec4f start, Vec4f end){
        GLuint VBO_LineDraw, VAO_LineDraw;
        glGenVertexArrays(1, &VAO_LineDraw);
        glGenBuffers(1, &VBO_LineDraw);

        glBindVertexArray(VAO_LineDraw);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_LineDraw);

        float vertices[] = {
            start.x, start.y, start.z,
            end.x, end.y, end.z
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);

        glDrawArrays(GL_LINES, 0, 2); // Draw the line
        glBindVertexArray(0);

        glDeleteVertexArrays(1, &VAO_LineDraw);
        glDeleteBuffers(1, &VBO_LineDraw);
    }
}
