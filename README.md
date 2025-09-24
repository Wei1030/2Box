# 2Box

许多软件会阻止自身多实例运行，这通常是为了简化逻辑或因为业务上并无必要。但在某些场景下，我们仍希望同时启动多个实例以满足特定需求。

2Box 正是一款能够尽可能不影响软件原有功能的前提下实现软件的多实例运行的轻量级工具(:

---
交流社区

https://kook.vip/8z8C9U


# 2Box-cli 程序使用说明

## 简介

提供CLI工具主要为了帮助开发者使用命令行方式使用2Box，注意，其依赖2Box，不能单独使用，且必须与2Box.exe在同一目录。CLI在必要的时候会自动隐式启动2Box

## 使用方法

```bash
2Box-cli.exe <target_program> [program_arguments...]
```

### 参数说明

- **`<target_program>`** (必需) - 要启动的目标程序路径（如果路径带空格，请用双引号包含完整路径）
- **`[program_arguments...]`** (可选) - 透传给目标程序的参数(如果某个参数带空格，也必须使用双引号包含)

### 示例

```bash
# 启动 test.exe，不传递额外参数
2Box-cli.exe test.exe

# 启动 test.exe 并传递参数
2Box-cli.exe test.exe --config config.json --verbose

# 启动路径带空格，或参数带空格
2Box-cli.exe "C:\Program Files\My App\app.exe" --input "file name with spaces.txt"
```

## 退出码说明

程序执行完成后会返回以下退出码：

### 成功代码
- **`0`** - 成功：目标程序正常启动

### 错误代码
- **`-1`** - 启动程序错误：无法启动指定的目标程序
- **`-2`** - 启动 2box.exe 错误：特定的 2box.exe 启动失败
- **`-3`** - 参数错误：未提供必需的参数或参数格式不正确
- **`-4`** - 参数解析错误：命令行参数解析失败

### Windows API 错误代码
- **`正数`** - Windows API 调用失败时返回的系统错误码
    - 这些是标准的 Windows 错误代码，具体含义请参考 [Microsoft 官方文档](https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes)
