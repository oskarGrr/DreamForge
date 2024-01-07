#pragma once
#include <string_view>
#include "HelpfulTypeAliases.hpp"
#include "glm/glm.hpp"

struct SDL_Texture;
struct SDL_Renderer;

//RAII wrapper for SDL textures
class Texture
{
public:
    Texture(std::string_view filePath, SDL_Renderer* renderer);
    ~Texture();

    Texture(Texture&&)noexcept;
    Texture& operator=(Texture&&)noexcept;


    Texture(Texture const&)=delete;
    Texture& operator=(Texture const&)=delete;
 
    auto getWidth()  const {return m_width;}
    auto getHeight() const {return m_height;}

    void draw(glm::vec2 const& pos, SDL_Renderer* renderer)const;
    void setAsRenderTarget(SDL_Renderer* renderer)const;
    void tint(glm::vec<3, U8> const& newColor);

private:
    SDL_Texture* m_texture{nullptr};
    S32 m_width{0}, m_height{0};
};