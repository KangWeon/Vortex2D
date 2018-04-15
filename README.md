[![Build Status](https://vortex2d.cachemiss.xyz:6500/badge/1)](https://vortex2d.cachemiss.xyz:6500/repo/1)
[![Build Status](https://travis-ci.org/mmaldacker/Vortex2D.svg?branch=master)](https://travis-ci.org/mmaldacker/Vortex2D)
[![Build Status](https://ci.appveyor.com/api/projects/status/p7q9aple11yhs1ck?svg=true)](https://ci.appveyor.com/project/mmaldacker/vortex2d)
[![codecov](https://codecov.io/gh/mmaldacker/Vortex2D/branch/master/graph/badge.svg)](https://codecov.io/gh/mmaldacker/Vortex2D)
[![Docs](https://readthedocs.org/projects/vortex2d/badge/?version=latest)](https://vortex2d.readthedocs.io)

# Vortex2D

A C++ real time fluid simulation engine based on the incompressible Navier-Stokes equation.
This implementation is a hybrid method using particles for advection and a grid for solving the incompressible equations.
All computing is done on the GPU with Vulkan using compute shaders.

Documentation can be found at https://vortex2d.readthedocs.io.

## Examples

<p align="middle">
  <img src="https://github.com/mmaldacker/Vortex2D/raw/master/Docs/vortex2d_example1.gif " width="350"/> 
  <img src="https://github.com/mmaldacker/Vortex2D/raw/master/Docs/vortex2d_example2.gif " width="350"/> 
</p>

<p align="middle">
  <img src="https://github.com/mmaldacker/Vortex2D/raw/master/Docs/vortex2d_example3.gif " width="350"/>
  <img src="https://github.com/mmaldacker/Vortex2D/raw/master/Docs/vortex2d_example4.gif " width="350"/> 
</p>

## Quick start

### Linux

Get the LunarG Vulkan SDK, and extract somewhere. Vortex2D requires the `glslc` compiler which can built/installed with the SDK. 
The script `build_tools.sh` takes care of that. In addition, need to set some env vars so cmake can find the SDK: `source setup-env.sh`.

Vortex2D can then be built with cmake as so:

```
mkdir build && cd build
cmake .. -DVORTEX2D_ENABLE_EXAMPLES=On -DVORTEX2D_ENABLE_TESTS=On
make -j 4
```

### Windows

Get the lunarG Vulkan SDK and install. That's it, you can then build Vortex2D with cmake. Start the cmake-gui, select the source and build folder and configure. Select `DVORTEX2D_ENABLE_EXAMPLES` to also build the examples.

## Roadmap

In no particular order:

* Bicubic interpolation
* Strong rigidbody coupling
* CFL number
* Dynamic timestep
* Improved/faster solver
* Fire 