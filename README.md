<a id="readme-top"></a>
<!--Don't go too crazy for these numbers-->
[![Contributors][contributors-shield]][contributors-url] [![Forks][forks-shield]][forks-url] [![Stargazers][stars-shield]][stars-url] [![Issues][issues-shield]][issues-url] [![Unlicense License][license-shield]][license-url]

<!-- HEADER -->
<br />
<div align="center">
  <!--
  <a href="https://github.com/GruncornTheLesser/Arawn">
    <img src="images/logo.png" alt="Logo" width="80" height="80">
  </a>
  -->
  <h3 align="center">ARAWN</h3>

  <p align="center">
    <a href="https://github.com/GruncornTheLesser/Arawn">View Demo</a>
    &middot;
    <a href="https://github.com/GruncornTheLesser/Arawn/issues/new?labels=bug&template=bug-report---.md">Report Bug</a>
    &middot;
    <a href="https://github.com/GruncornTheLesser/Arawn/issues/new?labels=enhancement&template=feature-request---.md">Request Feature</a>
  </p>
</div>



<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project

![product-screenshot]

##### Built With
[![Cmake][Cmake]][CMake-url] [![Vulkan][Vulkan]][Vulkan-url] [![GLFW][GLFW]][GLFW-url]

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- GETTING STARTED -->
## Getting Started

### Prerequisites
- a c++23 compiler ([GCC](https://gcc.gnu.org/), [Clang](https://clangd.llvm.org/) or [MSVC](https://visualstudio.microsoft.com/vs/features/cplusplus/))
- [Cmake](https://cmake.org/download/)
- [VulkanSDK](https://vulkan.lunarg.com/doc/view/latest/linux/getting_started_ubuntu.html)
###### Ubuntu 24.04 
```sh
> wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc
> sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-noble.list http://packages.lunarg.com/vulkan/lunarg-vulkan-noble.list
> sudo apt update
> sudo apt install vulkan-sdk cmake
```

###### Windows
```sh
> winget install Kitware.CMake
> winget install LunarG.VulkanSDK
```



### Installation
```sh
> git clone https://github.com/GruncornTheLesser/Arawn.git
> cd Arawn
> cmake . -B build
> cmake --build build
```
<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Usage
`cfg\settings.json` defines the configuration of the renderer:
```json
{
    "device" : "NVIDIA GeForce RTX 3060 Laptop GPU",
    "display mode" : "windowed",    // windowed/fullscreen/exclusive
    "aspect ratio" : "4:3",         // "4:3"/"16:9"
    "resolution" : [ 800, 600 ],
    "frame count" : 2,
    "vsync" : true,
    "z prepass" : true,
    "anti alias" : "msaa8",         // none/msaa2/msaa4/msaa8
    "render mode" : "forward",      // forward/deferred
    "culling mode" : "clustered"    // none/clustered/tiled
}
```
- `R` to reload the renderer with the current configuration
- `W` `A` `S` `D` to move the camera
- `+` and `-` to change the resolution
- left click and drag to rotate the camera

###### note: 
changes to device requires the application is restarted. If the the device string does not match, a device is selected automatically.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- MARKDOWN LINKS & IMAGES -->
[contributors-shield]: https://img.shields.io/github/contributors/GruncornTheLesser/Arawn.svg?style=for-the-badge
[contributors-url]: https://github.com/GruncornTheLesser/Arawn/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/GruncornTheLesser/Arawn.svg?style=for-the-badge
[forks-url]: https://github.com/GruncornTheLesser/Arawn/network/members
[stars-shield]: https://img.shields.io/github/stars/GruncornTheLesser/Arawn.svg?style=for-the-badge
[stars-url]: https://github.com/GruncornTheLesser/Arawn/stargazers
[issues-shield]: https://img.shields.io/github/issues/GruncornTheLesser/Arawn.svg?style=for-the-badge
[issues-url]: https://github.com/GruncornTheLesser/Arawn/issues
[license-shield]: https://img.shields.io/github/license/GruncornTheLesser/Arawn.svg?style=for-the-badge
[license-url]: https://github.com/GruncornTheLesser/Arawn/blob/master/LICENSE.txt
[CMake-url]: https://cmake.org/
[CMake]: https://img.shields.io/badge/cmake-004078?style=for-the-badge&logo=cmake&logoColor=White&logoSize=auto&logoWidth=auto
[Vulkan-url]: https://www.vulkan.org/
[Vulkan]: https://img.shields.io/badge/-b40000?style=for-the-badge&logo=Vulkan&logoColor=White&logoSize=auto&logoWidth=auto
[GLFW-url]: https://www.glfw.org/
[GLFW]: https://img.shields.io/badge/GLFW-fb7e13?style=for-the-badge&logo=GLFW&logoColor=White&logoSize=auto&logoWidth=auto
[GLM-url]: https://www.glfw.org/
[product-screenshot]: res/image/screenshot.png