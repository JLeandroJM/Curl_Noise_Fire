#include "FrameParticleScene.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifdef HAS_OPENCV
#include <opencv2/opencv.hpp>
#endif

namespace {

struct ImageRGB {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;
};

enum class FrameMaterial : int {
    Background = 0,
    Paper,
    Fire,
    Smoke,
    Char,
    Cement,
    Wood,
    Ground,
    Leaf,
    Fabric,
    Carpet,
    Wall,
    MetalGlass
};

constexpr int MaterialCount = 13;

struct MaterialStats {
    int pixelCount = 0;
    int particleCount = 0;
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
};

struct MotionField {
    std::vector<float> magnitude;
    std::vector<glm::vec2> flow;
    std::string method = "disabled";
};

float clamp01(float v) {
    return std::max(0.0f, std::min(1.0f, v));
}

float hash01(int x, int y, int salt) {
    uint32_t h = static_cast<uint32_t>(x) * 374761393u
        + static_cast<uint32_t>(y) * 668265263u
        + static_cast<uint32_t>(salt) * 2246822519u;
    h = (h ^ (h >> 13u)) * 1274126177u;
    return static_cast<float>(h ^ (h >> 16u)) / 4294967295.0f;
}

std::string materialName(FrameMaterial material) {
    switch (material) {
        case FrameMaterial::Paper: return "PAPER/papel combustible";
        case FrameMaterial::Fire: return "FIRE/llama dinamica";
        case FrameMaterial::Smoke: return "SMOKE/humo";
        case FrameMaterial::Char: return "CHAR/carbonizado";
        case FrameMaterial::Cement: return "CEMENT/concreto";
        case FrameMaterial::Wood: return "WOOD/madera";
        case FrameMaterial::Ground: return "GROUND/suelo";
        case FrameMaterial::Leaf: return "LEAF/vegetacion";
        case FrameMaterial::Fabric: return "FABRIC/tela cortina cama";
        case FrameMaterial::Carpet: return "CARPET/alfombra";
        case FrameMaterial::Wall: return "WALL/pared no combustible";
        case FrameMaterial::MetalGlass: return "METAL_GLASS/metal vidrio";
        default: return "BACKGROUND/ignorado";
    }
}

uint32_t particleTypeFor(FrameMaterial material) {
    switch (material) {
        case FrameMaterial::Paper: return TYPE_PAPER;
        case FrameMaterial::Fire: return TYPE_FIRE;
        case FrameMaterial::Smoke: return TYPE_SMOKE;
        case FrameMaterial::Char: return TYPE_CHAR;
        case FrameMaterial::Cement: return TYPE_CEMENT;
        case FrameMaterial::Wood: return TYPE_WOOD;
        case FrameMaterial::Ground: return TYPE_GROUND;
        case FrameMaterial::Leaf: return TYPE_LEAF;
        case FrameMaterial::Fabric: return TYPE_FABRIC;
        case FrameMaterial::Carpet: return TYPE_CARPET;
        case FrameMaterial::Wall: return TYPE_WALL;
        case FrameMaterial::MetalGlass: return TYPE_METAL_GLASS;
        default: return TYPE_DEAD;
    }
}

bool isCombustibleMaterial(FrameMaterial material) {
    return material == FrameMaterial::Paper ||
        material == FrameMaterial::Wood ||
        material == FrameMaterial::Leaf ||
        material == FrameMaterial::Fabric ||
        material == FrameMaterial::Carpet;
}

bool isCombustibleType(uint32_t type) {
    return type == TYPE_PAPER ||
        type == TYPE_WOOD ||
        type == TYPE_LEAF ||
        type == TYPE_FABRIC ||
        type == TYPE_CARPET;
}

bool isImageFile(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga";
}

std::vector<std::filesystem::path> collectInputFrames(const std::string& inputPath) {
    std::vector<std::filesystem::path> frames;
    std::filesystem::path path(inputPath);

    if (std::filesystem::is_regular_file(path) && isImageFile(path)) {
        frames.push_back(path);
    } else if (std::filesystem::is_directory(path)) {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_regular_file() && isImageFile(entry.path())) {
                frames.push_back(entry.path());
            }
        }
    }

    std::sort(frames.begin(), frames.end());
    return frames;
}

