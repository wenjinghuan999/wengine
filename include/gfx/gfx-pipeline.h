#pragma once

#include "common/common.h"
#include "common/owned_resources.h"

#include <memory>

namespace wg {

class GfxVertexFactory {

};

class GfxPipelineState {

};

class GfxUniformLayout {

};

class GfxProgrammableStage {

};

class GfxPipeline : public std::enable_shared_from_this<GfxPipeline> {
public:
    static std::shared_ptr<GfxPipeline> Create(std::string name);
    ~GfxPipeline() = default;

public:
    const std::string& name() const { return name_; }
    const GfxVertexFactory& vertex_factroy() const { return vertex_factory_; }
    const GfxPipelineState& pipeline_state() const { return pipeline_state_; }
    const GfxUniformLayout& uniform_layout() const { return uniform_layout_; }
    const std::vector<GfxProgrammableStage>& programmable_stages() const { return programmable_stages_; }

    void setVertexFactroy(GfxVertexFactory vertex_factory) { vertex_factory_ = vertex_factory; valid_ = false; }
    void setPipelineState(GfxPipelineState pipeline_state) { pipeline_state_ = pipeline_state; valid_ = false; }
    void setUniformLayout(GfxUniformLayout uniform_layout) { uniform_layout_ = uniform_layout; valid_ = false; }
    void addProgrammableStage(GfxProgrammableStage programmable_stage) { programmable_stages_.push_back(programmable_stage); valid_ = false; }
    void setProgrammableStages(std::vector<GfxProgrammableStage> programmable_stages) { programmable_stages_ = std::move(programmable_stages); valid_ = false; }
    void clearProgrammableStages() { programmable_stages_.clear(); valid_ = false; }

    bool valid() const { return valid_; }
protected:
    std::string name_;
    GfxVertexFactory vertex_factory_;
    GfxPipelineState pipeline_state_;
    GfxUniformLayout uniform_layout_;
    std::vector<GfxProgrammableStage> programmable_stages_;
    bool valid_{false};
protected:
    explicit GfxPipeline(std::string name);
    friend class Gfx;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}