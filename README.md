# Hook汉化

这是一个面向 Windows 平台的程序汉化 Hook 框架，主要用于对部分 Qt 应用程序和 DLL 代理程序进行运行时文本替换。

本项目不包含任何商业软件本体、Qt 运行库、目标程序 DLL 或其他专有资源。

## 项目功能

- Qt `QString` 文本替换
- Qt 控件文本汉化
- Qt 菜单、按钮、标签、窗口标题汉化
- Qt 工具提示、状态提示和帮助文本汉化
- Qt HTML、纯文本和富文本内容替换
- Win32 GDI 绘制文本替换
- DLL 代理与导出转发
- UTF-8 字典加载
- 支持中英文文本切换方向的字典匹配
- 支持 HTML 标签、换行和转义字符
- 针对不同程序提供独立适配层

## 当前构建目标

当前项目包含以下构建目标：

- `ida_version`：生成 `version.dll` 代理
- `ida_winmm`：生成 `winmm.dll` 代理
- `knald_hook`：生成 `KnaldHook.dll`
- `world_machine_hook`：生成 `WorldMachineHook.dll`

目前项目中没有 `wininet.dll` 代理实现。如果需要支持 `wininet.dll`，需要另外实现对应的导出转发代码并进行测试。

## 项目目录

```text
common/
    通用编码、字典、日志、Qt 字符串和 Hook 功能

hooks/
    ida/
        通用 Qt Hook 和 Win32 文本 Hook
    launcher/
        可选启动器源码
    worldmachine/
        World Machine 专用字典

proxy/
    version.dll 和 winmm.dll 代理源码

third_party/
    Microsoft Detours 第三方库

CMakeLists.txt
    项目构建配置

LICENSE
    项目许可证

NOTICE.md
    第三方组件和版权说明
```

## 编译要求

- Windows 64 位系统
- Visual Studio C++ 构建工具
- Windows SDK
- CMake 3.20 或更高版本
- Ninja 或 Visual Studio 生成器

建议使用 Visual Studio 的 x64 开发者命令提示符执行编译命令。

## 编译方法

在项目根目录执行：

```
cmake -S . -B build-open -G Ninja "-DHOOK_PROXY_NAMES=version;winmm" -DL10N_BUILD_LAUNCHERS=OFF
cmake --build build-open --target ida_version ida_winmm knald_hook world_machine_hook
```

生成的文件通常位于：

```
build/bin/ida/x64/version.dll
build/bin/ida/x64/winmm.dll
build/bin/ida/x64/KnaldHook.dll
build/bin/worldmachine/x64/WorldMachineHook.dll
```

实际输出位置以 CMake 配置为准。

## 字典格式

字典使用 UTF-8 编码，每行一条记录：

```
原文<TAB>译文
```

示例：

```
Undo Changes	撤销更改
Redo Changes	重做更改
```

原文和译文之间必须使用制表符分隔。

### 多行文本

多行文本必须写在同一行中，不能在字典记录中直接换行。

应使用转义字符：

```
The first line.\r\nThe second line.	第一行。\r\n第二行。
```

支持的常用转义字符包括：

```
\n
\r\n
\t
\\
```

如果一条字典记录被拆成多行，字典加载器会把它们当成不同记录，导致文本无法替换。

### HTML 文本

HTML 内容可以直接保留标签：

```
<b>Checkpoint</b><br/>Organize your network.	<b>检查点</b><br/>整理您的网络。
```

如需保留换行、粗体、斜体或段落结构，应尽量保持原有 HTML 标签和转义形式。

## 字典位置

程序运行时会从 Hook DLL 所在目录的 `Dictionaries` 子目录读取：

```
Dictionaries/translations.txt
```

例如：

```
WorldMachineHook.dll
Dictionaries/
    translations.txt
```

不同应用程序使用各自对应的字典文件，不建议混用不同程序的翻译内容。

## 启动器说明

启动器默认不参与构建。

公开源码中没有包含从商业软件中提取的程序图标。如果需要编译启动器，请准备自己拥有使用权的图标文件，并放置到：

```
hooks/launcher/
```

然后使用：

```
cmake -S . -B build-open -G Ninja -DL10N_BUILD_LAUNCHERS=ON
cmake --build build-open --target KnaldLauncher WorldMachineLauncher
```

## 使用范围

本项目主要用于：

- 软件本地化研究
- 个人汉化
- Qt 文本 Hook 技术研究
- DLL 代理和 API Hook 学习
- 自有软件或获得授权的软件适配

本项目不提供以下内容：

- 商业软件主程序
- 商业软件 DLL
- Qt 运行库
- 破解补丁
- 授权验证绕过功能
- 从目标程序提取的完整资源文件

使用者应自行确认目标软件的许可协议，并承担使用和发布相关汉化文件的责任。

## 已知限制

- 本项目主要针对程序创建或设置文本时进行替换。
- 已经缓存的 Qt 控件文本不一定能够在运行过程中重新翻译。
- 某些自绘界面、特殊 HTML 文档或自定义渲染文本可能无法捕获。
- 不同 Qt 版本、程序版本和编译方式可能需要单独适配。
- 本项目不保证所有程序都能完整汉化。
- 当前不包含运行时全界面中英文即时切换功能。

## 第三方组件

本项目使用 Microsoft Detours 进行 API Hook。

Detours 的源码位于：

```
third_party/Detours/
```

Detours 保留其原始版权声明和许可证，具体内容请查看：

```
third_party/Detours/LICENSE.md
third_party/Detours/README.md
```

## 许可证

本项目原创代码使用 MIT 许可证，具体内容请查看：

```
LICENSE
```

第三方组件和相关翻译数据按照各自的许可证或使用条款执行。