bool loadImage(const std::filesystem::path& path, ImageRGB& image) {
    int channels = 0;
    unsigned char* data = stbi_load(path.string().c_str(), &image.width, &image.height, &channels, 3);
    if (!data) {
        return false;
    }

    image.pixels.assign(data, data + image.width * image.height * 3);
    stbi_image_free(data);
    return true;
}

float luminanceAt(const ImageRGB& img, int x, int y) {
    int idx = (y * img.width + x) * 3;
    float r = img.pixels[idx + 0] / 255.0f;
    float g = img.pixels[idx + 1] / 255.0f;
    float b = img.pixels[idx + 2] / 255.0f;
    return r * 0.2126f + g * 0.7152f + b * 0.0722f;
}

MotionField computeMotionField(const ImageRGB& current,
                               const ImageRGB* next,
                               bool enabled) {
    MotionField motion;
    motion.magnitude.assign(current.width * current.height, 0.0f);
    motion.flow.assign(current.width * current.height, glm::vec2(0.0f));

    if (!enabled || !next || next->width != current.width || next->height != current.height) {
        motion.method = enabled ? "disabled: necesita dos frames del mismo tamano" : "disabled";
        return motion;
    }

#ifdef HAS_OPENCV
    cv::Mat a(current.height, current.width, CV_8UC3, const_cast<unsigned char*>(current.pixels.data()));
    cv::Mat b(next->height, next->width, CV_8UC3, const_cast<unsigned char*>(next->pixels.data()));
    cv::Mat grayA;
    cv::Mat grayB;
    cv::Mat flow;
    cv::cvtColor(a, grayA, cv::COLOR_RGB2GRAY);
    cv::cvtColor(b, grayB, cv::COLOR_RGB2GRAY);
    cv::calcOpticalFlowFarneback(grayA, grayB, flow, 0.5, 3, 15, 3, 5, 1.2, 0);

    float maxMag = 0.0001f;
    for (int y = 0; y < current.height; ++y) {
        for (int x = 0; x < current.width; ++x) {
            cv::Point2f f = flow.at<cv::Point2f>(y, x);
            float mag = std::sqrt(f.x * f.x + f.y * f.y);
            int idx = y * current.width + x;
            motion.flow[idx] = glm::vec2(f.x, -f.y);
            motion.magnitude[idx] = mag;
            maxMag = std::max(maxMag, mag);
        }
    }

    for (float& mag : motion.magnitude) {
        mag = clamp01(mag / std::max(1.0f, maxMag * 0.65f));
    }
    motion.method = "OpenCV Farneback optical flow";
#else
    float maxDiff = 0.0001f;
    for (int y = 0; y < current.height; ++y) {
        for (int x = 0; x < current.width; ++x) {
            float diff = std::abs(luminanceAt(current, x, y) - luminanceAt(*next, x, y));
            int idx = y * current.width + x;
            motion.magnitude[idx] = diff;
            maxDiff = std::max(maxDiff, diff);
        }
    }

    for (float& mag : motion.magnitude) {
        mag = clamp01(mag / std::max(0.08f, maxDiff));
    }
    motion.method = "frame-difference fallback";
#endif

    return motion;
}

FrameMaterial classifyMaterial(float r, float g, float b, float motion, bool allowAutoFire) {
    float maxC = std::max(r, std::max(g, b));
    float minC = std::min(r, std::min(g, b));
    float sat = maxC <= 0.0001f ? 0.0f : (maxC - minC) / maxC;
    float lum = r * 0.2126f + g * 0.7152f + b * 0.0722f;

    bool orangeHot = r > 0.58f && g > 0.18f && g < 0.88f && b < 0.32f && sat > 0.38f;
    bool yellowHot = r > 0.75f && g > 0.55f && b < 0.40f && sat > 0.28f;
    if (allowAutoFire && (orangeHot || yellowHot)) {
        return FrameMaterial::Fire;
    }

    bool nearWhite = lum > 0.74f && sat < 0.24f;
    bool warmWall = r > g * 0.96f && g > b * 0.82f && lum > 0.46f && lum < 0.86f && sat < 0.34f;
    bool fabricLike = sat < 0.32f && lum > 0.24f && lum < 0.86f;
    bool reflective = lum > 0.78f && sat < 0.12f;

    if (reflective && !allowAutoFire) {
        return FrameMaterial::MetalGlass;
    }

    if (nearWhite) {
        return allowAutoFire ? FrameMaterial::Paper : FrameMaterial::Fabric;
    }

    if (lum < 0.15f && sat < 0.48f) {
        return FrameMaterial::Char;
    }

    bool gray = sat < 0.18f && lum > 0.16f && lum < 0.72f;
    if (gray) {
        if (motion > 0.22f) {
            return FrameMaterial::Smoke;
        }
        if (lum > 0.36f && lum < 0.68f) {
            return FrameMaterial::Fabric;
        }
        return FrameMaterial::Cement;
    }

    bool green = g > r * 1.08f && g > b * 1.08f && sat > 0.22f;
    if (green) {
        return FrameMaterial::Leaf;
    }

    bool brown = r > g * 1.05f && g > b * 1.05f && sat > 0.22f && lum < 0.55f;
    if (brown) {
        return FrameMaterial::Wood;
    }

    if (warmWall) {
        return FrameMaterial::Wall;
    }

    if (fabricLike) {
        return lum < 0.42f ? FrameMaterial::Carpet : FrameMaterial::Fabric;
    }

    if (lum > 0.18f && lum < 0.52f && sat < 0.35f) {
        return FrameMaterial::Ground;
    }

    return FrameMaterial::Background;
}

