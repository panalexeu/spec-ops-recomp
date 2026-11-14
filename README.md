# specops

Static recompilation of the Xbox 360 version of Spec Ops: The Line (2012) using rexglue-sdk.

This repo is a rexglue-sdk recipe for recompiling and playing your **legally owned and purchased**
Xbox 360 copy of Spec Ops: The Line on PC, with emulated Xbox 360 rendering. It contains no game
code or data: the recompiled C++ is generated locally from your own `default.xex`.

Work in progress: the game compiles, boots and reaches the main menu. Gameplay has not been tested yet.

The setup follows dishonored-recomp, another Unreal Engine 3 title on the same SDK.

### Instructions

Requirements: Linux, clang-20, CMake 3.25+, Ninja, a Vulkan GPU.

1. Clone the repo with its submodules. rexglue-sdk is pinned to upstream commit `bd833a2`
   in `thirdparty/rexglue-sdk`. Then apply the SDK patches:

   ```bash
   git clone --recursive <this repo's URL> spec-ops-recomp
   cd spec-ops-recomp
   tools/apply_patches.sh
   ```

2. Extract your copy of the game (`default.xex` and the `SRGame/` data) into
   `spec-ops-extract/` (gitignored; a symlink to the extracted folder also works). Only the
   `default.xex` with SHA-256
   `9615cdb648b1c67d8636f5ad4efcd33e6b5fbf808a811da0c3e3e7fe5fac8ec7`
   (Title ID 54540843, Media ID 22D4312F, v0.0.0.6, built 2012-05-15) is supported.

   ```text
   spec-ops-recomp/
     spec-ops-extract/
       default.xex
       SRGame/
     thirdparty/rexglue-sdk/
   ```

3. Build. The SDK is built as part of the project. Every build first runs codegen (about 4 minutes;
   it re-runs whenever its inputs change), then compiles the recompiled game (about 10–15 minutes
   the first time on 8 cores).

   ```bash
   cmake --preset linux-amd64-release
   cmake --build --preset linux-amd64-release -j6
   ```

4. Run:

   ```bash
   tools/run.sh
   ```

   The script removes guest memory segments left in `/dev/shm` by crashed runs and selects
   the NVIDIA GPU on hybrid laptops. Runtime settings are in `config/specops.toml`; CMake copies
   it next to the executable on every build. Logs go to `out/build/linux-amd64-release/logs/`.

   The first launches sit on the title screen with a loading spinner for a while: every new shader
   is translated and a Vulkan pipeline is built for it, and the game streams large `CookedXenon`
   packages. It is a slow load, not a hang.

### Changes

rexglue-sdk patches (`patches/`, on top of upstream `bd833a2`), taken unchanged from
dishonored-recomp (`0001`–`0004` originally from singularity-recomp):

| Patch | What was wrong | Source |
|---|---|---|
| `0001` | On GPUs without `VK_EXT_shader_stencil_export` (NVIDIA), stencil copies between render targets never discarded pixels, so stencil saturated at 255 | Xenia Canary `24f90c0ef` |
| `0002` | Texture `exp_adjust` was read from the wrong fetch constant word | Upstream PR #490 |
| `0003` | Depth written by pixel shaders was not remapped for float24 depth buffers; UE3 restores scene depth this way | Xenia Canary `b15fcc73e` |
| `0004` | The achievement popup existed for the whole session, so the window repainted at the monitor rate | — |
| `0005` | Occlusion queries (`EVENT_WRITE_ZPD`) were answered with fake counts or stalling host queries; UE3 then culls visible meshes | Port of Xenia Canary's ZPD occlusion queries (`fbd620c22`, `3d233a5b2`) |

Config (`config/specops.toml`):

| Setting | Why |
|---|---|
| `occlusion_query = "fast"` | Real occlusion queries (patch `0005`) without stalling the CPU |
| `execute_unclipped_draw_vs_on_cpu = true` | Full-screen lighting passes get their real screen extent (UE3 lighting) |
| `readback_memexport = false`, `clear_memory_page_state = false` | The CPU does not wait for the GPU after GPU particle effects |
| `vulkan_allow_present_mode_immediate/mailbox = false` | Presents with vsync instead of redrawing the same frame hundreds of times per second |

### Function overrides

`overrides.toml` declares 167 function entry points the codegen analyzer misses. Without them codegen
fails, or the game stops with "call to invalid or unregistered function" when one is called.

| Count | How it was found |
|---:|---|
| 5 | Tail-call (`b`) targets reported by codegen as `UnresolvedCall: target not in any function` (shared `li r3,0; blr` epilogues and a forwarder) |
| 162 | Code addresses in the game's data sections (mostly vtables and `.pdata`) pointing into gaps between detected functions, found by scanning the loaded image with `tools/find_missing_functions.py` (category `strong`) |

To redo the scan after changing overrides:

```bash
# Dump the loaded (decrypted, decompressed) image and exit. The dump is game data: never commit it.
SPECOPS_DUMP_IMAGE=$PWD/out/image_82000000.bin SPECOPS_DUMP_EXIT=1 tools/run.sh
# 0x82243870-0x82243CF4 holds the SEH scope tables (they point inside functions); exclude it.
python3 tools/find_missing_functions.py out/image_82000000.bin --seh-tables 82243870 82243CF4
# Inspect a candidate (needs llvm-mc-20).
tools/disasm.py out/image_82000000.bin 82EE6974 8
```

Rejected from the scan: `0x8301840C` (a padding word after `blr`), all 395 code-referenced `review`
entries (switch jump-table bases: `lis/addi/lwzx/mtctr/bctr` with the table right after `bctr`) and
all 33 data-referenced `review` entries (coincidental values, mid-function).
