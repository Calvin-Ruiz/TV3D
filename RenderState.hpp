/*
** EPITECH PROJECT, 2020
** 
** File description:
** RenderState.hpp
*/

#ifndef RENDERSTATE_HPP_
#define RENDERSTATE_HPP_

#include <iostream>
#include <string>
#include <memory>

class RenderState {
public:
    RenderState();
    virtual ~RenderState();
    RenderState(const RenderState &cpy) = default;
    RenderState &operator=(const RenderState &src) = default;
private:
};

#endif /* RENDERSTATE_HPP_ */