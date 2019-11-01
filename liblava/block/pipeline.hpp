// file      : liblava/block/pipeline.hpp
// copyright : Copyright (c) 2018-present, Lava Block OÜ
// license   : MIT; see accompanying LICENSE file

#pragma once

#include <liblava/block/descriptor.hpp>
#include <liblava/base/base.hpp>
#include <liblava/core/math.hpp>

namespace lava {

struct pipeline_layout : id_obj {

    using ptr = std::shared_ptr<pipeline_layout>;
    using list = std::vector<ptr>;

    static ptr make() { return std::make_shared<pipeline_layout>(); }

    void add(descriptor::ptr const& descriptor) { descriptors.push_back(descriptor); }
    void add(VkPushConstantRange const& range) { push_constant_ranges.push_back(range); }
    
    void add_layout(descriptor::ptr const& layout) { add(layout); }
    void add_range(VkPushConstantRange const& range) { add(range); }

    bool create(device* device);
    void destroy();

    VkPipelineLayout get() const { return layout; }

    descriptor::list const& get_descriptors() const { return descriptors; }

    VkPushConstantRanges const& get_push_constant_ranges() const { return push_constant_ranges; }

    using offset_list = std::vector<ui32>;

    void bind_descriptor_set(VkCommandBuffer cmdBuffer, VkDescriptorSet descriptorSet, offset_list offsets = {});

private:
    device* dev = nullptr;

    VkPipelineLayout layout = nullptr;

    descriptor::list descriptors;
    VkPushConstantRanges push_constant_ranges;
};

constexpr name _main_ = "main";

struct pipeline : id_obj {

    using ptr = std::shared_ptr<pipeline>;
    using list = std::vector<ptr>;

    using process_func = std::function<void(VkCommandBuffer)>;
    process_func on_process;

    explicit pipeline(device* device, VkPipelineCache pipeline_cache = nullptr);
    ~pipeline() override;

    bool create();
    void destroy();

    virtual void bind(VkCommandBuffer cmd_buf) = 0;

    void set_active(bool value = true) { active = value; }
    bool is_active() const { return active; }

    void toggle() { active = !active; }

    void set_auto_bind(bool value = true) { auto_bind = value; }
    bool is_auto_bind() const { return auto_bind; }

    bool is_ready() const { return _pipeline != nullptr; }

    VkPipeline get() const { return _pipeline; }
    device* get_device() { return dev; }

    pipeline_layout::ptr get_layout() const { return layout; }
    void set_layout(pipeline_layout::ptr const& value) { layout = value; }

    struct shader_stage {

        using ptr = std::shared_ptr<shader_stage>;
        using list = std::vector<ptr>;

        explicit shader_stage();
        ~shader_stage();

        static shader_stage::ptr create(VkShaderStageFlagBits stage);
        static shader_stage::ptr create(device* device, data const& data, VkShaderStageFlagBits stage);

        void set_stage(VkShaderStageFlagBits stage) { create_info.stage = stage; }

        bool create(device* device, data const& data);
        void destroy();

        VkPipelineShaderStageCreateInfo const& get_create_info() const { return create_info; }

    private:
        device* dev = nullptr;

        VkPipelineShaderStageCreateInfo create_info;
    };

protected:
    virtual bool create_internal() = 0;
    virtual void destroy_internal() = 0;

    device* dev = nullptr;
    VkPipeline _pipeline = nullptr;

    VkPipelineCache pipeline_cache = nullptr;
    pipeline_layout::ptr layout;

private:
    bool active = true;
    bool auto_bind = false;
};

struct graphics_pipeline : pipeline {

    using ptr = std::shared_ptr<graphics_pipeline>;
    using map = std::map<id, ptr>;
    using list = std::vector<ptr>;

    enum class size_type : type {

        input = 0,
        absolute,
        relative
    };

    static ptr make(device* device, VkPipelineCache pipeline_cache = nullptr) {

        return std::make_shared<graphics_pipeline>(device, pipeline_cache);
    }

    explicit graphics_pipeline(device* device, VkPipelineCache pipeline_cache);

    void bind(VkCommandBuffer cmd_buf) override;
    void set_viewport_and_scissor(VkCommandBuffer cmd_buf, uv2 size);

    void set_render_pass(VkRenderPass pass) { render_pass = pass; }
    void set(VkRenderPass pass) { set_render_pass(pass); }

    VkRenderPass get_render_pass() const { return render_pass; }

