# Interview-Assistant
Interview Assistant software.

Build instructions:

- Building: cmake --build build --config Debug --target all --
- Deploying: run deploy.py script with optional --config parameter (Release / Debug / RelWithDebInfo / MinSizeRel) and optional --installer parameter to create an installer.
  Example: `python deploy.py --config Debug --installer`

Prerequsites:

- MSVC compiler.
- Ninja build system.
- Python
- Qt 6.8.1