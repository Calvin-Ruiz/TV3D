#include <signal.h>
#include "Core.hpp"
#include "Graphics.hpp"
#include "TextureLoader.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fstream>

bool effect = true;
bool rescale = false;
int gheight = 0;
int gwidth = 0;

//#define API GLFW_NO_API
#define API GLFW_OPENGL_API

static void sigTerm(int sig)
{
    (void) sig;
    if (Core::core == nullptr)
        return;
    Core::core->kill();
    std::cout << "\rExiting...\n";
}

void start(GLFWwindow *window)
{
    Core core;
    std::array<TextureLoader *, 8> loaders;
    std::string path = "assets/source/source_0";
    std::string preFile = "/source_0";
    std::string postFile = "_";
    int fps = 30;
    std::ifstream file("config.txt");
    if (file) {
        std::string type;
        while (!file.eof()) {
            file >> type;
            if (type == "PATH") {
                file >> path >> preFile >> postFile;
            } else if (type == "FPS") {
                file >> fps;
            } else if (type == "EFFECT") {
                file >> type;
                effect = (type == "on");
            } else if (type == "FAST") {
                file >> type;
                rescale = (type == "off");
            }
        }
    }
    file.close();

    for (int i = 0; i < loaders.size(); ++i) {
        std::string num = std::to_string(i);
        loaders[i] = new TextureLoader(i + 1, path + num + preFile + num + postFile);
    }
    core.startMainloop(fps, new Graphics(window, loaders));
    for (auto loader : loaders)
        core.startMainloop(fps, loader);
    core.mainloop();
}

int main(int argc, char const *argv[])
{
    (void) argc;
    (void) argv;
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CLIENT_API, API);
    signal(SIGINT, sigTerm);
    signal(SIGTERM, sigTerm);
    auto monitor = glfwGetPrimaryMonitor();
    auto video = glfwGetVideoMode(monitor);
    gwidth = video->width;
    gheight = video->height;
    GLFWwindow *VkWindow = glfwCreateWindow(gwidth, gheight, "OpenGL 4", monitor, nullptr);
    start(VkWindow);
    glfwDestroyWindow(VkWindow);
    glfwTerminate();
    return 0;
}
