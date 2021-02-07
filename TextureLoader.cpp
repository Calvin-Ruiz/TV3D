/*
** EPITECH PROJECT, 2020
**
** File description:
** TextureLoader.cpp
*/
#include "TextureLoader.hpp"
#include "Texture.hpp"

TextureLoader::TextureLoader(int id, const std::string &path) : id(id), path(path), count(0) {}

TextureLoader::~TextureLoader() {}

void TextureLoader::initialize()
{
    Texture::lock();
    for (int i = 0; i < 4; ++i)
        textures.emplace_back(new Texture(1920, 1080));
    Texture::unlock();
    ready = true;
}

void TextureLoader::update()
{
    if (count == 3) // Don't overwrite acquired texture
        return;
    std::string num = std::to_string(frameID++);
    std::string filler;
    filler.resize(4 - num.size(), '0');
    std::string filename = path + filler + num + ".png";
    if (textures[writeIndex]->load(filename)) {
        ++count;
        ++writeIndex;
        writeIndex &= 3;
    } else {
        std::cout << "No frame '" + filename + "' found\n";
        frameID = 0;
    }
}

bool TextureLoader::hasFrame()
{
    return (count > 0);
}

void TextureLoader::bindFrame()
{
    glActiveTexture(GL_TEXTURE0 + id);
    glBindTexture(TEX, textures[readIndex]->get());
}

void TextureLoader::releaseFrame()
{
    --count;
    ++readIndex;
    readIndex &= 3;
}
