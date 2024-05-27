# SIMD Ray Tracer
This is the reference code base for SIMD: A practical guide There are currently two branches:
- `master`: Contains a basic ray tracer loosely based on Ray Tracing in One Weekend that's optimized with SIMD and multi-threading that will compile for SSE2, AVX2, and WASM
- `SIMD_Article`: A stripped-down version of the codebase as shown in the article

## Building for Windows:
```ps
.\win32\compile.ps1
```
The `.exe` file will show up in `.\build\win32\`

## Building for WASM:
```ps
.\wasm\compile.ps1
```

The project can be run by creating a local server in `.\build\wasm`
