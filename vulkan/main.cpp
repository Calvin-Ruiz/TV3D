#include <signal.h>
#include "Core.hpp"
#include "Graphics.hpp"
#include "TextureLoader.hpp"
#include <GL/glew.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <fstream>

bool effect = true;
bool rescale = false;
bool useDump = true;
int gheight = 0;
int gwidth = 0;

#define API GLFW_NO_API
//#define API GLFW_OPENGL_API

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
    std::string dumpPath;
    std::string dumpPreFile;
    std::string dumpPostFile;
    int fps = 30;
    std::ifstream file("config.txt");
    if (file) {
        std::string type;
        while (!file.eof()) {
            file >> type;
            if (type == "PATH") {
                file >> path >> preFile >> postFile;
            } else if (type == "DUMP") {
                file >> dumpPath >> dumpPreFile >> dumpPostFile;
            } else if (type == "FPS") {
                file >> fps;
            } else if (type == "EFFECT") {
                file >> type;
                effect = (type == "on");
            } else if (type == "SIZE") {
                file >> gwidth >> gheight;
            } else if (type == "USEDUMP") {
                file >> type;
                useDump = (type != "off");
            }
        }
    }
    file.close();

    Graphics *tmp = new Graphics(window, loaders);
    for (int i = 0; i < loaders.size(); ++i) {
        std::string num = std::to_string(i);
        loaders[i] = new TextureLoader(Graphics::vulkan.get(), i + 1, path + num + preFile + num + postFile, dumpPath + num + dumpPreFile + num + dumpPostFile);
    }
    core.startMainloop(fps, tmp);
    for (auto loader : loaders)
        core.startMainloop(fps, loader);
    core.mainloop();
}

int main(int argc, char const *argv[])
{
    (void) argc;
    (void) argv;
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, API);
    signal(SIGINT, sigTerm);
    signal(SIGTERM, sigTerm);
    auto monitor = glfwGetPrimaryMonitor();
    auto video = glfwGetVideoMode(monitor);
    int width = video->width;
    int height = video->height;
    GLFWwindow *VkWindow = glfwCreateWindow(width, height, "Vulkan", monitor, nullptr);
    start(VkWindow);
    glfwDestroyWindow(VkWindow);
    glfwTerminate();
    return 0;
}
