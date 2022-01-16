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
    
protected:
    friend class Material;
    MaterialRenderData() = default;
};

class Material : public IGfxObject, public std::enable_shared_from_this<Material> {
public:
    static std::shared_ptr<Material> Create(const std::string& name, 
        const std::string& vert_shader_filename, const std::string& frag_shader_filename);
    ~Material() override = default;
    
    void addTexture(const std::shared_ptr<Texture>& texture) {
        textures_.push_back(texture);
    }
    void clearTextures() { textures_.clear(); }
    [[nodiscard]] const std::vector<std::shared_ptr<Texture>>& textures() const { return textures_; }
    
    [[nodiscard]] const std::string& name() const { return name_; }
    std::shared_ptr<IRenderData> createRenderData() override;
    const std::shared_ptr<MaterialRenderData>& render_data() const { return render_data_; }
    
protected:
    std::string name_;
    std::string vert_shader_filename_;
    std::string frag_shader_filename_;
    std::vector<std::shared_ptr<Texture>> textures_;
    std::shared_ptr<MaterialRenderData> render_data_;
    
protected:
    Material(const std::string& name,
        const std::string& vert_shader_filename, const std::string& frag_shader_filename);
    GfxVertexFactory createVertexFactory() const;
    GfxUniformLayout createUniformLayout() const;
    GfxSamplerLayout createSamplerLayout() const;
};

}