FrameMaterial visualFallbackMaterial(float r, float g, float b, float normY) {
    float maxC = std::max(r, std::max(g, b));
    float minC = std::min(r, std::min(g, b));
    float sat = maxC <= 0.0001f ? 0.0f : (maxC - minC) / maxC;
    float lum = r * 0.2126f + g * 0.7152f + b * 0.0722f;

    // El fondo (pixeles no reconocidos) se vuelve un telon NO combustible:
    // conserva el color de la foto pero no se incendia, para que solo arda el objeto.
    if (sat < 0.16f && lum > 0.70f) {
        return FrameMaterial::MetalGlass;
    }

    if (lum < 0.18f) {
        return FrameMaterial::Char;
    }

    return FrameMaterial::Wall;
}

FrameMaterial classifyIgnitionContact(const ImageRGB& frame, float normX, float normY) {
    int cx = std::clamp(static_cast<int>(normX * static_cast<float>(frame.width - 1)), 0, frame.width - 1);
    int cy = std::clamp(static_cast<int>(normY * static_cast<float>(frame.height - 1)), 0, frame.height - 1);
    std::array<int, MaterialCount> counts{};
    std::array<float, MaterialCount> nearest{};
    nearest.fill(std::numeric_limits<float>::max());

    int radius = std::max(10, std::min(frame.width, frame.height) / 80);
    for (int y = std::max(0, cy - radius); y <= std::min(frame.height - 1, cy + radius); ++y) {
        for (int x = std::max(0, cx - radius); x <= std::min(frame.width - 1, cx + radius); ++x) {
            float dx = static_cast<float>(x - cx);
            float dy = static_cast<float>(y - cy);
            float d2 = dx * dx + dy * dy;
            if (d2 > static_cast<float>(radius * radius)) {
                continue;
            }

            int idx = (y * frame.width + x) * 3;
            FrameMaterial material = classifyMaterial(
                frame.pixels[idx + 0] / 255.0f,
                frame.pixels[idx + 1] / 255.0f,
                frame.pixels[idx + 2] / 255.0f,
                0.0f,
                false
            );

            int mi = static_cast<int>(material);
            counts[mi]++;
            nearest[mi] = std::min(nearest[mi], d2);
        }
    }

    FrameMaterial bestCombustible = FrameMaterial::Background;
    int bestCombustibleCount = 0;
    for (FrameMaterial material : {
             FrameMaterial::Fabric,
             FrameMaterial::Carpet,
             FrameMaterial::Wood,
             FrameMaterial::Paper,
             FrameMaterial::Leaf
         }) {
        int mi = static_cast<int>(material);
        if (counts[mi] > bestCombustibleCount) {
            bestCombustible = material;
            bestCombustibleCount = counts[mi];
        }
    }

    int totalSamples = 0;
    for (int c : counts) {
        totalSamples += c;
    }

    if (normY > 0.78f) {
        int carpetVotes = counts[static_cast<int>(FrameMaterial::Carpet)] +
            counts[static_cast<int>(FrameMaterial::Fabric)] +
            counts[static_cast<int>(FrameMaterial::Ground)] +
            counts[static_cast<int>(FrameMaterial::Cement)];
        int hardObjectVotes = counts[static_cast<int>(FrameMaterial::Wood)] +
            counts[static_cast<int>(FrameMaterial::MetalGlass)] +
            counts[static_cast<int>(FrameMaterial::Char)];
        if (carpetVotes > hardObjectVotes) {
            return FrameMaterial::Carpet;
        }
    }

    if (bestCombustibleCount > std::max(6, totalSamples / 10)) {
        return bestCombustible;
    }

    FrameMaterial best = FrameMaterial::Background;
    int bestCount = 0;
    for (int i = 1; i < MaterialCount; ++i) {
        if (counts[static_cast<size_t>(i)] > bestCount) {
            best = static_cast<FrameMaterial>(i);
            bestCount = counts[static_cast<size_t>(i)];
        }
    }
    return best;
}

