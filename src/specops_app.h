// specops - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <rex/rex_app.h>
#include <rex/runtime.h>
#include <rex/system/xmemory.h>

class SpecOpsApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<SpecOpsApp>(new SpecOpsApp(ctx, "specops",
        PPCImageConfig));
  }

  // Development helper: dump the loaded (decrypted, decompressed) game image so
  // tools/find_missing_functions.py can scan it for function pointers.
  //
  // This cannot live in OnPreSetup: that hook runs before the runtime and guest
  // memory exist. OnPostLoadXexImage is the first point where the image is mapped.
  //
  // Only runs when SPECOPS_DUMP_IMAGE is set to an output path. If
  // SPECOPS_DUMP_EXIT is also set, the process exits right after the dump.
  // The dump is game data: keep it local, never commit or share it.
  void OnPostLoadXexImage() override {
    const char* out_path = std::getenv("SPECOPS_DUMP_IMAGE");
    if (!out_path || !*out_path) return;

    constexpr uint32_t kImageBase = 0x82000000;
    const uint8_t* host = runtime()->memory()->TranslateVirtual(kImageBase);

    // Image size from the PE header the loader placed at the base (little-endian).
    uint32_t size = 0;
    uint32_t e_lfanew = 0;
    if (host[0] == 'M' && host[1] == 'Z') {
      std::memcpy(&e_lfanew, host + 0x3C, 4);
      if (std::memcmp(host + e_lfanew, "PE\0\0", 4) == 0) {
        // SizeOfImage: optional header (PE + 24) + 56.
        std::memcpy(&size, host + e_lfanew + 24 + 56, 4);
      }
    }
    if (size == 0) {
      std::fprintf(stderr, "[dump] no PE header at 0x%08X; not dumping\n", kImageBase);
      return;
    }

    std::FILE* f = std::fopen(out_path, "wb");
    if (!f) {
      std::fprintf(stderr, "[dump] cannot open %s\n", out_path);
      return;
    }
    const size_t written = std::fwrite(host, 1, size, f);
    std::fclose(f);
    std::fprintf(stderr, "[dump] wrote %zu of %u bytes (0x%08X-0x%08X) to %s\n", written, size,
                 kImageBase, kImageBase + size, out_path);

    if (std::getenv("SPECOPS_DUMP_EXIT")) std::_Exit(0);
  }

  // Override virtual hooks for customization:
  // void OnPostInitLogging() override {}
  // void OnPreSetup(rex::RuntimeConfig& config) override {}
  // void OnLoadXexImage(std::string& xex_image) override {}
  // void OnPostSetup() override {}
  // void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {}
  // std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementsOverlay() override;
  // std::unique_ptr<rex::ui::AchievementNotificationDialog>
  // CreateAchievementNotificationDialog() override;
  // void OnShutdown() override {}
  // void OnConfigurePaths(rex::PathConfig& paths) override {}
};
