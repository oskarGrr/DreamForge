#include "Texture.hpp"
#include "SDL.h"
#include "SDL_image.h"
#include <iostream>

Texture::Texture(std::string_view filePath, SDL_Renderer* renderer)
{
    m_texture = IMG_LoadTexture(renderer, filePath.data());
    if(!m_texture) 
    {
        std::cerr << IMG_GetError();
        //TODO error logging
    }

    SDL_QueryTexture(m_texture, nullptr, nullptr, &m_width, &m_height);
}

Texture::~Texture()
{
    if(m_texture) SDL_DestroyTexture(m_texture);
}

Texture::Texture(Texture&& oldObj) noexcept
    : m_texture {oldObj.m_texture}, m_width{oldObj.m_width}, m_height{oldObj.m_height}
{
    oldObj.m_texture = nullptr;
    oldObj.m_width   = 0;
    oldObj.m_height  = 0;
}

Texture& Texture::operator=(Texture&& rhs) noexcept
{
    SDL_DestroyTexture(m_texture);

    m_texture = rhs.m_texture;
    m_width   = rhs.m_width;
    m_height  = rhs.m_height;

    rhs.m_texture = nullptr;
    rhs.m_width   = 0;
    rhs.m_height  = 0;

    return *this;
}

void Texture::setAsRenderTarget(SDL_Renderer* renderer) const
{
    SDL_SetRenderTarget(renderer, m_texture);
}

void Texture::tint(glm::vec<3, U8> const& newColor)
{
    SDL_SetTextureColorMod(m_texture, newColor[0], newColor[1], newColor[2]);
}

void Texture::draw(glm::vec2 const& pos, SDL_Renderer* renderer) const
{
    SDL_Rect destination{pos.x, pos.y, m_width, m_height};
    SDL_RenderCopy(renderer, m_texture, nullptr, &destination);
}