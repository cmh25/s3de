# s3de

Software 3d renderer for 3ds files.

The renderer, `cmh3d`, is a plain C library with no dependency on Windows. It
draws through one callback, `void pixel(int x, int y, int r, int g, int b)`,
that the host hands it with `Setppixel()`, so it can render into anything: a
window, a file, a terminal. Two hosts are included:

- `s3de`: the Win32 viewer (toolbar, settings dialog, mouse orbit).
- `headless`: a console program that renders a model to a `.ppm` image using
  only the C runtime. It is the smallest possible host and doubles as a check
  that the engine stays independent of the platform.
- `tests`: regression tests that render hand-built scenes and check pixels.

Models are loaded with the bundled lib3ds. Textures must be 24-bit `.bmp`
files in the same folder as the `.3ds` file.

## Building

Open `s3de.sln` in Visual Studio 2022 or later and build the x64 configuration,
or from a developer prompt:

    msbuild s3de.sln -p:Configuration=Release -p:Platform=x64

## Headless host

    x64\Release\headless.exe models\box05.3ds out.ppm

Optional arguments after the output name: shade flags as a hex number (see
`cmh3d/cmh3d/shade.h`), rotation about x and y in degrees, zoom, and a camera
position for a first-person view instead of an orbit. Run it without arguments
for the usage line.

## Tests

    x64\Release\tests.exe

Prints one line per test and exits with the number of failures. The tests
cover texture edge orientation and the exact-zero seam, specular state
leaking between frames, the world-space light, the clipper's vertex count,
binary-mode texture loading, repeated reloads, and bad input handling.

## Writing a host

1. `Setppixel(fn)` with your pixel callback. The engine only calls it with
   `x` and `y` inside the screen; `y` grows upward.
2. `InitializeWorld(filename)` loads the model and sets up cameras and lights.
3. `SetScreenW(w)` and `SetScreenH(h)` whenever the output size changes.
4. Position the camera (see `ProcessInput()` in `s3de/s3de.cpp` or
   `headless/headless.c`) and call `DrawScene(world)` once per frame.
5. The engine never exits the process. `InitializeWorld()` returns NULL and
   `DrawScene()` returns 0 on failure, with the reason in
   `GetLastEngineError()`. After a successful load a non-empty message is a
   warning, such as a texture that could not be read.

![alt text](screenshots/0.png?raw=true "")

![alt text](screenshots/1.png?raw=true "")

![alt text](screenshots/2.png?raw=true "")
