# 家庭信息管理系统 - 图形界面 (gui-tkinter)

基于 Python 标准库 tkinter 的美化版图形界面，通过 HTTP REST API 接入 C++ 后端（`command-line.exe --server`）。

## 运行前准备

1. 用 Visual Studio 编译后端（x64 Debug/Release），得到 `command-line/x64/Debug/command-line.exe`
2. 安装 Python 3（自带 tkinter，无需安装任何第三方包）

## 用 Visual Studio 打开

本目录已提供 `gui-tkinter.pyproj`（VS 的 Python 项目）：

- 单独打开：VS 里「文件 → 打开 → 项目/解决方案」，选择 `gui-tkinter/gui-tkinter.pyproj`
- 或在解决方案里打开：`FamilyInfo.slnx` 已包含本项目，把 `gui-tkinter` 设为启动项目后按 F5 即可运行调试

需要 VS 装有「Python 开发」工作负载（本机已装好）。

## 启动步骤

### 方式一：手动启动后端，再启动界面

```bat
:: 终端1：启动后端（默认端口 8080）
command-line\x64\Debug\command-line.exe --server 8080

:: 终端2：启动界面
python gui-tkinter\main.py
```

### 方式二：界面里一键启动后端

直接运行 `python gui-tkinter\main.py`，如果后端没启动，会弹出提示框，点「启动后端」即可
（程序会自动查找 `command-line.exe`，也可以手动用 `--backend` 指定路径）。

## 命令行参数

| 参数 | 说明 |
|------|------|
| `--url http://127.0.0.1:8080` | 后端地址，默认 `http://127.0.0.1:8080` |
| `--backend 路径\command-line.exe` | 指定后端 exe 路径（用于一键启动） |
| `--smoke` | 自检模式：启动 1.5 秒后自动关闭，用于验证界面能正常构建 |

## 界面功能

- **成员列表**：ID / 姓名 / 生日 / 年龄 / 性别 / 关系数，点击表头排序，双击行编辑
- **添加 / 编辑成员**：弹窗表单，带生日合法性校验（YYYY-MM-DD 真实日期）
- **删除成员**：二次确认，删除时自动清理其亲属关系
- **搜索**：按姓名关键字，不区分大小写
- **统计条**：总人数、男女人数、平均年龄、最年长 / 最年幼
- **亲属关系页**：下拉选择成员与关系类型，添加 / 删除关系（自动补全反向关系）
- **导出 CSV**：UTF-8 BOM 编码，Excel 可直接打开

## 后端 REST API 速查

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/persons` | 成员列表 |
| POST | `/api/persons` | 新增成员，body: `{"name","birthday","sex"}` |
| GET | `/api/persons/{id}` | 单个成员 |
| PUT | `/api/persons/{id}` | 修改成员，body 可含 `name` / `birthday` / `sex` |
| DELETE | `/api/persons/{id}` | 删除成员（自动清理关系） |
| GET | `/api/relations?person_id=` | 关系列表（可只查某成员） |
| POST | `/api/relations` | 添加关系，body: `{"person_id","type","target_id"}` |
| DELETE | `/api/relations` | 删除关系，body 同上 |
| GET | `/api/stats` | 统计信息 |
| GET | `/api/search?q=` | 按姓名搜索 |
| GET | `/api/export` | 导出 CSV |

`type` 取值：`father` / `mother` / `spouse` / `child` / `sibling`
（`sex` 取值：`man` / `woman`，也接受 `0` / `1`）

## 目录结构

```
gui-tkinter/
├── main.py    # 程序入口（解析参数）
├── api.py     # REST API 客户端（仅标准库 urllib）
└── gui.py     # tkinter 界面（配色、样式、表格、弹窗）
```

## 常见问题

- **提示无法连接后端**：先确认 `command-line.exe --server 8080` 已启动，或在弹窗里点「启动后端」。
- **端口被占用**：换一个端口，例如 `command-line.exe --server 9000`，界面用 `python main.py --url http://127.0.0.1:9000`。
