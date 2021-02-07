/*
** EPITECH PROJECT, 2020
** B-CPP-300-MAR-3-1-CPPrush3-antoine.viala
** File description:
** Core.cpp
*/
#include "Core.hpp"
#include "ThreadedModule.hpp"
#include <SFML/System/Clock.hpp>
#include <chrono>
#include <string>

Core *Core::core = nullptr;

Core::Core()
{
    core = this;
}

Core::~Core()
{
}

void Core::mainloop()
{
    std::string command = "\0";

    bool aliveProcess = false;
    int timeout = 200; // 10 seconds
    while (!aliveProcess && timeout--) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        aliveProcess = true;
        for (auto &module : modules) {
            aliveProcess &= module->isReady();
        }
    }
    if (timeout == -1) {
        isPaused = true;
        std::cerr << "Timeout : Modules has taken too many time to start.\n";
        return;
    }
    isAlive = true;
    while (isAlive && aliveProcess) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        for (auto &module : modules) {
            aliveProcess &= module->isReady();
        }
    }
    isAlive = false;
    for (auto &th : threads) {
        th.join();
    }
    threads.clear();
    modules.clear();
}

void Core::startMainloop(int refreshFrequency, ThreadedModule *module)
{
    modules.emplace_back(module);
    threads.emplace_back(threadLoop, &isAlive, &isPaused, refreshFrequency, std::move(module));
}

void Core::threadLoop(bool *pIsAlive, bool *pIsPaused, int refreshFrequency, ThreadedModule *module)
{
    bool &isAlive = *pIsAlive;
    bool &isPaused = *pIsPaused;
    module->initialize();

    // Wait other threads
    while (!isAlive && !isPaused) // isPaused is set to true in case of timeout
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 100 check per second

    std::chrono::duration delay = std::chrono::duration<int, std::ratio<1,1000000>>(1000000 / refreshFrequency);
    auto clock = std::chrono::system_clock::now();
    while (isAlive) {
        clock += delay;
        if (!isPaused)
            module->update();
        module->refresh();
        if (isPaused)
            module->onPause();
        std::this_thread::sleep_until(clock);
    }
    module->destroy();
}
