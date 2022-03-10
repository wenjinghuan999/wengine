#pragma once

#include <memory>
#include <vector>
#include <string>

#include "common/common.h"
#include "gfx/gfx-constants.h"
#include "gfx/gfx-pipeline.h"
#include "engine/texture.h"

namespace wg {

class MaterialRenderData : public IRenderData, public std::enable_shared_from_this<MaterialRenderData> {
public:
    ~MaterialRenderData() override = default;
    void createGfxResources(class Gfx& gfx) override;
    
public:
    std::shared_ptr<GfxPipeline> pipeline;
    std::vector<std::shared_ptr<Sampler>> samplers;
    
protected:
    friend class Material;
    MaterialRenderData() = default;
};

struct CombinedTextureSampler {
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Sampler> sampler;
};

struct MaterialConfig {
    float min_sample_shading = 0.f;
};

class Material : public IGfxObject, public std::enable_shared_from_this<Material> {
public:
    static std::shared_ptr<Material> Create(const std::string& name, 
        const std::string& vert_shader_filename, const std::string& frag_shader_filename);
    ~Material() override = default;

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] const MaterialConfig& config() const { return config_; }
    [[nodiscard]] MaterialConfig& config() { return config_; }
    
    void addTexture(const std::shared_ptr<Texture>& texture, SamplerConfig sampler_config = {}) {
        std::shared_ptr<Sampler> sampler = Sampler::Create(texture->image());
        sampler->setConfig(sampler_config);
        textures_.emplace_back(CombinedTextureSampler{ texture, sampler });
    }
    void clearTextures() { textures_.clear(); }
    [[nodiscard]] const std::vector<CombinedTextureSampler>& textures() const { return textures_; }
    
    std::shared_ptr<IRenderData> createRenderData() override;
    const std::shared_ptr<MaterialRenderData>& render_data() const { return render_data_; }
    
protected:
    std::string name_;
    std::string vert_shader_filename_;
    std::string frag_shader_filename_;
    MaterialConfig config_;
    std::vector<CombinedTextureSampler> textures_;
    std::shared_ptr<MaterialRenderData> render_data_;
    
protected:
    Material(const std::string& name,
        const std::string& vert_shader_filename, const std::string& frag_shader_filename);
    GfxVertexFactory createVertexFactory() const;
    GfxPipelineState createPipelineState() const;
    GfxUniformLayout createUniformLayout() const;
    GfxSamplerLayout createSamplerLayout() const;
};

}