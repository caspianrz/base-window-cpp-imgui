#include <SDL3/SDL_video.h>
#include <imgui.h>

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl3.h>

#include <SDL3/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

/*
#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif
*/

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

struct ApplicationConfig {
  std::string ApplicationName;
  std::uint32_t WindowX;
  std::uint32_t WindowY;
  std::uint32_t WindowWidth;
  std::uint32_t WindowHeight;
  SDL_WindowFlags WindowFlags;
};

const static ApplicationConfig AppConfig = {
    .ApplicationName = "Base Template",
    .WindowX = SDL_WINDOWPOS_CENTERED,
    .WindowY = SDL_WINDOWPOS_CENTERED,
    .WindowWidth = 800,
    .WindowHeight = 600,
    .WindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                   SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY,
};

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

int main(void) {
  // Hint SDL that we are using wayland.
  SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "wayland");

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
    return EXIT_FAILURE;
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8); // For allowing transparency
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

  SDL_Window *window =
      SDL_CreateWindow(AppConfig.ApplicationName.c_str(), AppConfig.WindowWidth,
                       AppConfig.WindowHeight, AppConfig.WindowFlags);

  if (!window) {
    std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
    SDL_Quit();
    return EXIT_FAILURE;
  }

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);

  if (!gl_context) {
    std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << '\n';
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_FAILURE;
  }

  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1); // Enables V-Sync
  SDL_SetWindowPosition(window, AppConfig.WindowX, AppConfig.WindowY);
  SDL_ShowWindow(window);

  // ----------
  // Dear ImGui
  // ----------

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

  // Styles
  ImGui::StyleColorsLight();
  SetupImGuiStyles();
  ImGuiStyle &style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);
  style.FontScaleDpi = main_scale;

  ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init("#version 460");

  // ----

  // ---------
  // Main loop
  // ---------

  bool running = true;

  while (running) {
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

    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // ---
    // UI
    // ---

    ImGui::Begin("App!");

    ImGui::Text("This is a simple ui component to use in here.");

    if (ImGui::Button("Click!")) {
      std::cout << "Clicked!\n";
    }

    ImGui::End();

    // ------
    // Render
    // ------

    ImGui::Render();
    glViewport(0, 0, static_cast<int>(io.DisplaySize.x),
               static_cast<int>(io.DisplaySize.y));
    glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window);
  }

  // -------
  // Cleanup
  // -------

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL3_Shutdown();

  ImGui::DestroyContext();

  SDL_GL_DestroyContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return EXIT_SUCCESS;
}