glm::vec4 colorForMaterial(FrameMaterial material, float r, float g, float b) {
    if (material != FrameMaterial::Fire && material != FrameMaterial::Smoke) {
        return glm::vec4(r, g, b, 1.0f);
    }

    switch (material) {
        case FrameMaterial::Paper:
            return glm::vec4(std::max(r, 0.86f), std::max(g, 0.85f), std::max(b, 0.80f), 1.0f);
        case FrameMaterial::Fire:
            return glm::vec4(1.0f, std::max(0.22f, g), std::max(0.03f, b * 0.55f), 0.88f);
        case FrameMaterial::Smoke:
            return glm::vec4(0.42f, 0.42f, 0.40f, 0.13f);
        case FrameMaterial::Char:
            return glm::vec4(0.030f, 0.026f, 0.022f, 1.0f);
        case FrameMaterial::Cement:
            return glm::vec4(r * 0.72f + 0.12f, g * 0.72f + 0.12f, b * 0.72f + 0.12f, 1.0f);
        case FrameMaterial::Wood:
            return glm::vec4(0.34f, 0.18f, 0.075f, 1.0f);
        case FrameMaterial::Ground:
            return glm::vec4(0.20f, 0.19f, 0.17f, 1.0f);
        case FrameMaterial::Leaf:
            return glm::vec4(0.12f, 0.35f, 0.13f, 1.0f);
        case FrameMaterial::Fabric:
            return glm::vec4(r * 0.92f + 0.04f, g * 0.92f + 0.04f, b * 0.92f + 0.04f, 1.0f);
        case FrameMaterial::Carpet:
            return glm::vec4(r * 0.78f + 0.08f, g * 0.78f + 0.08f, b * 0.78f + 0.08f, 1.0f);
        case FrameMaterial::Wall:
            return glm::vec4(r * 0.72f + 0.16f, g * 0.72f + 0.16f, b * 0.72f + 0.16f, 1.0f);
        case FrameMaterial::MetalGlass:
            return glm::vec4(r * 0.85f + 0.08f, g * 0.85f + 0.08f, b * 0.85f + 0.08f, 1.0f);
        default:
            return glm::vec4(0.0f);
    }
}

float baseParticleSize(FrameMaterial material, float worldWidth, int imageWidth, int stride) {
    float pixelWorld = worldWidth / static_cast<float>(imageWidth);
    float size = pixelWorld * static_cast<float>(stride) * 1.75f;
    switch (material) {
        case FrameMaterial::Fire: return std::max(size * 1.9f, 0.010f);
        case FrameMaterial::Smoke: return std::max(size * 2.8f, 0.018f);
        case FrameMaterial::Char: return size * 1.10f;
        case FrameMaterial::Paper: return size * 1.15f;
        case FrameMaterial::Fabric: return size * 1.18f;
        case FrameMaterial::Carpet: return size * 1.30f;
        default: return size * 1.05f;
    }
}

class FrameParticleScene : public Scene {
public:
    explicit FrameParticleScene(FrameParticleSceneConfig cfg)
        : config(std::move(cfg)) {}

    std::string getName() const override {
        return "Frame Particle Objects";
    }

    float getDuration() const override {
        return config.duration;
    }

    glm::vec3 getBackgroundColor() const override {
        return glm::vec3(0.055f, 0.060f, 0.065f);
    }

