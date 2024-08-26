# riscv-rtthread 发行版

[English](README.md) | **中文** |

## 概述

基于 RISC-V 架构的 RT-Thread 发行版，我们集成了 RT-Thread 内核，保留了 RT-Thread 构建系统，并对RT-Thread 的仓库进行了一些重构工作，让结构更清楚，让开发者更加关注自身产品开发，开箱即用。

### why

1. 为什么要做一个这样的发行版
RT-Thread 官方仓库已支持多种 RISC-V 架构芯片，但 RT-Thread 官方仓库以内核最重点，没有针对性的上层应用。

### how
1. 取消在线 package，采用集成 components 和 device_driver 的方式，所见即所得，当前发行版集成的 components 和 device_driver 是经过实际硬件和项目测试的，也会定期修复存在的 bug。

如果你需要的组件或设备驱动没有集成，依然可以采用 package 方式集成。可在需要的项目 Kconfig 中添加 `source "$PKGS_DIR/Kconfig"` 进行集成，并使用 RT-Thread 默认的 package 下载方式下载。

2. 优化 RT-Thread 构建系统，让开发者更关注应用开发。以应用（examples/applications）为入口，取消 bsp ，以 boards 替代，开发者可在应用中选择编译目标板和组件，完成应用配置。

3. 降低使用者的使用门槛，让首次使用的开发者也可以快速上手。
- 尽量简单的环境搭建是手上的第一步，我们优化了开发环境搭建流程，尽量少的软件包安装。
- 优化了 RT-Thread toolchain 配置方式，自动下载 toolchain，无需手动下载与配置。
- 优化了芯片的打包脚本，让本项目可以跨平台编译，目前支持 Windows x86_64、Linux x86_64/arm64/riscv64、macOS x86_64/riscv64 全系列操作系统。


## 目录

```shell
.
├── applications                    # 应用
├── examples                        # 示例
│   └── get-started                 # 快速上手
├── board                           # 板级代码及配置
├── components                      # 组件
├── docs                            # 文档
├── rt-thread                       # 内核
├── scripts                         # 工具脚本
```

- RT-Thread 内核版本：5.1.0
- 支持开发板

| 厂家 |  芯片  |  开发板 | RISC-V 架构 | 详细介绍 | 
| ----- | ----- | ------------- | ----------------------| ------ | 
| sophgo | cv180x | MilkV Duo | xuantie C906 +C906 双核 | |
| sophgo | sg2002 | MilkV Duo256M | xuantie C906 +C906 双核 | | 
| sophgo | sg2002 | licheeRV-Nano | xuantie C906 +C906 双核 | |


## 环境搭建

本项目支持 Windows x86_64、Linux x86_64/arm64/riscv64、macOS x86_64/riscv64 全系列操作系统支持。

本发行版目前只支持 gcc 编译，对应的 toolchain 会根据当前运行的操作系统和硬件环境自动下载。

### Linux
```shell
$ sudo apt-get update -y
$ sudo apt-get install -y python3 python3-pip scons
$ sudo apt-get install -y u-boot-tools device-tree-compiler
```

### Windows
本项目支持 Powershell、windows cmd、mingw、msys2 等多种环境编译，不支持 RT-Thread env 和 cygwin ，请不要在 env 、cygwin 环境下编译。

1. Powershell、windows cmd、mingw 环境
- 安装 python3
请在 [https://www.python.org/downloads/windows](https://www.python.org/downloads/windows) 下载 python3 安装包，安装时请勾选 `Add python.exe to PATH` 和 `Disable path length limit` 选项。

> 注：请确保安装的 python 版本为 3.8 或以上版本，不支持 python 2.x 版本

- 安装 git 
请在 [https://gitforwindows.org](https://gitforwindows.org) 下载 git 安装包，安装时请勾选 `Git Bash Here` 选项。

安装完成后请将安装路径添加到系统环境变量中，例如：`C:\Program Files\Git\bin`

- 安装 scons 相关工具
在 Powershell 或 windows cmd 或 mingw 命令行中运行以下命令：
```shell
$ pip install scons windows-curses
```

2. msys2 环境
msys2 集成了 `pacman`，可以方便的安装各种软件包，请参考 [msys2 官方文档](https://www.msys2.org) 安装 msys2。

安装完成后，请运行以下命令安装软件包：
```shell
$ pacman -S python3 python3-pip git scons
$ pip install windows-curses
```

> 注意：
- 本项目可在 VSCode 命令行编译，但在 VSCode 命令行中运行 `scons --menucnfig` 功能时，无法使用上/下键移动配置项，可使用 J/K 键替代，系统 Powershell 或 windows cmd 系统终端则可正常使用。

### macOS
请通过 brew 工具安装以下软件包：
```shell
$ brew install scons u-boot-tools dtc
```

## 配置与编译
本发行版应用位于 examples 和 applications 目录下，以 examples/get-started/blink 目录为例。
```shell
$ cd examples/get-started/blink
```

### 项目配置

- 可根据当前运行目标开发板，选择默认配置，config 文件位于 boards/configs 目录下
```shell
$ scons --defconfig=milkv_duo256m_sd_c906_config
```

- 菜单化配置

```shell
$ scons --menuconfig 
```
选中当前需要编译的目标板，然后对 RT-Thread 内核及软件进行配置，配置完成后保存。


### 编译
```shell
$ cd examples/get-started/blink
$ scons -j8
```

编译完成后，在 examples/get-started/blink/output 目录下会生成对应开发板的固件。

## 如何使用
每种不同的开发板烧录/运行方式可能略有不同，具体请参考对应开发板（如 board/sopogo）下的文档。

1. 将固件烧录到开发板中，烧录方式请参考对应开发板的文档。
2. 烧录完成后，复位开发板，即可看到 RT-Thread 启动信息。
```shell
$ Hello RT-Thread!
```

## FAQ
1. Linux/macOS 下提示未安装 u-boot-tools
```shell
$ mkimage not found, please check ...
```
解决方法：为安装 u-boot-tools
```shell
$ sudo apt-get install -y u-boot-tools device-tree-compiler
```
或
```shell
$ brew install u-boot-tools dtc
```


## 其他
1. 我们依然遵循 RT-Thread 官方仓库的代码风格 和 License，代码风格请参考 [RT-Thread 编程风格](https://github.com/RT-Thread/rt-thread/blob/master/documentation/contribution_guide/coding_style_cn.md)。
2. 更多资料可在 [RT-Thread 官方文档中心](https://www.rt-thread.org/document/site) 查阅。
3. 欢迎加入我们，一起完善这个发行版，欢迎提交 issue 或 PR。
3. 欢迎与我联系：flyingcys@163.com
