/*
** EPITECH PROJECT, 2020
**
** File description:
** VertexArray.cpp
*/
#include "VertexArray.hpp"
#include "Buffer.hpp"

GLuint VertexArray::bound = 0;

VertexArray::VertexArray(atype access) : buffer(GL_ARRAY_BUFFER, access)
{
    glGenVertexArrays(1, &vertex);
}

VertexArray::~VertexArray()
{
    glDeleteVertexArrays(1, &vertex);
}

void VertexArray::addComponent(int _size)
{
    if (_size == 16) {
        // Support for Mat4f
        for (int i = 0; i < 4; ++i) {
            blocks.push_back({4, blockSize});
            blockSize += 4;
        }
    } else {
        blocks.push_back({_size, blockSize});
        blockSize += _size;
    }
}

void VertexArray::bind()
{
    if (vertex != bound) {
        glBindVertexArray(vertex);
        bound = vertex;
    }
}

void VertexArray::update(float *data)
{
    buffer.update(data);
}

void VertexArray::resize(int size)
{
    buffer.resize(size * blockSize);
}

int VertexArray::size()
{
    return buffer.size() / blockSize;
}

void VertexArray::bindLocation(int locationOffset)
{
    bind();
    for (unsigned int i = 0; i < blocks.size(); ++i) {
        glEnableVertexAttribArray(i + locationOffset);
        glVertexAttribPointer(i + locationOffset, blocks[i].size, GL_FLOAT, GL_FALSE, blockSize * sizeof(GLfloat), (void *) (long) (blocks[i].offset * sizeof(GLfloat)));
    }
}
