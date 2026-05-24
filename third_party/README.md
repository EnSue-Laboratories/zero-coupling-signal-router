# third_party

Vendored single-header dependencies (no package manager; keeps the project dependency-light).

- **`stb_image.h`** — used ONLY by `modules/render` for PNG decoding (Task #7, Game Core Agent 3).
  Vendor the official single header here (public domain / MIT). In exactly one TU define
  `#define STB_IMAGE_IMPLEMENTATION` before including it, and configure `STBI_MALLOC`/`STBI_FREE`
  to draw from the render module's cache buffer so there's no global heap growth.
  (Not yet committed — Agent 3 adds it when implementing the renderer.)
