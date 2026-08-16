# 家庭信息管理系统

突发奇想做的

我还在小升初的暑假，别骂我

## 目录结构

```
FamilyInfo/
├── command-line/           # C++ 命令行版（数据层 + CLI 菜单 + HTTP 服务器模式）
├── gui-tkinter/            # Python tkinter 图形界面（通过 REST API 接入后端）
│   └── gui-tkinter.pyproj  # VS 的 Python 项目（可单独打开或从解决方案打开）
└── FamilyInfo.slnx         # Visual Studio 解决方案（含 C++ 和 Python 两个项目）
```

## 快速开始

### 命令行版

用 Visual Studio 编译并运行 `command-line.exe`，按菜单操作（添加 / 显示 / 删除 / 编辑 / 备份 / 恢复 / 设置 / 搜索 / 统计 / 导出 / 日志 / 亲属关系）。

### 图形界面

用 VS 打开 `FamilyInfo.slnx`（或单独打开 `gui-tkinter/gui-tkinter.pyproj`），把
`gui-tkinter` 设为启动项目后按 F5 即可运行调试；命令行方式如下：

```bat
:: 1. 启动后端（默认端口 8080）
command-line\x64\Debug\command-line.exe --server 8080

:: 2. 另开终端，启动界面（后端没启动时界面里也可以一键启动）
python gui-tkinter\main.py
```

详细说明见 [gui-tkinter/README.md](gui-tkinter/README.md)。

## 数据文件

- `command-line/data/family_data.json` - 家庭成员数据
- `command-line/data/config.json` - 配置（排序、备份、日志级别等）
- `command-line/data/backup/` - 备份文件
- `command-line/logs/` - 日志
