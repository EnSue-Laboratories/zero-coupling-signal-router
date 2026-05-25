# third_party

Vendored single-header dependencies (no package manager; keeps the project dependency-light).

- **`stb_image.h`** — used ONLY by `modules/render` for PNG decoding (Task #7, Game Core Agent 3).
  Vendor the official single header here (public domain / MIT). In exactly one TU define
  `#define STB_IMAGE_IMPLEMENTATION` before including it, and configure `STBI_MALLOC`/`STBI_FREE`
  to draw from the render module's cache buffer so there's no global heap growth.
  (Not yet committed — Agent 3 adds it when implementing the renderer.)

- **`glad/`** — minimal vendored OpenGL 3.3 Core loader surface, used ONLY by `modules/glrender`
  (Engine-ext Agent 1). It exposes the subset of GL entrypoints the fixed renderer needs, keeps the
  single-header + single-source boundary, and is compiled into the glrender module only on backends
  that implement OpenGL loading. No other module may include it.