    bool create(VkRenderPass pass) {
        
        set(pass);
        
        return pipeline::create();
    }

    void set_vertex_input_binding(VkVertexInputBindingDescription const& description);
    void set_vertex_input_bindings(VkVertexInputBindingDescriptions const& descriptions);

    void set_vertex_input_attribute(VkVertexInputAttributeDescription const& attribute);
    void set_vertex_input_attributes(VkVertexInputAttributeDescriptions const& attributes);

    void set_depth_test_and_write(bool test_enable, bool write_enable);
    void set_depth_compare_op(VkCompareOp compare_op);

    void set_rasterization_cull_mode(VkCullModeFlags cull_mode);
    void set_rasterization_front_face(VkFrontFace front_face);
    void set_rasterization_polygon_mode(VkPolygonMode polygon_mode);

    static VkPipelineColorBlendAttachmentState create_color_blend_attachment();
    void add_color_blend_attachment(VkPipelineColorBlendAttachmentState const& attachment = create_color_blend_attachment());

    void set_dynamic_states(VkDynamicStates const& states);
    void add_dynamic_state(VkDynamicState state);

    bool add_shader_stage(data const& data, VkShaderStageFlagBits stage);
    bool add_shader_stage(name filename, VkShaderStageFlagBits stage);
    void add(shader_stage::ptr const& shader_stage) { shader_stages.push_back(shader_stage); }

    shader_stage::list const& get_shader_stages() const { return shader_stages; }
    void clear_shader_stages() { shader_stages.clear(); }

    void set_auto_size(bool value = true) { auto_size = value; }
    bool is_auto_size() const { return auto_size; }

    VkViewport get_viewport() const { return viewport; }
    void set_viewport(VkViewport value) { viewport = value; }

    VkRect2D get_scissor() const { return scissor; }
    void set_scissor(VkRect2D value) { scissor = value; }

    size_type get_size_type() const { return _size_type; }
    void set_size_type(size_type value) { _size_type = value; }

    void copy_to(graphics_pipeline* target) const;
    void copy_from(ptr const& source) { source->copy_to(this); }

    void set_line_width(r32 value) { line_width = value; }
    r32 get_line_width() const { return line_width; }

    bool is_auto_line_width() const { return auto_line_width; }
    void set_auto_line_width(bool value = true) { auto_line_width = value; }

    void set_line_width(VkCommandBuffer cmd_buf) { vkCmdSetLineWidth(cmd_buf, line_width); }

private:
    bool create_internal() override;
    void destroy_internal() override;

    VkRenderPass render_pass = nullptr;

    VkPipelineVertexInputStateCreateInfo vertex_input_state;
    VkVertexInputBindingDescriptions vertex_input_bindings;
    VkVertexInputAttributeDescriptions vertex_input_attributes;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
    VkPipelineViewportStateCreateInfo viewport_state;
    VkPipelineMultisampleStateCreateInfo multisample_state;
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
    VkPipelineRasterizationStateCreateInfo rasterization_state;

    VkPipelineColorBlendStateCreateInfo color_blend_state;
    VkPipelineColorBlendAttachmentStates color_blend_attachment_states;

    VkPipelineDynamicStateCreateInfo dynamic_state;
    VkDynamicStates dynamic_states;

    shader_stage::list shader_stages;

    size_type _size_type = size_type::input;
    VkViewport viewport;
    VkRect2D scissor;

    bool auto_size = true;

    bool auto_line_width = false;
    r32 line_width = 1.f;
};

struct compute_pipeline : pipeline {

    using ptr = std::shared_ptr<compute_pipeline>;
    using map = std::map<id, ptr>;
    using list = std::vector<ptr>;

    using pipeline::pipeline;

    static ptr make(device* device, VkPipelineCache pipeline_cache = nullptr) {

        return std::make_shared<compute_pipeline>(device, pipeline_cache);
    }

    void bind(VkCommandBuffer cmdBuffer) override;

    bool set_shader_stage(data const& data, VkShaderStageFlagBits stage);
    bool set_shader_stage(name filename, VkShaderStageFlagBits stage);
    void set(shader_stage::ptr const& stage) { shader_stage = stage; }
    shader_stage::ptr const& get_shader_stage() const { return shader_stage; }

    void copy_to(compute_pipeline* target) const;
    void copy_from(ptr const& source) { source->copy_to(this); }

private:
    bool create_internal() override;
    void destroy_internal() override;

    shader_stage::ptr shader_stage;
};

} // lava