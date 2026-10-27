# file_simulator

用 C++17 实现的交互式文件系统模拟器：在内存中管理目录、文件内容、时间和权限，并通过快照文件保存、恢复模拟状态。操作对象是模拟文件系统。

## 环境与构建

需要 Git、CMake 3.13+ 和支持 C++17 的编译器。

Windows + MinGW-w64（`g++`、`mingw32-make` 需在 PATH 中）：

```powershell
git clone https://github.com/L1ttle-Q/file_simulator.git
cd file_simulator
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
.\build\file_simulator.exe
```

Linux/macOS：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/file_simulator
```

Visual Studio 可用 `cmake -S . -B build-vs`、`cmake --build build-vs --config Release` 构建，再运行 `build-vs/Release/file_simulator.exe`。历史 `execute.bat` 使用 `make/` 目录；建议采用上述命令以明确构建和运行位置。

## 从创建文件到保存、恢复

启动后在 `>>>` 提示符下逐行输入：

```text
mkdir demo
cd demo
create note
write note hello
append note _world
read note
cd ..
tree
export sample
exit
```

`read note` 应输出 `hello_world`。`export sample` 将整个模拟器保存到**进程工作目录**的 `saved/sample.simsave`，不是模拟目录 `demo` 中。退出时不会自动保存。

在相同工作目录重新启动，再输入：

```text
import sample
y
cd demo
read note
exit
```

导入会提示确认；成功后 `read note` 仍应显示 `hello_world`。导入将替换当前内存状态，应先导出需要保留的状态。

## 命令

| 命令 | 作用 |
| --- | --- |
| `pwd` / `ls` | 当前模拟路径 / 当前目录内容 |
| `tree` / `treeall` | 当前目录树 / 内存分配情况和根目录树 |
| `mkdir name` / `cd name` | 建目录 / 进入直接子目录；支持 `.`、`..` |
| `create name` / `read name` | 建空文件 / 读取内容 |
| `write name data` / `append name data` | 覆写 / 追加内容 |
| `cp source destination` | 将当前目录中的文件复制为新文件 |
| `rename old new` | 重命名当前目录中的文件或目录 |
| `delete name` / `deldir name` | 删除文件 / 目录 |
| `chmod name number` | 设置模拟权限；参数按十进制读取，例如 `448` 对应八进制 `0700` |
| `export name` / `import name` | 保存 / 恢复 `saved/name.simsave`，不需要输入后缀 |
| `exit` | 退出；需要保存时先 `export` |

## 输入限制与排错

- 参数由空白分隔，名称和 `write` / `append` 内容不能含空格。命令使用小写。
- 名称不要使用 `;[]()"\{}` 等保留字符；保持短于 64 字节，整条输入短于 256 字节。
- 模拟内存容量为 32768 字节，默认采用 first fit。`defragment()` 目前是空实现。
- `cd` 按目录层级逐次操作，不能假定支持真实 shell 的任意路径语法。
- `Permission denied` 来自模拟权限；`error: no such file` 导入错误应检查**工作目录下**的 `saved/`。
- `tree` 不执行完整权限检查。快照采用自定义文本格式，不是压缩包；不要将损坏或不可信快照作为健壮性已保证的输入。
- 自动化输入应显式以 `exit` 结束；当前交互循环没有完整处理标准输入 EOF。

## 源码与验证

`include/file_simulator.h` 实现文件树，`include/mem_simulator.h` 实现内存分配，`src/file_save.cpp` 实现快照序列化和恢复，`src/file_simulator.cpp` 负责命令解析。

2026-10-26：Windows、MinGW-w64 GCC 8.1、CMake 3.31 下构建通过；上面的创建、追加、导出及跨进程导入示例通过。尚无自动测试套件，未验证全部错误输入和跨平台运行。

使用 [Apache License 2.0](LICENSE)。