    void setup(std::vector<Particle>& particles,
               std::vector<EmitterConfig>& emitters,
               CurlNoiseParams& curlParams,
               CameraPath& cameraPath,
               bool lightweight) override {
        emitters.clear();

        std::vector<std::filesystem::path> framePaths = collectInputFrames(config.inputPath);
        if (framePaths.empty()) {
            std::cerr << "[FrameParticleScene] No encontre imagenes en: "
                      << config.inputPath << "\n";
            setupFallbackPoster(particles, emitters, curlParams, cameraPath, lightweight);
            return;
        }

        ImageRGB frame;
        if (!loadImage(framePaths.front(), frame)) {
            std::cerr << "[FrameParticleScene] No pude leer: "
                      << framePaths.front().string() << "\n";
            setupFallbackPoster(particles, emitters, curlParams, cameraPath, lightweight);
            return;
        }

        ImageRGB nextFrame;
        ImageRGB* nextFramePtr = nullptr;
        if (framePaths.size() > 1 && loadImage(framePaths[1], nextFrame)) {
            nextFramePtr = &nextFrame;
        }

        MotionField motion = computeMotionField(frame, nextFramePtr, config.motionFromFrames);

        int particleBudget = config.maxObjectParticles;
        particleBudget = std::max(1000, particleBudget);

        int stride = config.sampleStrideOverride > 0 ? config.sampleStrideOverride : std::max(
            1,
            static_cast<int>(std::ceil(std::sqrt(
                static_cast<float>(frame.width * frame.height) /
                static_cast<float>(particleBudget)))))
        ;

        std::array<MaterialStats, MaterialCount> stats{};
        std::vector<glm::vec3> visualFireSeeds;
        std::vector<glm::vec3> propagationSeeds;

        float worldWidth = config.worldWidth;
        float worldHeight = worldWidth * static_cast<float>(frame.height) /
            std::max(1.0f, static_cast<float>(frame.width));
        float worldCenterY = 0.95f;

        float ignitionNormX = config.ignitionNormX;
        float ignitionNormY = config.ignitionNormY;
        if (config.hasIgnitionPoint && config.ignitionUsesPixels) {
            ignitionNormX = clamp01(config.ignitionNormX / std::max(1.0f, static_cast<float>(frame.width - 1)));
            ignitionNormY = clamp01(config.ignitionNormY / std::max(1.0f, static_cast<float>(frame.height - 1)));
        }

        glm::vec3 ignitionWorld(
            (ignitionNormX - 0.5f) * worldWidth,
            worldCenterY + (0.5f - ignitionNormY) * worldHeight,
            -0.035f
        );

        FrameMaterial ignitionMaterial = FrameMaterial::Background;
        if (config.hasIgnitionPoint) {
            ignitionMaterial = classifyIgnitionContact(frame, ignitionNormX, ignitionNormY);
            visualFireSeeds.push_back(ignitionWorld);
            if (isCombustibleMaterial(ignitionMaterial)) {
                propagationSeeds.push_back(ignitionWorld);
            }
        }

        int ss = std::max(1, config.superSample);
        float invSS = 1.0f / static_cast<float>(ss);
        for (int y = 0; y < frame.height; y += stride) {
            for (int x = 0; x < frame.width; x += stride) {
                int idx = (y * frame.width + x) * 3;
                float r = frame.pixels[idx + 0] / 255.0f;
                float g = frame.pixels[idx + 1] / 255.0f;
                float b = frame.pixels[idx + 2] / 255.0f;
                int motionIdx = y * frame.width + x;
                float motionMag = motion.magnitude.empty() ? 0.0f : motion.magnitude[motionIdx];

                float nyPixel = static_cast<float>(y) / static_cast<float>(frame.height);

                FrameMaterial material = classifyMaterial(r, g, b, motionMag, !config.hasIgnitionPoint);
                stats[static_cast<int>(material)].pixelCount++;
                if (material == FrameMaterial::Background) {
                    material = visualFallbackMaterial(r, g, b, nyPixel);
                    stats[static_cast<int>(material)].pixelCount++;
                }

                // Telas/alfombras (ej. cortinas) quedan como telon calmado que NO arde;
                // asi solo se incendia el objeto solido (madera/papel/hoja).
                if (material == FrameMaterial::Fabric || material == FrameMaterial::Carpet) {
                    material = FrameMaterial::Wall;
                }

                uint32_t pType = particleTypeFor(material);
                glm::vec4 baseColor = colorForMaterial(material, r, g, b);
                float pSize = baseParticleSize(material, worldWidth, frame.width, stride) * invSS;
                glm::vec2 flow = motion.flow.empty() ? glm::vec2(0.0f) : motion.flow[motionIdx];

                // Supersampling: emite ss*ss particulas por pixel con jitter sub-pixel.
                for (int sy = 0; sy < ss; ++sy) {
                    for (int sx = 0; sx < ss; ++sx) {
                        int hx = x * ss + sx;
                        int hy = y * ss + sy;
                        float subx = (static_cast<float>(sx) + 0.5f) * invSS;
                        float suby = (static_cast<float>(sy) + 0.5f) * invSS;
                        subx += (hash01(hx, hy, 7) - 0.5f) * invSS * 0.6f;
                        suby += (hash01(hx, hy, 8) - 0.5f) * invSS * 0.6f;

                        float nx = (static_cast<float>(x) + subx * static_cast<float>(stride)) / static_cast<float>(frame.width);
                        float ny = (static_cast<float>(y) + suby * static_cast<float>(stride)) / static_cast<float>(frame.height);

                        float px = (nx - 0.5f) * worldWidth;
                        float py = worldCenterY + (0.5f - ny) * worldHeight;
                        float pz = -0.11f * motionMag - 0.025f * hash01(hx, hy, 3);

                        Particle p{};
                        p.type = pType;
                        p.position = glm::vec4(px, py, pz, pSize);
                        p.velocity = glm::vec4(0.0f);
                        p.color = baseColor;
                        p.lifetime = 1.0f;
                        p.maxLifetime = 1.0f;
                        p.temperature = 0.0f;

                        bool firstSub = (sx == 0 && sy == 0);
                        if (material == FrameMaterial::Fire) {
                            p.velocity = glm::vec4(flow.x * 0.010f, 0.38f + motionMag * 0.30f, flow.y * 0.010f, 1.0f);
                            p.lifetime = 0.65f + hash01(hx, hy, 4) * 0.65f;
                            p.maxLifetime = p.lifetime;
                            p.temperature = 0.92f;
                            if (firstSub) {
                                visualFireSeeds.push_back(glm::vec3(p.position));
                                propagationSeeds.push_back(glm::vec3(p.position));
                            }
                        } else if (material == FrameMaterial::Smoke) {
                            p.velocity = glm::vec4(flow.x * 0.006f, 0.12f + motionMag * 0.20f, flow.y * 0.006f, 1.0f);
                            p.lifetime = 2.2f + hash01(hx, hy, 5) * 1.8f;
                            p.maxLifetime = p.lifetime;
                            p.temperature = 0.12f;
                        } else if (material == FrameMaterial::Paper ||
                                   material == FrameMaterial::Wood ||
                                   material == FrameMaterial::Leaf ||
                                   material == FrameMaterial::Fabric ||
                                   material == FrameMaterial::Carpet) {
                            p.velocity.w = 999.0f;
                        } else {
                            p.velocity.w = 9999.0f;
                        }

                        updateBounds(stats[static_cast<int>(material)], px, py);
                        stats[static_cast<int>(material)].particleCount++;
                        particles.push_back(p);
                    }
                }
            }
        }

        if (config.hasIgnitionPoint) {
            addIgnitionFireParticles(particles, ignitionWorld, config.ignitionRadius, lightweight);
        }

        assignCombustibleBurnTimes(particles, propagationSeeds);
        addEmittersFromFireSeeds(emitters, visualFireSeeds, lightweight);
        configureCamera(cameraPath, worldHeight);

        curlParams.frequency = 2.2f;
        curlParams.amplitude = 1.10f;
        curlParams.octaves = 4;
        curlParams.lacunarity = 2.0f;
        curlParams.persistence = 0.48f;
        curlParams.timeScale = 0.90f;
        curlParams.epsilon = 0.01f;

        printReport(framePaths, frame, motion, stride, stats, visualFireSeeds.size(),
                    particles.size(), config.hasIgnitionPoint, ignitionNormX, ignitionNormY,
                    ignitionMaterial, !propagationSeeds.empty());
    }

private:
    FrameParticleSceneConfig config;

