/*
** EPITECH PROJECT, 2020
** B-CPP-300-MAR-3-1-CPPrush3-antoine.viala
** File description:
** Core.hpp
*/

#ifndef CORE_HPP_
#define CORE_HPP_

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <thread>

class ThreadedModule;

class Core {
public:
    Core();
    virtual ~Core();
    Core(const Core &cpy) = delete;
    Core &operator=(const Core &src) = delete;

    void mainloop();
    void startMainloop(int refreshFrequency, ThreadedModule *module);
    void setPause(bool state) {isPaused = state;}
    void kill() {isAlive = false;}
    static Core *core;
    bool paused() {return isPaused;}
private:
    bool isAlive = false;
    bool isPaused = false;
    static void threadLoop(bool *isAlive, bool *pIsPaused, int refreshFrequency, ThreadedModule *engine);
    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<ThreadedModule>> modules;
};

#endif /* CORE_HPP_ */
