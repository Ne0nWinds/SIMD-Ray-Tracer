# SIMD Ray Tracer
This is a brute-force ray tracer optimized with SIMD and multi-threading

## Building for Windows:
This Win32 version will only from [this commit](2a18de811a183a9c76aebd0279941832a39032f1) and earlier
```ps
.\win32\compile.ps1
```
The `.exe` file will show up in `.\build\win32\`

## Building for WASM:
```ps
.\wasm\compile.ps1
```

The project can be run by [creating a local server](https://github.com/Ne0nWinds/Basic-File-Server) in `.\build\wasm`
