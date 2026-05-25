# third_party

Vendored single-header dependencies (no package manager; keeps the project dependency-light).

- **`stb_image.h`** — committed (public domain / MIT). Used by `modules/render` (Game Core Agent 3,
  PNG decode) and `modules/image` (Engine-ext, multi-format decode → RGBA32). Each module defines
  `#define STB_IMAGE_IMPLEMENTATION` in exactly one of its own TUs and configures
  `STBI_MALLOC`/`STBI_FREE` to bump-allocate from that module's caller buffer (no global heap growth).
  Because two static libs embed the implementation, define **`STB_IMAGE_STATIC`** in each so the
  `stbi_*` symbols are file-local and don't collide when an executable links both modules.

- **`glad/`** — minimal vendored OpenGL 3.3 Core loader surface, used ONLY by `modules/glrender`
  (Engine-ext Agent 1). It exposes the subset of GL entrypoints the fixed renderer needs, keeps the
  single-header + single-source boundary, and is compiled into the glrender module only on backends
  that implement OpenGL loading. No other module may include it.
