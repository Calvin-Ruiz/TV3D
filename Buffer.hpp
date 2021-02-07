/*
** EPITECH PROJECT, 2020
**
** File description:
** Buffer.hpp
*/

#ifndef BUFFER_HPP_
#define BUFFER_HPP_

#include <GL/glew.h>
#include <iostream>
#include <string>
#include <memory>
#include <map>

class Buffer {
public:
    typedef typeof(GL_ELEMENT_ARRAY_BUFFER) btype;
    typedef typeof(GL_STREAM_DRAW) atype;
    Buffer(btype type = GL_ELEMENT_ARRAY_BUFFER, atype access = GL_STATIC_DRAW);
    virtual ~Buffer();
    Buffer(const Buffer &cpy) = delete;
    Buffer &operator=(const Buffer &src) = delete;

    void bind();
    void update(float *data);
    void resize(int size);
    int size() {return _size;}
private:
    GLuint buffer;
    btype type;
    atype access;
    int _size = 0;
    static std::map<btype, GLuint> bound;
};

#endif /* BUFFER_HPP_ */
