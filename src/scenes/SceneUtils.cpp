#include "Scene.h"
#include <random>
#include <algorithm>

namespace GeometryUtils {

    void generatePlane(std::vector<Particle>& particles,
                       const glm::vec3& center,
                       const glm::vec3& u_dir, float u_size, int u_count,
                       const glm::vec3& v_dir, float v_size, int v_count,
                       uint32_t type, const glm::vec4& color, float pSize) {
        glm::vec3 corner = center - u_dir * (u_size * 0.5f) - v_dir * (v_size * 0.5f);
        float du = u_count > 1 ? u_size / (u_count - 1) : 0.0f;
        float dv = v_count > 1 ? v_size / (v_count - 1) : 0.0f;

        for (int i = 0; i < u_count; ++i) {
            for (int j = 0; j < v_count; ++j) {
                glm::vec3 pos = corner + u_dir * (i * du) + v_dir * (j * dv);
                Particle p{};
                p.position = glm::vec4(pos, pSize);
                p.velocity = glm::vec4(0.0f); // velocity.w will be burnStartTime
                p.color = color;
                p.lifetime = 10000.0f;
                p.maxLifetime = 10000.0f;
                p.temperature = 0.0f;
                p.type = type;
                particles.push_back(p);
            }
        }
    }

    void generateBox(std::vector<Particle>& particles,
                     const glm::vec3& center, const glm::vec3& size,
                     int countX, int countY, int countZ,
                     uint32_t type, const glm::vec4& color, float pSize) {
        glm::vec3 corner = center - size * 0.5f;
        float dx = countX > 1 ? size.x / (countX - 1) : 0.0f;
        float dy = countY > 1 ? size.y / (countY - 1) : 0.0f;
        float dz = countZ > 1 ? size.z / (countZ - 1) : 0.0f;

        for (int i = 0; i < countX; ++i) {
            for (int j = 0; j < countY; ++j) {
                for (int k = 0; k < countZ; ++k) {
                    glm::vec3 pos = corner + glm::vec3(i * dx, j * dy, k * dz);
                    Particle p{};
                    p.position = glm::vec4(pos, pSize);
                    p.velocity = glm::vec4(0.0f);
                    p.color = color;
                    p.lifetime = 10000.0f;
                    p.maxLifetime = 10000.0f;
                    p.temperature = 0.0f;
                    p.type = type;
                    particles.push_back(p);
                }
            }
        }
    }

    void generateCylinder(std::vector<Particle>& particles,
                          const glm::vec3& base, const glm::vec3& dir,
                          float radius, float height, int radial_count, int height_count,
                          uint32_t type, const glm::vec4& color, float pSize) {
        glm::vec3 d = glm::normalize(dir);
        glm::vec3 u = glm::vec3(0, 1, 0);
        if (std::abs(glm::dot(d, u)) > 0.99f) u = glm::vec3(1, 0, 0);
        glm::vec3 v = glm::normalize(glm::cross(d, u));
        u = glm::normalize(glm::cross(v, d));

        float dh = height_count > 1 ? height / (height_count - 1) : 0.0f;

        for (int i = 0; i < height_count; ++i) {
            glm::vec3 center = base + d * (i * dh);
            for (int j = 0; j < radial_count; ++j) {
                float theta = 2.0f * 3.14159265f * j / radial_count;
                glm::vec3 pos = center + (u * std::cos(theta) + v * std::sin(theta)) * radius;
                Particle p{};
                p.position = glm::vec4(pos, pSize);
                p.velocity = glm::vec4(0.0f);
                p.color = color;
                p.lifetime = 10000.0f;
                p.maxLifetime = 10000.0f;
                p.temperature = 0.0f;
                p.type = type;
                particles.push_back(p);
            }
        }
    }

    void generateSphere(std::vector<Particle>& particles,
                        const glm::vec3& center, float radius, int count,
                        uint32_t type, const glm::vec4& color, float pSize) {
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        for (int i = 0; i < count; ++i) {
            glm::vec3 pos;
            do {
                pos = glm::vec3(dist(rng), dist(rng), dist(rng));
            } while (glm::length(pos) > 1.0f);
            
            pos = center + pos * radius;
            Particle p{};
            p.position = glm::vec4(pos, pSize);
            p.velocity = glm::vec4(0.0f);
            p.color = color;
            p.lifetime = 10000.0f;
            p.maxLifetime = 10000.0f;
            p.temperature = 0.0f;
            p.type = type;
            particles.push_back(p);
        }
    }

    void generateLine(std::vector<Particle>& particles,
                      const glm::vec3& start, const glm::vec3& end, int count,
                      uint32_t type, const glm::vec4& color, float pSize) {
        glm::vec3 dir = end - start;
        for (int i = 0; i < count; ++i) {
            float t = count > 1 ? static_cast<float>(i) / (count - 1) : 0.0f;
            glm::vec3 pos = start + dir * t;
            Particle p{};
            p.position = glm::vec4(pos, pSize);
            p.velocity = glm::vec4(0.0f);
            p.color = color;
            p.lifetime = 10000.0f;
            p.maxLifetime = 10000.0f;
            p.temperature = 0.0f;
            p.type = type;
            particles.push_back(p);
        }
    }

    void computeBurnTimes(std::vector<Particle>& particles,
                          const std::vector<glm::vec3>& ignitionPoints,
                          float burnSpeed, float randomVariation) {
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(-randomVariation, randomVariation);

        for (auto& p : particles) {
            if (p.type >= TYPE_PAPER && p.type <= TYPE_GROUND) {
                float minDist = 1e9f;
                glm::vec3 pos = glm::vec3(p.position);
                for (const auto& ip : ignitionPoints) {
                    float d = glm::distance(pos, ip);
                    if (d < minDist) {
                        minDist = d;
                    }
                }
                float burnTime = minDist / burnSpeed + dist(rng);
                p.velocity.w = std::max(0.0f, burnTime);
            }
        }
    }

}
