# riscv-rtthread Release Version

**English** | [中文](README_zh.md)

## Overview

This release version is based on the RISC-V architecture, integrating the RT-Thread kernel while retaining the RT-Thread build system. We have also restructured the RT-Thread repository to make the structure clearer and focus developers on their own product development. The components and device drivers integrated in this release version have been tested on actual hardware and projects, and bugs are fixed regularly. Out-of-the-box usage is supported.

### Why

1. Why create such a release version?
The official RT-Thread repository supports multiple RISC-V architecture chips, but the official repository focuses on the kernel and lacks targeted upper-layer applications.

### How

1. Cancel online package management and adopt an integrated components and device_driver approach. The components and device drivers integrated in this release version have been tested and will be regularly fixed.
If the required components or device drivers are not integrated, they can still be integrated using the package approach. You can add `source "$PKGS_DIR/Kconfig"` to the Kconfig file of the required project to integrate it, and use the default RT-Thread package download method.

2. Optimize the RT-Thread build system to allow developers to focus more on application development. The application (examples/applications) is the entry point, and the BSP is canceled, replaced by boards. Developers can select the target board and components to compile in the application, completing the application configuration.

3. Reduce the barrier to entry for users, allowing first-time users to quickly get started.
- Simplifying the environment setup is the first step in hand. We have optimized the process of setting up the development environment, minimizing the number of software packages installed.
- Optimized the RT-Thread toolchain configuration method, automatically downloading the toolchain, eliminating the need for manual download and configuration.
- Optimized the chip packaging script, allowing this project to be cross-platform compiled, currently supporting Windows x86_64, Linux x86_64/arm64/riscv64, macOS x86_64/riscv64 series operating systems.

## Directory

```shell
.
├── applications                    # Applications
├── examples                        # Examples
│   └── get-started                 # Quick Start
├── board                           # Board-level code and configuration
├── components                      # Components
├── docs                            # Documentation
├── rt-thread                       # Kernel
├── scripts                         # Tool Scripts
```

- RT-Thread Kernel Version: 5.1.0
- Supported Development Boards

| Manufacturer | Chip | Development Board | RISC-V Architecture | Detailed Introduction | 
| ----- | ----- | ------------- | ----------------------| ------ | 
| sophgo | cv180x | MilkV Duo | xuantie C906 +C906 Dual-core | |
| sophgo | sg2002 | MilkV Duo256M | xuantie C906 +C906 Dual-core | | 
| sophgo | sg2002 | licheeRV-Nano | xuantie C906 +C906 Dual-core | |


## Environment Setup

This project supports Windows x86_64, Linux x86_64/arm64/riscv64, macOS x86_64/riscv64 series operating systems.

This release version currently only supports gcc compilation, and the corresponding toolchain will be automatically downloaded based on the current operating system and hardware environment.

### Linux
```shell
$ sudo apt-get update -y
$ sudo apt-get install -y python3 python3-pip scons
$ sudo apt-get install -y u-boot-tools device-tree-compiler
```

### Windows
This project supports Powershell, windows cmd, mingw, msys2 for compilation, and does not support RT-Thread env and cygwin. Please do not compile in the env or cygwin environment.

1. Powershell, windows cmd, mingw environment
- Install python3
Please download the python3 installation package from [https://www.python.org/downloads/windows](https://www.python.org/downloads/windows), and check the `Add python.exe to PATH` and `Disable path length limit` options during installation.

> Note: Ensure the installed python version is 3.8 or above, and python 2.x is not supported.

- Install git
Please download the git installation package from [https://gitforwindows.org](https://gitforwindows.org), and check the `Git Bash Here` option during installation.

After installation, add the installation path to the system environment variables, for example: `C:\Program Files\Git\bin`

- Install scons related tools
Run the following command in Powershell or windows cmd or mingw command line:
```shell
$ pip install scons windows-curses
```

2. msys2 environment
msys2 integrates `pacman`, which can conveniently install various software packages. Please refer to the [msys2 official documentation](https://www.msys2.org) to install msys2.

After installation, run the following command to install software packages:
```shell
$ pacman -S python3 python3-pip git scons
$ pip install windows-curses
```

> Note:
- This project can be compiled in the VSCode command line, but the `scons --menuconfig` function cannot be used in the VSCode command line, and the up/down arrow keys cannot be used to move the configuration items. Instead, the J/K keys can be used. The system Powershell or windows cmd terminal can be used normally.

### macOS
Please install the following software packages using the brew tool:
```shell
$ brew install scons u-boot-tools dtc
```

## Configuration and Compilation
Applications in this release version are located in the examples and applications directories. Using the examples/get-started/blink directory as an example:
```shell
$ cd examples/get-started/blink
```

### Project Configuration

- You can select the default configuration based on the current running target development board. The config file is located in the boards/configs directory.
```shell
$ scons --defconfig=milkv_duo256m_sd_c906_config
```

- Menu-based configuration

```shell
$ scons --menuconfig 
```
Select the current target board to compile, then configure the RT-Thread kernel and software. Save the configuration after completion.


### Compilation
```shell
$ cd examples/get-started/blink
$ scons -j8
```

After compilation, firmware for the corresponding development board will be generated in the output directory of examples/get-started/blink.

## How to Use
The method of burning/run for each different development board may vary slightly. Please refer to the documentation for the corresponding development board (e.g., board/sopogo) for details.

1. Burn the firmware to the development board, and refer to the documentation for the corresponding development board for the burning method.
2. After burning, reset the development board, and you will see the RT-Thread startup information.
```shell
$ Hello RT-Thread!
```

## FAQ
1. Linux/macOS prompt "u-boot-tools not installed"
```shell
$ mkimage not found, please check ...
```
Solution: Install u-boot-tools
```shell
$ sudo apt-get install -y u-boot-tools device-tree-compiler
```
or
```shell
$ brew install u-boot-tools dtc
```

## Other
1. We still follow the code style and License of the official RT-Thread repository. Please refer to [RT-Thread Coding Style](https://github.com/RT-Thread/rt-thread/blob/master/documentation/contribution_guide/coding_style_cn.md) for code style.
2. More information can be found in the [RT-Thread Official Documentation Center](https://www.rt-thread.org/document/site).
3. Welcome to join us and help improve this release version. Welcome to submit issues or PRs.
3. Welcome to contact me: flyingcys@163.com
