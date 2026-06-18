#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include "main/app_ctx.h"
#include "main/app_gui.h"
#include "utils.h"

static ImFont* g_guifontdefaut = nullptr;
static ImFont* g_guifontlg     = nullptr;

static bool g_guishow_demo     = false;

bool app_gui_demo(app_ctx_t* p_ctx) {

  static bool show_demo_window    = true;
  static bool show_another_window = false;
  // array of 3 float

  ImGuiIO& io = ImGui::GetIO();
  // io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;

  // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
  if (show_demo_window)
    ImGui::ShowDemoWindow(&show_demo_window);

  // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
  if (g_guishow_demo) {
    static float f     = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!",
                 &g_guishow_demo);  // Create a window called "Hello, world!" and append into it.

    ImGui::Text(
        "This is some useful text.");  // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window",
                    &show_demo_window);  // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_another_window);

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);  // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::ColorEdit3("clear color",
                      (float*)&p_ctx->clrscr);  // Edit 3 floats representing a color

    if (ImGui::Button(
            "Button"))  // Buttons return true when clicked (most widgets return true when edited/activated)
      counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate,
                io.Framerate);
    ImGui::End();
  }

  // 3. Show another simple window.
  if (show_another_window) {
    ImGui::Begin(
        "Another Window",
        &show_another_window);  // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::Text("Hello from another window!");
    if (ImGui::Button("Close Me"))
      show_another_window = false;
    ImGui::End();
  }

  return true;
}

void app_guibottom_menu(app_ctx_t* p_ctx) {
  ImGuiViewport* vp = ImGui::GetMainViewport();

  // 1. Define your global scale factor
  float scl = 2.f;
  ANDROID_ONLY({ scl *= 1.5; });

  // 2. Scale the base height of the bar
  float b_ht = 25.0f * scl;
  ANDROID_ONLY({ b_ht *= 1.5; });

  ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y + vp->WorkSize.y - b_ht));
  ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, b_ht));

  ImGuiWindowFlags w_flgs = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;

  // 3. Fetch current style to calculate scaled variations
  ImGuiStyle& st = ImGui::GetStyle();

  // 4. Push scaled layout variables (Symmetry matters here!)
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 0.0f) * scl);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, st.ItemSpacing * scl);

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                      ImVec2(st.FramePadding.x * scl, st.FramePadding.y * scl * 2.5f));

  DESKTOP_ONLY({
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(st.ItemSpacing.x * scl, st.ItemSpacing.y * scl));
  })
  ANDROID_ONLY({
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(st.ItemSpacing.x * 2.0f, st.ItemSpacing.y * scl));
  });

  if (ImGui::Begin("##BottomMenuBar", nullptr, w_flgs)) {
    // 5. Scale the font/text inside the window
    // ImGui::SetWindowFontScale(scl);

    ImGui::PushFont(g_guifontlg);

    if (ImGui::BeginMenuBar()) {

      // Make this menu button on menu bar a bit to the right (important for android)
      // ANDROID_ONLY({
      // });
      // ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (30.0f * scl));

      if (ImGui::BeginMenu("Menu")) {
        if (ImGui::MenuItem("GUI Demo")) {
          g_guishow_demo = !g_guishow_demo;
        }

        if (ImGui::MenuItem("Exit")) { /* Handle click */
        }
        ImGui::EndMenu();
      }

      ImGui::Text("Status: Ready");

      // The right-alignment automatically adapts because CalcTextSize()
      // accounts for the current window font scale!
      float r_st_w = ImGui::CalcTextSize("FPS: 60.0").x;
      ImGui::SameLine(ImGui::GetWindowWidth() - r_st_w - ImGui::GetStyle().ItemSpacing.x * 2);
      ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

      ImGui::EndMenuBar();
    }

    // Reset font scale for safety
    ImGui::PopFont();  // Cleanly restore font back to default
    ImGui::End();
  }

  ImGui::PopStyleVar(5);
}

/*
 * GUI ENTRY POINT
 * */

bool app_guisetup(app_ctx_t* p_ctx) {
  ImGuiIO& io = ImGui::GetIO();
  ImFontConfig cfg;

  // Load your normal default font
  g_guifontdefaut = io.Fonts->AddFontDefault(&cfg);

  // Load an isolated large version (e.g., 26.0f instead of 13.0f)
  g_guifontlg        = io.Fonts->AddFontDefault(&cfg);
  g_guifontlg->Scale = 2.0f;

  // ANDROID_ONLY({ g_guifontlg->Scale = 4.0f; });

  io.Fonts->Build();

  return true;
}
bool app_guimain(app_ctx_t* p_ctx) {

  app_guibottom_menu(p_ctx);

  if (g_guishow_demo)
    app_gui_demo(p_ctx);

  return true;
}

/*
 *
 * GUI MISCS 
 *
 *
 * */
bool app_guiwant_capture_mouse() {
  ImGuiIO& io = ImGui::GetIO();
  return io.WantCaptureMouse;
}