    static void updateBounds(MaterialStats& stats, float x, float y) {
        stats.minX = std::min(stats.minX, x);
        stats.maxX = std::max(stats.maxX, x);
        stats.minY = std::min(stats.minY, y);
        stats.maxY = std::max(stats.maxY, y);
    }

    static float distanceToFire(const glm::vec3& pos, const std::vector<glm::vec3>& fireSeeds) {
        if (fireSeeds.empty()) {
            return 0.0f;
        }

        float minD = std::numeric_limits<float>::max();
        int step = std::max(1, static_cast<int>(fireSeeds.size() / 300));
        for (size_t i = 0; i < fireSeeds.size(); i += static_cast<size_t>(step)) {
            minD = std::min(minD, glm::length(pos - fireSeeds[i]));
        }
        return minD;
    }

    static void addIgnitionFireParticles(std::vector<Particle>& particles,
                                         const glm::vec3& center,
                                         float radius,
                                         bool lightweight) {
        int count = lightweight ? 700 : 1800;
        for (int i = 0; i < count; ++i) {
            float a = hash01(i, 17, 1) * 6.2831853f;
            float t = hash01(i, 17, 3);
            float coneRadius = radius * (1.0f - t * 0.55f);
            float r = std::sqrt(hash01(i, 17, 2)) * coneRadius;
            float lift = t * radius * 2.2f;

            Particle p{};
            p.type = TYPE_FIRE;
            p.position = glm::vec4(
                center.x + std::cos(a) * r,
                center.y + lift,
                center.z + std::sin(a) * r * 0.35f,
                0.010f + hash01(i, 17, 4) * 0.012f
            );
            p.velocity = glm::vec4(
                (hash01(i, 17, 5) - 0.5f) * 0.10f,
                0.22f + hash01(i, 17, 6) * 0.34f,
                (hash01(i, 17, 7) - 0.5f) * 0.06f,
                1.0f
            );
            p.color = glm::vec4(1.0f, 0.62f + hash01(i, 17, 8) * 0.32f, 0.035f, 0.96f);
            p.lifetime = 0.45f + hash01(i, 17, 9) * 0.55f;
            p.maxLifetime = p.lifetime;
            p.temperature = 0.92f;
            particles.push_back(p);
        }
    }

