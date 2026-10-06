#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_video.h>

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl3.h>
#include <imgui.h>

#include <wayland-client.h>
#define namespace namespace_
#include <wlr-layer-shell-unstable-v1-client-protocol.h>
#undef namespace

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

// ============================================================
// Application configuration
// ============================================================

struct ApplicationConfig {
  std::string ApplicationName;
  std::uint32_t WindowWidth;
  std::uint32_t WindowHeight;
};

static const ApplicationConfig AppConfig{
    .ApplicationName = "Base Template",
    .WindowWidth = 800,
    .WindowHeight = 600,
};

// ============================================================
// Wayland state
// ============================================================

struct WaylandState {
  wl_display *display = nullptr;
  wl_registry *registry = nullptr;
  wl_compositor *compositor = nullptr;

  zwlr_layer_shell_v1 *layer_shell = nullptr;
  zwlr_layer_surface_v1 *layer_surface = nullptr;

  wl_surface *surface = nullptr;

  std::uint32_t width = 800;
  std::uint32_t height = 600;

  bool configured = false;
  bool closed = false;
};

// ============================================================
// Wayland registry
// ============================================================

static void RegistryGlobal(void *data, wl_registry *registry,
                           std::uint32_t name, const char *interface,
                           std::uint32_t version) {
  auto *state = static_cast<WaylandState *>(data);

  if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
    state->compositor = static_cast<wl_compositor *>(wl_registry_bind(
        registry, name, &wl_compositor_interface, std::min(version, 4u)));
  } else if (std::strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
    state->layer_shell = static_cast<zwlr_layer_shell_v1 *>(wl_registry_bind(
        registry, name, &zwlr_layer_shell_v1_interface, std::min(version, 4u)));
  }
}

static void RegistryGlobalRemove(void *, wl_registry *, std::uint32_t) {}

static const wl_registry_listener RegistryListener{
    .global = RegistryGlobal,
    .global_remove = RegistryGlobalRemove,
};

// ============================================================
// Layer-shell callbacks
// ============================================================

static void LayerConfigure(void *data, zwlr_layer_surface_v1 *layer_surface,
                           std::uint32_t serial, std::uint32_t width,
                           std::uint32_t height) {
  auto *state = static_cast<WaylandState *>(data);

  zwlr_layer_surface_v1_ack_configure(layer_surface, serial);

  /*
   * When both left+right and top+bottom are anchored,
   * the compositor chooses the actual size.
   */
  if (width != 0) {
    state->width = width;
  }

  if (height != 0) {
    state->height = height;
  }

  state->configured = true;
}

static void LayerClosed(void *data, zwlr_layer_surface_v1 *) {
  auto *state = static_cast<WaylandState *>(data);

  state->closed = true;
}

static const zwlr_layer_surface_v1_listener LayerSurfaceListener{
    .configure = LayerConfigure,
    .closed = LayerClosed,
};

// ============================================================
// ImGui styling
// ============================================================

