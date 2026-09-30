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

/// Observe real NanoVG image allocation/deletion without changing the renderer.
struct ImageLifecycleCheck {
    NVGparams* params = nvgInternalParams(APP->window->vg);
    decltype(NVGparams::renderCreateTexture) create = params->renderCreateTexture;
    decltype(NVGparams::renderDeleteTexture) destroy = params->renderDeleteTexture;
    int created = 0;
    int deleted = 0;
    bool fail_creation = false;
    static ImageLifecycleCheck* active;

    ImageLifecycleCheck() {
        active = this;
        params->renderCreateTexture = [](void* user, int type, int width, int height,
            int flags, const unsigned char* pixels) {
            if (active->fail_creation) return 0;
            int image = active->create(user, type, width, height, flags, pixels);
            if (image) active->created++;
            return image;
        };
        params->renderDeleteTexture = [](void* user, int image) {
            active->deleted++;
            return active->destroy(user, image);
        };
    }

    ~ImageLifecycleCheck() {
        params->renderCreateTexture = create;
        params->renderDeleteTexture = destroy;
        active = nullptr;
    }
};

ImageLifecycleCheck* ImageLifecycleCheck::active = nullptr;

/// Verify allocation failure, context teardown/recreation, and module removal.
static void check_display_lifecycle() {
    ImageLifecycleCheck check;
    const uint8_t pixel[] = {255, 255, 255, 255};
    auto draw = [](Display& display) {
        auto vg = APP->window->vg;
        nvgBeginFrame(vg, 800, 420, 1.f);
        Widget::DrawArgs args = {};
        args.vg = vg;
        args.clipBox = Rect(Vec(0, 0), Vec(800, 420));
        display.drawLayer(args, 1);
        nvgEndFrame(vg);
    };
    {
        Display preview(Vec(), nullptr, Vec(1, 1), Vec(1, 1));
        preview.is_on = true;
        draw(preview);
        if (check.created != 0) throw std::runtime_error("Preview allocated an image");
        Display display(Vec(), pixel, Vec(1, 1), Vec(1, 1));
        display.is_on = true;
        check.fail_creation = true;
        draw(display);
        check.fail_creation = false;
        draw(display);
        draw(display);
        if (check.created != 1 || check.deleted != 0)
            throw std::runtime_error("Display did not retry/reuse its image");
        Widget::ContextDestroyEvent destroy;
        destroy.vg = APP->window->vg;
        display.onContextDestroy(destroy);
        display.onContextDestroy(destroy);
        if (check.deleted != 1)
            throw std::runtime_error("Context teardown did not delete exactly one image");
        Widget::ContextCreateEvent create;
        create.vg = APP->window->vg;
        display.onContextCreate(create);
        draw(display);
        if (check.created != 2)
            throw std::runtime_error("Display did not recreate its image");
    }
    if (check.deleted != 2)
        throw std::runtime_error("Display destruction leaked or double-deleted an image");
    std::cout << "Display image retry, reuse, context events, and destruction passed\n";
}

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
        // Menus and tooltips live on the scene, outside the module hierarchy.
        for (auto child : APP->scene->children) {
            if (!dynamic_cast<ui::Tooltip*>(child) &&
                !dynamic_cast<ui::MenuOverlay*>(child)) continue;
            child->step();
            APP->scene->drawChild(child, args);
        }
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

/// Find the production selector by its displayed assignment, not its layout.
static LedDisplayChoice* find_choice(Widget* widget, const std::string& text) {
    if (auto choice = dynamic_cast<LedDisplayChoice*>(widget))
        if (choice->text == text) return choice;
    for (auto child : widget->children)
        if (auto choice = find_choice(child, text)) return choice;
    return nullptr;
}

/// Capture hover help for a clipped name, and the actual assignment menu.
static void capture_genie_help(ModuleWidget* widget, const std::string& directory) {
    json_t* state = json_loads(
        "{\"Game\":0,\"Memory Locations\":[{\"Location\":6}]}", 0, nullptr);
    widget->module->dataFromJson(state);
    json_decref(state);
    widget->step();
    auto choice = find_choice(widget, "Player Horizontal Screen Position");
    if (!choice) throw std::runtime_error("Genie assignment label missing");
    APP->scene->box.size = Vec(800, 420);
    APP->scene->mousePos = Vec(190, 65);
    settings::tooltips = true;
    Widget::EnterEvent enter;
    choice->onEnter(enter);
    capture(widget, directory + "/CVGenie-Tooltip.ppm");
    Widget::LeaveEvent leave;
    choice->onLeave(leave);
    Widget::ActionEvent action;
    choice->onAction(action);
    capture(widget, directory + "/CVGenie-Menu.ppm");
    // Close the overlay before returning to ordinary panel captures.
    auto overlay = APP->scene->children.back();
    if (!dynamic_cast<ui::MenuOverlay*>(overlay))
        throw std::runtime_error("Genie assignment menu missing");
    APP->scene->removeChild(overlay);
    delete overlay;
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
        settings::showTipsOnLaunch = false;
        context.scene = new app::Scene;
        check_display_lifecycle();
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
            if (!is_nes) capture_genie_help(widget.get(), argv[3]);
            std::unique_ptr<ModuleWidget> preview(model->createModuleWidget(nullptr));
            for (const std::string theme : {"Light", "Dark"}) {
                preview->setPanel(context.window->loadSvg(asset::plugin(&plugin,
                    "res/" + name + "-" + theme + ".svg")));
                capture(preview.get(), std::string(argv[3]) + "/" + name
                    + "-Preview-" + theme + ".ppm");
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
