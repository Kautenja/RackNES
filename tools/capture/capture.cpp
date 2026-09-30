// Render production widgets with a real original ROM, without an audio device.
// Adapted from Fourier's test/rack/inspect_panels.cpp capture approach.
// Copyright 2026 Arhythmetic Units
// SPDX-License-Identifier: GPL-3.0-or-later

#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>
#include "../../src/RackNES.cpp"

Plugin* plugin_instance = nullptr;

/// @brief Load through the normal module path, then run two seconds at 48 kHz.
/// @details Processing finishes before any widget reads emulator pixels.
static void prepare_rom(RackNES* module, const std::string& path) {
    module->rom_path_signal = path;
    Module::ProcessArgs args = {};
    args.sampleRate = 48000.f;
    args.sampleTime = 1.f / args.sampleRate;
    for (int sample = 0; sample < 96000; ++sample) {
        args.frame = sample;
        module->process(args);
        if (module->rom_load_failed_signal || module->mapper_not_found_signal)
            throw std::runtime_error("Demo ROM failed to load");
    }
    const auto ram = module->emulator.get_memory_buffer();
    if (ram[0x11] != 0xA7 || ram[0x10] < 60)
        throw std::runtime_error("ROM initialization or frame heartbeat failed");
    std::set<uint32_t> colors;
    for (int i = 0; i < NES::Emulator::SCREEN_BYTES; i += 4) {
        const auto p = module->screen + i;
        colors.insert((uint32_t(p[0]) << 16) | (uint32_t(p[1]) << 8) | p[2]);
    }
    if (colors.size() < 4)
        throw std::runtime_error("Emulated video is blank or incomplete");
    std::cout << "ROM initialized; heartbeat=" << int(ram[0x10])
        << ", RGB colors=" << colors.size() << ", samples=96000 at 48000 Hz\n";
}

/// @brief Check all component caches, including buttons late in child order.
static void check_framebuffers(Widget* widget) {
    if (auto cache = dynamic_cast<FramebufferWidget*>(widget)) {
        if (!cache->bypassed && (cache->dirty || cache->getImageHandle() <= 0))
            throw std::runtime_error("Component framebuffer did not settle");
        // Nested caches render directly within the parent framebuffer.
        return;
    }
    for (auto child : widget->children) check_framebuffers(child);
}

/// @brief Draw all production layers and read native framebuffer pixels.
static void capture(ModuleWidget* widget, const std::string& filename) {
    constexpr int canvas_width = 800;
    constexpr int canvas_height = 420;
    int width, height;
    glfwGetFramebufferSize(APP->window->win, &width, &height);
    if (width % canvas_width || height != canvas_height * (width / canvas_width))
        throw std::runtime_error("Unexpected desktop framebuffer dimensions");
    const float ratio = width / float(canvas_width);
    APP->window->pixelRatio = ratio;
    // Rack caps framebuffer updates each frame; let all components settle.
    for (int frame = 0; frame < 160; ++frame) {
        widget->step();
        auto vg = APP->window->vg;
        nvgBeginFrame(vg, canvas_width, canvas_height, ratio);
        Widget::DrawArgs args = {};
        args.vg = vg;
        args.clipBox = Rect(Vec(0, 0), Vec(canvas_width, canvas_height));
        APP->window->fbCount() = 0;
        nvgSave(vg);
        nvgTranslate(vg, 10.f, 20.f);
        widget->draw(args);
        widget->drawLayer(args, 1);
        nvgRestore(vg);
        glViewport(0, 0, width, height);
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        nvgEndFrame(vg);
        glFinish();
        if (glGetError() != GL_NO_ERROR)
            throw std::runtime_error("OpenGL capture failed");
    }
    check_framebuffers(widget);
    std::vector<unsigned char> pixels(width * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    if (glGetError() != GL_NO_ERROR)
        throw std::runtime_error("Framebuffer read failed");
    std::ofstream output(filename, std::ios::binary);
    output << "P6\n" << width << ' ' << height << "\n255\n";
    for (int y = height - 1; y >= 0; --y)
        output.write(reinterpret_cast<const char*>(pixels.data() + y * width * 3), width * 3);
    if (!output) throw std::runtime_error("Cannot write " + filename);
    std::cout << filename << " pixelRatio=" << ratio << " GL=OK\n";
}

/// @brief Optional desktop tool; never loads personal patches or starts an engine thread.
int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: capture RACK_DIR PLUGIN_DIR OUTPUT_DIR\n";
        return 1;
    }
    Context context;
    contextSet(&context);
    context.engine = new engine::Engine;
    context.engine->setSampleRate(48000.f);
    context.event = new widget::EventState;
    context.history = new history::State;
    asset::systemDir = argv[1];
    asset::userDir = std::string(argv[3]) + "/user";
    plugin::Plugin plugin;
    plugin.path = argv[2];
    plugin.slug = "KautenjaDSP-RackNES";
    plugin.addModel(modelRackNES);
    plugin.addModel(modelInputGenie);
    plugin_instance = &plugin;
    if (!glfwInit()) {
        std::cerr << "Capture requires an active graphical desktop and OpenGL\n";
        return 2;
    }
    int result = 0;
    try {
        context.window = new window::Window;
        glfwHideWindow(context.window->win);
        glfwSetWindowSize(context.window->win, 800, 420);
        context.scene = new app::Scene;
        for (auto model : {modelRackNES, modelInputGenie}) {
            auto module = model->createModule();
            module->id = 1;
            context.engine->addModule(module);
            // ModuleWidget owns its module and removes it from the engine.
            std::unique_ptr<ModuleWidget> widget(model->createModuleWidget(module));
            const bool is_nes = model == modelRackNES;
            const std::string name = is_nes ? "RackNES" : "CVGenie";
            if (widget->box.size != Vec(is_nes ? 570.f : 180.f, 380.f))
                throw std::runtime_error("Module geometry changed; update crop and wireframe");
            if (is_nes) prepare_rom(static_cast<RackNES*>(module),
                std::string(argv[3]) + "/arhythmetic-units.nes");
            for (const std::string theme : {"Light", "Dark"}) {
                widget->setPanel(context.window->loadSvg(asset::plugin(&plugin,
                    "res/" + name + "-" + theme + ".svg")));
                capture(widget.get(), std::string(argv[3]) + "/" + name + "-" + theme + ".ppm");
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        result = 3;
    }
    delete context.scene;
    context.scene = nullptr;
    delete context.window;
    context.window = nullptr;
    glfwTerminate();
    contextSet(nullptr);
    return result;
}