static ImVec4 HexColor(int r, int g, int b, int a = 255) {
  return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

static void SetupImGuiStyles() {
  ImGuiStyle &style = ImGui::GetStyle();

  style.WindowRounding = 0.0f;
  style.ChildRounding = 0.0f;
  style.FrameRounding = 0.0f;
  style.PopupRounding = 0.0f;
  style.ScrollbarRounding = 0.0f;
  style.GrabRounding = 0.0f;
  style.TabRounding = 0.0f;

  style.WindowBorderSize = 3.0f;
  style.ChildBorderSize = 3.0f;
  style.PopupBorderSize = 3.0f;
  style.FrameBorderSize = 2.0f;

  style.WindowPadding = ImVec2(12.0f, 12.0f);
  style.FramePadding = ImVec2(10.0f, 7.0f);
  style.ItemSpacing = ImVec2(10.0f, 10.0f);

  style.Colors[ImGuiCol_WindowBg] = HexColor(0xF5, 0xF5, 0xF0);

  style.Colors[ImGuiCol_ChildBg] = HexColor(0xFF, 0xFF, 0xFF);

  style.Colors[ImGuiCol_Text] = HexColor(0x11, 0x11, 0x11);

  style.Colors[ImGuiCol_Border] = HexColor(0x11, 0x11, 0x11);

  style.Colors[ImGuiCol_Button] = HexColor(0xFF, 0xD4, 0x3B);

  style.Colors[ImGuiCol_ButtonHovered] = HexColor(0xFF, 0xE0, 0x65);

  style.Colors[ImGuiCol_ButtonActive] = HexColor(0xE6, 0xBA, 0x20);
}

// ============================================================
// Main
// ============================================================

int main() {
  // --------------------------------------------------------
  // SDL
  // --------------------------------------------------------

  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';

    return EXIT_FAILURE;
  }

  // --------------------------------------------------------
  // OpenGL configuration
  // --------------------------------------------------------

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  /*
   * We need an RGBA framebuffer.
   */
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

  // --------------------------------------------------------
  // Create SDL custom Wayland surface
  // --------------------------------------------------------

  SDL_PropertiesID props = SDL_CreateProperties();

  SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING,
                        AppConfig.ApplicationName.c_str());

  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER,
                        AppConfig.WindowWidth);

  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER,
                        AppConfig.WindowHeight);

  SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);

  /*
   * CRITICAL:
   *
   * Do NOT let SDL give the wl_surface an xdg_toplevel role.
   *
   * We will assign zwlr_layer_surface_v1 ourselves.
   */
  SDL_SetBooleanProperty(
      props, SDL_PROP_WINDOW_CREATE_WAYLAND_SURFACE_ROLE_CUSTOM_BOOLEAN, true);

  /*
   * Tell SDL that the surface contains transparent pixels.
   */
  SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_TRANSPARENT_BOOLEAN,
                         true);

  /*
   * Avoid automatic HiDPI handling for this minimal example.
   * The layer-shell dimensions directly correspond to the
   * OpenGL surface dimensions.
   */
  SDL_SetBooleanProperty(
      props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, false);

  SDL_Window *window = SDL_CreateWindowWithProperties(props);

  SDL_DestroyProperties(props);

  if (!window) {
    std::cerr << "SDL_CreateWindowWithProperties failed: " << SDL_GetError()
              << '\n';

    SDL_Quit();
    return EXIT_FAILURE;
  }

  // --------------------------------------------------------
  // OpenGL context
  // --------------------------------------------------------

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);

  if (!gl_context) {
    std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << '\n';

    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_FAILURE;
  }

  SDL_GL_MakeCurrent(window, gl_context);

  SDL_GL_SetSwapInterval(1);

  // --------------------------------------------------------
  // Get SDL's native Wayland objects
  // --------------------------------------------------------

  SDL_PropertiesID window_props = SDL_GetWindowProperties(window);

  WaylandState wayland{};

  wayland.display = static_cast<wl_display *>(SDL_GetPointerProperty(
      window_props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr));

  wayland.surface = static_cast<wl_surface *>(SDL_GetPointerProperty(
      window_props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr));

  if (!wayland.display) {
    std::cerr << "Failed to obtain wl_display from SDL\n";

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_FAILURE;
  }

  if (!wayland.surface) {
    std::cerr << "Failed to obtain wl_surface from SDL\n";

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_FAILURE;
  }

  // --------------------------------------------------------
  // Wayland registry
  // --------------------------------------------------------

  wayland.registry = wl_display_get_registry(wayland.display);

  wl_registry_add_listener(wayland.registry, &RegistryListener, &wayland);

  /*
   * Wait until the compositor tells us what globals
   * are available.
   */
  if (wl_display_roundtrip(wayland.display) < 0) {
    std::cerr << "wl_display_roundtrip failed\n";

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_FAILURE;
  }

  if (!wayland.layer_shell) {
    std::cerr << "Compositor does not support "
                 "wlr-layer-shell\n";

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_FAILURE;
  }

  // --------------------------------------------------------
  // Create layer-shell surface
  // --------------------------------------------------------

  wayland.layer_surface = zwlr_layer_shell_v1_get_layer_surface(
      wayland.layer_shell,

      wayland.surface,

      /*
       * nullptr means:
       * let the compositor choose the output.
       */
      nullptr,

      /*
       * OVERLAY means it is above normal windows.
       */
      ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,

      "base-template");

  if (!wayland.layer_surface) {
    std::cerr << "Failed to create layer surface\n";

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_FAILURE;
  }

  zwlr_layer_surface_v1_add_listener(wayland.layer_surface,
                                     &LayerSurfaceListener, &wayland);

  // --------------------------------------------------------
  // Configure layer surface
  // --------------------------------------------------------

  /*
   * Anchor to every edge.
   *
   * Because all four edges are anchored, the compositor
   * chooses the width and height.
   */
  zwlr_layer_surface_v1_set_anchor(wayland.layer_surface,
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                       ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                       ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                       ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);

  /*
   * 0 means we do not reserve space.
   *
   * This is an overlay, not a panel that pushes windows
   * away from it.
   */
  zwlr_layer_surface_v1_set_exclusive_zone(wayland.layer_surface, 0);

  /*
   * We want the layer to receive keyboard input.
   *
   * Change this to NONE if this should only be a visual
   * overlay.
   */
  zwlr_layer_surface_v1_set_keyboard_interactivity(
      wayland.layer_surface,
      ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND);

  // --------------------------------------------------------
  // Initial layer-shell commit
  // --------------------------------------------------------

  /*
   * The first commit has no buffer.
   *
   * This asks the compositor to send us the initial
   * configure event.
   */
  wl_surface_commit(wayland.surface);

  if (wl_display_roundtrip(wayland.display) < 0) {
    std::cerr << "Failed waiting for layer-shell configure\n";

    zwlr_layer_surface_v1_destroy(wayland.layer_surface);

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_FAILURE;
  }

  if (!wayland.configured) {
    std::cerr << "Layer surface was not configured\n";

    zwlr_layer_surface_v1_destroy(wayland.layer_surface);

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_FAILURE;
  }

  // --------------------------------------------------------
  // Tell SDL about the compositor-selected size
  // --------------------------------------------------------

  SDL_SetWindowSize(window, static_cast<int>(wayland.width),
                    static_cast<int>(wayland.height));

  // --------------------------------------------------------
  // Dear ImGui
  // --------------------------------------------------------

  float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

  IMGUI_CHECKVERSION();

  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();

  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsLight();

  SetupImGuiStyles();

  ImGuiStyle &style = ImGui::GetStyle();

  style.ScaleAllSizes(main_scale);

  style.FontScaleDpi = main_scale;

  ImGui_ImplSDL3_InitForOpenGL(window, gl_context);

  ImGui_ImplOpenGL3_Init("#version 460");

  // --------------------------------------------------------
  // Main loop
  // --------------------------------------------------------

  bool running = true;

  while (running && !wayland.closed) {

    // ----------------------------------------------------
    // SDL events
    // ----------------------------------------------------

    SDL_Event event;

    while (SDL_PollEvent(&event)) {

      ImGui_ImplSDL3_ProcessEvent(&event);

      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }

      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
          event.window.windowID == SDL_GetWindowID(window)) {
        running = false;
      }
    }

    // ----------------------------------------------------
    // ImGui frame
    // ----------------------------------------------------

    ImGui_ImplOpenGL3_NewFrame();

    ImGui_ImplSDL3_NewFrame();

    ImGui::NewFrame();

    // ----------------------------------------------------
    // UI
    // ----------------------------------------------------

    ImGui::Begin("App!");

    ImGui::Text("This is a transparent layer-shell surface.");

    ImGui::Text("Everything outside this ImGui window "
                "is transparent.");

    if (ImGui::Button("Click!")) {
      std::cout << "Clicked!\n";
    }

    ImGui::End();

    // ----------------------------------------------------
    // Render
    // ----------------------------------------------------

    ImGui::Render();

    int width = 0;
    int height = 0;

    SDL_GetWindowSizeInPixels(window, &width, &height);

    glViewport(0, 0, width, height);

    /*
     * THIS is what makes the background transparent.
     *
     * RGBA = 0,0,0,0
     */
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glClear(GL_COLOR_BUFFER_BIT);

    /*
     * ImGui itself uses alpha blending.
     */
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // ----------------------------------------------------
    // Present
    // ----------------------------------------------------

    SDL_GL_SwapWindow(window);
  }

  // --------------------------------------------------------
  // Cleanup
  // --------------------------------------------------------

  ImGui_ImplOpenGL3_Shutdown();

  ImGui_ImplSDL3_Shutdown();

  ImGui::DestroyContext();

  if (wayland.layer_surface) {
    zwlr_layer_surface_v1_destroy(wayland.layer_surface);

    wayland.layer_surface = nullptr;
  }

  if (wayland.layer_shell) {
    zwlr_layer_shell_v1_destroy(wayland.layer_shell);

    wayland.layer_shell = nullptr;
  }

  if (wayland.registry) {
    wl_registry_destroy(wayland.registry);

    wayland.registry = nullptr;
  }

  SDL_GL_DestroyContext(gl_context);

  SDL_DestroyWindow(window);

  SDL_Quit();

  return EXIT_SUCCESS;
}