    static void assignCombustibleBurnTimes(std::vector<Particle>& particles,
                                           const std::vector<glm::vec3>& fireSeeds) {
        for (auto& p : particles) {
            if (!isCombustibleType(p.type)) {
                continue;
            }

            if (fireSeeds.empty()) {
                p.velocity.w = 9999.0f;
                continue;
            }

            float seed = hash01(
                static_cast<int>(p.position.x * 10000.0f),
                static_cast<int>(p.position.y * 10000.0f),
                9
            );
            float dist = distanceToFire(glm::vec3(p.position), fireSeeds);

            if (p.type == TYPE_PAPER) {
                p.velocity.w = 0.5f + dist * 2.2f + seed * 0.8f;
            } else if (p.type == TYPE_LEAF) {
                p.velocity.w = 0.7f + dist * 3.0f + seed * 1.0f;
            } else if (p.type == TYPE_FABRIC) {
                p.velocity.w = 0.35f + dist * 2.6f + seed * 0.8f;
            } else if (p.type == TYPE_CARPET) {
                p.velocity.w = 0.3f + dist * 3.0f + seed * 1.0f;
            } else {
                p.velocity.w = 1.0f + dist * 4.5f + seed * 2.0f;
            }
        }
    }

    static void addEmittersFromFireSeeds(std::vector<EmitterConfig>& emitters,
                                         const std::vector<glm::vec3>& fireSeeds,
                                         bool lightweight) {
        if (fireSeeds.empty()) {
            return;
        }

        glm::vec3 minP(std::numeric_limits<float>::max());
        glm::vec3 maxP(std::numeric_limits<float>::lowest());
        for (const glm::vec3& seed : fireSeeds) {
            minP = glm::min(minP, seed);
            maxP = glm::max(maxP, seed);
        }

        glm::vec3 center = (minP + maxP) * 0.5f;
        EmitterConfig em;
        em.position = center;
        em.shape = EmitterShape::RECTANGLE;
        em.width = std::max(0.10f, maxP.x - minP.x);
        em.height = std::max(0.10f, maxP.y - minP.y);
        em.direction = glm::vec3(0.0f, 1.0f, 0.0f);
        em.emitRate = lightweight ? 3200.0f : 6000.0f;
        em.particleLife = 1.10f;
        em.lifeVariance = 0.55f;
        em.initialSpeed = 0.62f;
        em.speedVariance = 0.30f;
        em.temperature = 0.98f;
        em.particleSize = 0.022f;
        emitters.push_back(em);
    }

