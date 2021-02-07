/*
** EPITECH PROJECT, 2020
**
** File description:
** Buffer.cpp
*/
#include "Buffer.hpp"

std::map<Buffer::btype, GLuint> Buffer::bound;

Buffer::Buffer(btype type, atype access) : type(type), access(access)
{
    glGenBuffers(1, &buffer);
}

Buffer::~Buffer()
{
    glDeleteBuffers(1, &buffer);
}

void Buffer::bind()
{
    if (bound[type] != buffer) {
        glBindBuffer(type, buffer);
        bound[type] = buffer;
    }
}

void Buffer::update(float *data)
{
    bind();
    glBufferData(type, sizeof(float) * _size, data, this->access);
}

void Buffer::resize(int __size)
{
    _size = __size;
}
