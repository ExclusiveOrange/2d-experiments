#pragma once

#include <optional>

#include <glm/glm.hpp>

namespace raycasting
{
  constexpr const glm::vec3 right{1.f, 0.f, 0.f};
  constexpr const glm::vec3 left = -right;
  
  constexpr const glm::vec3 forward{0.f, 1.f, 0.f};
  constexpr const glm::vec3 backward = -forward;
  
  constexpr const glm::vec3 up{0.f, 0.f, 1.f};
  constexpr const glm::vec3 down = -up;
  
  // NOTE: shadows
  // Probably want to avoid shadows on pre-rendered objects for now in order to avoid
  // contradictions with varying lighting environments during run-time.
  
  struct Ray
  {
    glm::vec3 origin, unitDirection;
  };
  
  struct Intersection
  {
    glm::vec3 position, normal;
    glm::vec3 diffuse;
    float distance;
  };
  
  struct Intersectable
  {
    virtual ~Intersectable() {}
    
    virtual std::optional<Intersection> intersect(const Ray &ray) const = 0;
  };
  
  struct DirectionalLight
  {
    DirectionalLight(glm::vec3 direction, glm::vec3 intensity)
      : ndirection{-direction}, intensity{intensity} {}
    
    glm::vec3 calculate(glm::vec3 position, glm::vec3 normal) const
    {
      return intensity * glm::dot(normal, ndirection);
    }
  
  private:
    const glm::vec3 ndirection, intensity;
  };
  
  struct OrthogonalCamera
  {
    glm::vec3 position, normal;
    glm::vec3 w, h;
    
    void render(
      const ViewOfCpuImageWithDepth &destImage,
      const Intersectable &intersectable,
      const DirectionalLight *directionalLights,
      int numDirectionalLights,
      float minDepth, // becomes uint8_t 0 (anything less is clamped at 0)
      float maxDepth) // becomes uint8_t 254 (anything greater is clamped at 255, the special transparency value)
    const
    {
      const glm::vec3 horStep = this->w / (float)destImage.w;
      const glm::vec3 verStep = this->h / (float)destImage.h;
      const float rDepthRange = 254.f / (maxDepth - minDepth);
      
      Ray ray{.unitDirection = this->normal};
      
      for (int y = 0; y < destImage.h; ++y)
      {
        glm::vec3 yOffset = (destImage.h * -0.5f + y + 0.5f) * verStep;
        
        for (int x = 0; x < destImage.w; ++x)
        {
          glm::vec3 xOffset = (destImage.w * -0.5f + x + 0.5f) * horStep;
          ray.origin = this->position + yOffset + xOffset;
          
          uint32_t drgb;
          
          if (std::optional<Intersection> i = intersectable.intersect(ray))
          {
            glm::vec3 lightSum{};
            
            for (int il = 0; il < numDirectionalLights; ++il)
              lightSum += directionalLights[il].calculate(i->position, i->normal);
            
            glm::vec3 color = 255.f * glm::clamp(lightSum, 0.f, 1.f);
            uint8_t depth = (uint8_t)glm::clamp((i->distance - minDepth) * rDepthRange, 0.f, 255.f);
            
            drgb = (depth << 24) | (uint32_t(color.x) << 16) | (uint32_t(color.y) << 8) | uint32_t(color.z);
          }
          else
            drgb = 0xff000000;
          
          int dindex = y * destImage.w + x;
          destImage.drgb[dindex] = drgb;
        }
      }
    }
  };
}