    void configureCamera(CameraPath& cameraPath, float worldHeight) const {
        cameraPath.totalDuration = getDuration();
        cameraPath.keyframes.clear();
        glm::vec3 target(0.0f, 0.95f, -0.02f);
        float cameraDistance = std::max(2.05f, config.worldWidth * 0.70f);
        float cameraY = 0.95f + worldHeight * 0.08f;
        cameraPath.keyframes.push_back({0.0f, glm::vec3(0.0f, cameraY, cameraDistance), target});
        cameraPath.keyframes.push_back({getDuration(), glm::vec3(0.04f, cameraY + 0.02f, cameraDistance * 0.98f), target});
    }

    void setupFallbackPoster(std::vector<Particle>& particles,
                             std::vector<EmitterConfig>& emitters,
                             CurlNoiseParams& curlParams,
                             CameraPath& cameraPath,
                             bool lightweight) {
        int countX = lightweight ? 180 : 360;
        int countY = lightweight ? 120 : 240;
        GeometryUtils::generatePlane(
            particles,
            glm::vec3(0.0f, 0.92f, 0.0f),
            glm::vec3(1.0f, 0.0f, 0.0f), 2.2f, countX,
            glm::vec3(0.0f, 1.0f, 0.0f), 1.35f, countY,
            TYPE_CEMENT,
            glm::vec4(0.36f, 0.37f, 0.36f, 1.0f),
            lightweight ? 0.010f : 0.006f
        );

        std::vector<glm::vec3> fireSeeds = {glm::vec3(-0.45f, 0.45f, -0.03f)};
        addEmittersFromFireSeeds(emitters, fireSeeds, lightweight);
        configureCamera(cameraPath, 1.35f);

        curlParams.frequency = 2.0f;
        curlParams.amplitude = 1.0f;
    }

    static void printReport(const std::vector<std::filesystem::path>& frames,
                            const ImageRGB& image,
                            const MotionField& motion,
                            int stride,
                            const std::array<MaterialStats, MaterialCount>& stats,
                            size_t fireSeedCount,
                            size_t totalParticles,
                            bool hasIgnition,
                            float ignitionNormX,
                            float ignitionNormY,
                            FrameMaterial ignitionMaterial,
                            bool propagationEnabled) {
        std::cout << "\n========== FRAME PARTICLE ANALYSIS ==========\n";
        std::cout << "Input: " << frames.front().string() << "\n";
        std::cout << "Frames detected: " << frames.size() << "\n";
        std::cout << "Resolution: " << image.width << "x" << image.height << "\n";
        std::cout << "FMA stride: cada " << stride << " pixeles\n";
        std::cout << "Motion-from-frames: " << motion.method << "\n";
        std::cout << "Fire seed particles: " << fireSeedCount << "\n";
        if (hasIgnition) {
            std::cout << "Ignition point: (" << ignitionNormX << ", " << ignitionNormY << ") normalized\n";
            std::cout << "Ignition material: " << materialName(ignitionMaterial) << "\n";
            std::cout << "Propagation: "
                      << (propagationEnabled ? "enabled over combustible materials" : "blocked: non-combustible contact")
                      << "\n";
        }
        std::cout << "Total GPU particles after import: " << totalParticles << "\n\n";
        std::cout << "Detected object/material types:\n";

        for (int i = 1; i < static_cast<int>(stats.size()); ++i) {
            const MaterialStats& s = stats[static_cast<size_t>(i)];
            if (s.particleCount == 0) {
                continue;
            }

            std::cout << "  - " << materialName(static_cast<FrameMaterial>(i))
                      << " | particles: " << s.particleCount
                      << " | source pixels: " << s.pixelCount
                      << " | bounds: [" << s.minX << ", " << s.minY
                      << "] -> [" << s.maxX << ", " << s.maxY << "]\n";
        }

        std::cout << "=============================================\n\n";
    }
};

} // namespace

std::unique_ptr<Scene> createFrameParticleScene(const FrameParticleSceneConfig& config) {
    return std::make_unique<FrameParticleScene>(config);
}
