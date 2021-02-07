/*
** EPITECH PROJECT, 2020
**
** File description:
** VertexArray.hpp
*/

#ifndef VERTEXBUFFER_HPP_
#define VERTEXBUFFER_HPP_

#include <GL/glew.h>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include "Buffer.hpp"

class VertexArray {
public:
    typedef typeof(GL_STREAM_DRAW) atype;
    VertexArray(atype access = GL_DYNAMIC_DRAW);
    virtual ~VertexArray();
    VertexArray(const VertexArray &cpy) = default;
    VertexArray &operator=(const VertexArray &src) = default;

    //! Bind internal buffer
    void bind();
    // Raw update, you must know what you have put on it
    void update(float *data);
    void resize(int size);
    int size();
    //! Add one component
    void addComponent(int size);
    //! Bind VertexArray to shader
    void bindLocation(int locationOffset = 0);
private:
    Buffer buffer;
    GLuint vertex;
    int blockSize = 0;
    struct block {
        int size;
        int offset;
    };
    std::vector<block> blocks;
    static GLuint bound;
};

#endif /* VERTEXBUFFER_HPP_ */
