# -*- coding: utf-8 -*-
"""家庭信息管理系统 - tkinter图形界面（美化版）。

通过ApiClient与C++后端通信，后端未启动时可一键启动。
"""

import datetime
import os
import subprocess
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

from api import ApiClient, ApiError

# ---------- 配色方案 ----------
BG = "#EEF2F7"          # 窗口背景（浅灰蓝）
CARD = "#FFFFFF"        # 卡片背景
PRIMARY = "#2563EB"     # 主色（蓝）
PRIMARY_DARK = "#1D4ED8"
PRIMARY_LIGHT = "#DBEAFE"
TEXT = "#1E293B"        # 主文字
MUTED = "#64748B"       # 次要文字
BORDER = "#E2E8F0"      # 边框
DANGER = "#DC2626"
DANGER_DARK = "#B91C1C"
SUCCESS = "#16A34A"
HEADER_BG = "#F1F5F9"

FONT = ("Microsoft YaHei UI", 10)
FONT_BOLD = ("Microsoft YaHei UI", 10, "bold")
FONT_TITLE = ("Microsoft YaHei UI", 15, "bold")
FONT_SMALL = ("Microsoft YaHei UI", 9)

SEX_TEXT = {"man": "男", "woman": "女"}
RELATION_TYPES = ["father", "mother", "spouse", "child", "sibling"]
RELATION_TEXT = {"father": "父亲", "mother": "母亲", "spouse": "配偶",
                 "child": "子女", "sibling": "兄弟姐妹"}


def calc_age(birthday):
    """根据生日计算年龄，格式非法时返回None。"""
    try:
        b = datetime.date.fromisoformat(birthday)
        today = datetime.date.today()
        return today.year - b.year - ((today.month, today.day) < (b.month, b.day))
    except ValueError:
        return None


def normalize_birthday(text):
    """将各种常见格式的生日转为YYYY-MM-DD格式（零填充），失败返回None。

    支持：2000-1-5, 2000/1/5, 2000.1.5, 20000105, 2000-01-05
    """
    text = text.strip()
    if not text:
        return None
    # 带分隔符: 2000-1-5 / 2000/1/5 / 2000.1.5
    for sep in ("-", "/", "."):
        if sep in text:
            parts = text.split(sep)
            if len(parts) == 3:
                try:
                    y, m, d = int(parts[0]), int(parts[1]), int(parts[2])
                    datetime.date(y, m, d)  # 验证是否真实日期
                    return "%04d-%02d-%02d" % (y, m, d)
                except (ValueError, IndexError):
                    return None
    # 无分隔符: 20000105
    if len(text) == 8 and text.isdigit():
        try:
            y, m, d = int(text[:4]), int(text[4:6]), int(text[6:8])
            datetime.date(y, m, d)
            return "%04d-%02d-%02d" % (y, m, d)
        except ValueError:
            return None
    return None


def valid_birthday(text):
    """校验生日是否为可识别的合法日期。"""
    return normalize_birthday(text) is not None


def backend_cwd(exe_path):
    """向上查找包含data目录的目录（即command-line目录），作为后端工作目录。"""
    d = os.path.dirname(exe_path)
    while True:
        if os.path.isdir(os.path.join(d, "data")):
            return d
        parent = os.path.dirname(d)
        if parent == d:
            return d
        d = parent


class FamilyInfoApp:
    def __init__(self, api, backend_exe=None, port=8080, smoke=False):
        self.api = api
        self.backend_exe = backend_exe
        self.port = port
        self.smoke = smoke
        self.connected = False
        self._sort_col = None
        self._sort_desc = False
        self._persons = []
        self._all_persons = []  # 始终保存全量成员列表，供关系页下拉框使用
        self._backend_process = None  # 跟踪后端进程，避免重复启动
        self._connect_dialog = None  # 跟踪连接对话框，避免重复弹窗
        self._graph_nodes = {}  # 关系图谱节点坐标缓存 {id: (x1,y1,x2,y2)}

        self.root = tk.Tk()
        self.root.title("家庭信息管理系统")
        self.root.geometry("1000x660")
        self.root.minsize(860, 560)
        self.root.configure(bg=BG)
        self._center_window(1000, 660)

        self._build_styles()
        self._build_ui()

        # 启动后尝试连接后端
        self.root.after(100, self.connect_or_prompt)

        if self.smoke:
            # 自检模式：启动1.5秒后自动关闭，用于验证界面能否正常构建
            self.root.after(1500, lambda: (print("SMOKE OK"), self.root.destroy()))

    # ---------- 窗口与样式 ----------
    def _center_window(self, w, h):
        self.root.update_idletasks()
        x = (self.root.winfo_screenwidth() - w) // 2
        y = (self.root.winfo_screenheight() - h) // 2
        self.root.geometry("%dx%d+%d+%d" % (w, h, x, y))

    def _build_styles(self):
        style = ttk.Style(self.root)
        try:
            style.theme_use("clam")
        except tk.TclError:
            pass
        style.configure(".", font=FONT)
        # 按钮
        style.configure("TButton", padding=(14, 6), background=CARD, foreground=TEXT,
                        bordercolor=BORDER, focuscolor="none")
        style.map("TButton", background=[("active", PRIMARY_LIGHT)])
        style.configure("Primary.TButton", background=PRIMARY, foreground="white", bordercolor=PRIMARY)
        style.map("Primary.TButton", background=[("active", PRIMARY_DARK)], foreground=[("active", "white")])
        style.configure("Danger.TButton", background=DANGER, foreground="white", bordercolor=DANGER)
        style.map("Danger.TButton", background=[("active", DANGER_DARK)], foreground=[("active", "white")])
        # 下拉框
        style.configure("TCombobox", fieldbackground=CARD, background=CARD, foreground=TEXT,
                        arrowcolor=MUTED, bordercolor=BORDER)
        # 表格
        style.configure("Treeview", font=FONT, rowheight=30, background=CARD,
                        fieldbackground=CARD, foreground=TEXT, bordercolor=BORDER)
        style.map("Treeview", background=[("selected", PRIMARY_LIGHT)],
                  foreground=[("selected", PRIMARY_DARK)])
        style.configure("Treeview.Heading", font=FONT_BOLD, background=HEADER_BG,
                        foreground=TEXT, bordercolor=BORDER, relief="flat")
        # 标签页
        style.configure("TNotebook", background=BG, bordercolor=BORDER)
        style.configure("TNotebook.Tab", font=FONT, padding=(20, 8), background=BG, foreground=MUTED)
        style.map("TNotebook.Tab", background=[("selected", CARD)], foreground=[("selected", PRIMARY)])

    # ---------- 界面搭建 ----------
    def _build_ui(self):
        # 顶部横幅
        banner = tk.Frame(self.root, bg=PRIMARY, height=74)
        banner.pack(fill="x")
        banner.pack_propagate(False)
        tk.Label(banner, text="家庭信息管理系统", bg=PRIMARY, fg="white",
                 font=FONT_TITLE).pack(side="left", padx=24, pady=(14, 0))
        tk.Label(banner, text="v1.1.0 · 图形界面", bg=PRIMARY, fg="#BFDBFE",
                 font=FONT_SMALL).pack(side="left", padx=(10, 0), pady=(18, 0))
        self.status_dot = tk.Canvas(banner, width=14, height=14, bg=PRIMARY,
                                    highlightthickness=0)
        self.status_dot.pack(side="right", padx=(0, 10), pady=30)
        self.status_label = tk.Label(banner, text="正在连接后端...", bg=PRIMARY,
                                     fg="white", font=FONT_SMALL)
        self.status_label.pack(side="right", padx=(0, 4), pady=30)

        # 主内容区：三个标签页
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill="both", expand=True, padx=16, pady=(14, 6))
        self.tab_members = tk.Frame(self.notebook, bg=CARD)
        self.tab_relations = tk.Frame(self.notebook, bg=CARD)
        self.tab_graph = tk.Frame(self.notebook, bg=CARD)
        self.notebook.add(self.tab_members, text="  成员列表  ")
        self.notebook.add(self.tab_relations, text="  亲属关系  ")
        self.notebook.add(self.tab_graph, text="  关系图谱  ")
        self.notebook.bind("<<NotebookTabChanged>>", lambda e: self._on_tab_changed())

        self._build_member_tab()
        self._build_relation_tab()
        self._build_graph_tab()

        # 底部状态栏
        statusbar = tk.Frame(self.root, bg=BG)
        statusbar.pack(fill="x", side="bottom", padx=16, pady=(2, 8))
        self.status_msg = tk.Label(statusbar, text="就绪", bg=BG, fg=MUTED,
                                   font=FONT_SMALL, anchor="w")
        self.status_msg.pack(side="left")
        tk.Label(statusbar, text="后端: %s" % self.api.base_url, bg=BG, fg=MUTED,
                 font=FONT_SMALL).pack(side="right")

    def _card(self, parent, padx=14, pady=10):
        """白色卡片容器。"""
        frame = tk.Frame(parent, bg=CARD, highlightbackground=BORDER, highlightthickness=1)
        frame.pack(fill="x", padx=padx, pady=pady)
        return frame

    # ---------- 成员标签页 ----------
    def _build_member_tab(self):
        # 工具栏
        bar = self._card(self.tab_members, padx=10, pady=8)
        ttk.Button(bar, text="＋ 添加成员", style="Primary.TButton",
                   command=self.open_add_dialog).pack(side="left", padx=(4, 6))
        ttk.Button(bar, text="编辑", command=self.open_edit_dialog).pack(side="left", padx=4)
        ttk.Button(bar, text="删除", style="Danger.TButton",
                   command=self.delete_selected).pack(side="left", padx=4)
        ttk.Button(bar, text="刷新", command=self.refresh_all).pack(side="left", padx=4)
        ttk.Button(bar, text="导出CSV", command=self.export_csv).pack(side="left", padx=4)

        tk.Label(bar, text="搜索:", bg=CARD, fg=MUTED, font=FONT).pack(side="left", padx=(24, 4))
        self.search_var = tk.StringVar()
        search_entry = ttk.Entry(bar, textvariable=self.search_var, width=18)
        search_entry.pack(side="left", padx=(0, 4))
        search_entry.bind("<Return>", lambda e: self.refresh_all(search=self.search_var.get()))
        ttk.Button(bar, text="搜索", command=lambda: self.refresh_all(search=self.search_var.get())
                   ).pack(side="left", padx=4)

        # 统计信息条
        stats = self._card(self.tab_members, padx=10, pady=8)
        self.stat_labels = {}
        for key, label in [("total", "共 0 人"), ("man", "男 0"), ("woman", "女 0"),
                           ("avg", "平均年龄 --"), ("oldest", "最年长 --"), ("youngest", "最年幼 --")]:
            lab = tk.Label(stats, text=label, bg=CARD, fg=MUTED, font=FONT)
            lab.pack(side="left", padx=14)
            self.stat_labels[key] = lab

        # 成员表格
        tree_frame = tk.Frame(self.tab_members, bg=CARD)
        tree_frame.pack(fill="both", expand=True, padx=10, pady=(0, 10))
        columns = ("id", "name", "birthday", "age", "sex", "relation_count")
        self.member_tree = ttk.Treeview(tree_frame, columns=columns, show="headings", selectmode="browse")
        headers = {"id": "ID", "name": "姓名", "birthday": "生日", "age": "年龄",
                   "sex": "性别", "relation_count": "关系数"}
        widths = {"id": 70, "name": 160, "birthday": 130, "age": 90, "sex": 90, "relation_count": 90}
        for col in columns:
            self.member_tree.heading(col, text=headers[col],
                                     command=lambda c=col: self._sort_members(c))
            self.member_tree.column(col, width=widths[col], anchor="center")
        # 滚动条
        member_scroll = ttk.Scrollbar(tree_frame, orient="vertical", command=self.member_tree.yview)
        self.member_tree.configure(yscrollcommand=member_scroll.set)
        self.member_tree.pack(side="left", fill="both", expand=True)
        member_scroll.pack(side="right", fill="y")
        # 双击编辑、Delete删除、Enter编辑
        self.member_tree.bind("<Double-1>", lambda e: self.open_edit_dialog())
        self.member_tree.bind("<Delete>", lambda e: self.delete_selected())
        self.member_tree.bind("<Return>", lambda e: self.open_edit_dialog())

    # ---------- 关系标签页 ----------
    def _build_relation_tab(self):
        # 添加关系表单
        form = self._card(self.tab_relations, padx=10, pady=10)
        tk.Label(form, text="成员A", bg=CARD, fg=MUTED, font=FONT).pack(side="left", padx=4)
        self.rel_person_a = ttk.Combobox(form, width=16, state="readonly")
        self.rel_person_a.pack(side="left", padx=(0, 10))
        tk.Label(form, text="关系", bg=CARD, fg=MUTED, font=FONT).pack(side="left", padx=4)
        self.rel_type = ttk.Combobox(form, width=9, state="readonly",
                                     values=list(RELATION_TEXT.values()))
        self.rel_type.pack(side="left", padx=(0, 10))
        tk.Label(form, text="成员B", bg=CARD, fg=MUTED, font=FONT).pack(side="left", padx=4)
        self.rel_person_b = ttk.Combobox(form, width=16, state="readonly")
        self.rel_person_b.pack(side="left", padx=(0, 10))
        ttk.Button(form, text="添加关系", style="Primary.TButton",
                   command=self.add_relation).pack(side="left", padx=6)

        # 关系表格
        tree_frame = tk.Frame(self.tab_relations, bg=CARD)
        tree_frame.pack(fill="both", expand=True, padx=10, pady=(0, 10))
        columns = ("person", "type", "target")
        self.rel_tree = ttk.Treeview(tree_frame, columns=columns, show="headings", selectmode="browse")
        for col, text, width in [("person", "成员", 240), ("type", "关系", 140), ("target", "对方", 240)]:
            self.rel_tree.heading(col, text=text)
            self.rel_tree.column(col, width=width, anchor="center")
        # 滚动条
        rel_scroll = ttk.Scrollbar(tree_frame, orient="vertical", command=self.rel_tree.yview)
        self.rel_tree.configure(yscrollcommand=rel_scroll.set)
        self.rel_tree.pack(side="left", fill="both", expand=True)
        rel_scroll.pack(side="right", fill="y")
        # Delete键删除关系
        self.rel_tree.bind("<Delete>", lambda e: self.delete_relation())
        ttk.Button(tree_frame, text="删除选中关系", style="Danger.TButton",
                   command=self.delete_relation).pack(pady=8)

    # ---------- 关系图谱标签页 ----------
    def _build_graph_tab(self):
        bar = self._card(self.tab_graph, padx=10, pady=8)
        ttk.Button(bar, text="刷新图谱", command=self._refresh_graph).pack(side="left", padx=4)
        tk.Label(bar, text="提示：滚动条平移查看，单击节点查看详情", bg=CARD, fg=MUTED,
                 font=FONT_SMALL).pack(side="left", padx=12)

        frame = tk.Frame(self.tab_graph, bg=CARD)
        frame.pack(fill="both", expand=True, padx=10, pady=(0, 10))
        self.graph_canvas = tk.Canvas(frame, bg="white", highlightthickness=1,
                                      highlightbackground=BORDER)
        vbar = ttk.Scrollbar(frame, orient="vertical", command=self.graph_canvas.yview)
        hbar = ttk.Scrollbar(frame, orient="horizontal", command=self.graph_canvas.xview)
        self.graph_canvas.configure(yscrollcommand=vbar.set, xscrollcommand=hbar.set)
        self.graph_canvas.grid(row=0, column=0, sticky="nsew")
        vbar.grid(row=0, column=1, sticky="ns")
        hbar.grid(row=1, column=0, sticky="ew")
        frame.rowconfigure(0, weight=1)
        frame.columnconfigure(0, weight=1)
        self.graph_canvas.bind("<Button-1>", self._on_graph_click)

    def _on_tab_changed(self):
        """切换到关系图谱标签页时自动刷新。"""
        try:
            current = self.notebook.index(self.notebook.select())
        except tk.TclError:
            return
        if current == 2:  # 关系图谱是第三个标签页
            self._refresh_graph()

    def _refresh_graph(self):
        """拉取全量成员后重新绘制关系图谱。"""
        try:
            self._all_persons = self.api.list_persons()
        except ApiError:
            pass
        self._draw_graph()

    def _draw_graph(self):
        """在Canvas上绘制家族关系树。"""
        canvas = self.graph_canvas
        canvas.delete("all")
        persons = self._all_persons
        if not persons:
            canvas.create_text(320, 200, text="暂无成员数据，请先添加成员并建立关系",
                               fill=MUTED, font=FONT)
            canvas.configure(scrollregion=(0, 0, 640, 400))
            return

        # 1. 构建关系图
        parents = {p["id"]: set() for p in persons}
        children = {p["id"]: set() for p in persons}
        spouses = []
        for p in persons:
            for r in p.get("relations", []):
                t = r["type"]
                target = r["target_id"]
                if t in ("father", "mother", "parent"):
                    parents.setdefault(target, set()).add(p["id"])
                    children[p["id"]].add(target)
                elif t == "child":
                    parents[p["id"]].add(target)
                    children.setdefault(target, set()).add(p["id"])
                elif t == "spouse":
                    spouses.append((p["id"], target))

        # 2. 计算层级（辈分）：无父母者为第0层，子女+1，配偶同层
        level = {}
        visited = set()
        queue = []
        roots = [p["id"] for p in persons if not parents[p["id"]]]
        if not roots:
            roots = [p["id"] for p in persons]
        for r in roots:
            if r not in visited:
                level[r] = 0
                visited.add(r)
                queue.append(r)
        while queue:
            cur = queue.pop(0)
            lv = level[cur]
            for c in children[cur]:
                if c not in visited:
                    level[c] = lv + 1
                    visited.add(c)
                    queue.append(c)
            for a, b in spouses:
                if a == cur and b not in visited:
                    level[b] = lv
                    visited.add(b)
                    queue.append(b)
                elif b == cur and a not in visited:
                    level[a] = lv
                    visited.add(a)
                    queue.append(a)
        for p in persons:
            if p["id"] not in level:
                level[p["id"]] = 0

        # 3. 布局：同层横向排布，子女尽量排在父母下方
        NODE_W, NODE_H, GAP_X, GAP_Y = 120, 52, 170, 120
        by_level = {}
        for p in persons:
            by_level.setdefault(level[p["id"]], []).append(p)
        pos = {}

        def parent_x(p):
            xs = [pos[pid][0] + NODE_W / 2 for pid in parents[p["id"]] if pid in pos]
            if not xs:
                return None
            return sum(xs) / len(xs)

        for lv in sorted(by_level):
            people = by_level[lv]
            people.sort(key=lambda p: (parent_x(p) is None,
                                       parent_x(p) if parent_x(p) is not None else p["id"]))
            n = len(people)
            total_w = (n - 1) * GAP_X
            start_x = -total_w / 2
            for i, p in enumerate(people):
                pos[p["id"]] = (start_x + i * GAP_X, lv * GAP_Y)

        # 4. 画连线（父母-子女为灰线，配偶为橙色虚线）
        seen = set()
        for p in persons:
            for par in parents[p["id"]]:
                key = (par, p["id"])
                if key in seen:
                    continue
                seen.add(key)
                a, b = pos[par], pos[p["id"]]
                canvas.create_line(a[0] + NODE_W / 2, a[1] + NODE_H,
                                   b[0] + NODE_W / 2, b[1], fill="#94A3B8", width=2)
        for a_id, b_id in spouses:
            a, b = pos[a_id], pos[b_id]
            y = a[1] + NODE_H / 2
            canvas.create_line(min(a[0], b[0]) + NODE_W, y, max(a[0], b[0]), y,
                               fill="#F59E0B", width=2, dash=(5, 4))

        # 5. 画节点（男蓝女粉）
        self._graph_nodes = {}
        for p in persons:
            x, y = pos[p["id"]]
            is_man = p.get("sex") == "man"
            fill = "#EFF6FF" if is_man else "#FDF2F8"
            outline = "#2563EB" if is_man else "#DB2777"
            tag = "n%d" % p["id"]
            canvas.create_rectangle(x, y, x + NODE_W, y + NODE_H, fill=fill,
                                    outline=outline, width=2, tags=("node", tag))
            canvas.create_text(x + NODE_W / 2, y + 20, text=p["name"], fill=TEXT,
                               font=FONT_BOLD, tags=("node", tag))
            age = calc_age(p.get("birthday", ""))
            if age is not None:
                sub = "%d岁 · %s" % (age, SEX_TEXT.get(p.get("sex"), "?"))
            else:
                sub = SEX_TEXT.get(p.get("sex"), "?")
            canvas.create_text(x + NODE_W / 2, y + 38, text=sub, fill=MUTED,
                               font=FONT_SMALL, tags=("node", tag))
            self._graph_nodes[p["id"]] = (x, y, x + NODE_W, y + NODE_H)

        # 6. 设置滚动区域（含边距）
        xs = [pos[p["id"]][0] for p in persons]
        ys = [pos[p["id"]][1] for p in persons]
        margin = 80
        canvas.configure(scrollregion=(
            min(xs) - margin, min(ys) - margin,
            max(xs) + NODE_W + margin, max(ys) + NODE_H + margin))

    def _on_graph_click(self, event):
        """单击图谱节点，在状态栏显示该成员详情。"""
        canvas = self.graph_canvas
        x = canvas.canvasx(event.x)
        y = canvas.canvasy(event.y)
        for item in canvas.find_overlapping(x, y, x, y):
            for tag in canvas.gettags(item):
                if tag.startswith("n") and tag[1:].isdigit():
                    pid = int(tag[1:])
                    p = next((q for q in self._all_persons if q["id"] == pid), None)
                    if p:
                        age = calc_age(p.get("birthday", ""))
                        age_text = ("%d岁" % age) if age is not None else "?"
                        self.set_status("%s · %s · 生日 %s · %s" % (
                            p["name"], SEX_TEXT.get(p.get("sex"), "?"),
                            p["birthday"], age_text))
                    return

    # ---------- 后端连接 ----------
    def connect_or_prompt(self):
        """尝试连接后端，失败则弹窗（防止重复弹窗）。"""
        try:
            self.api.list_persons()
            self._set_connected(True)
            self.refresh_all()
        except ApiError as e:
            self._set_connected(False)
            if self.smoke:
                self.set_status("自检：后端未连接（%s）" % e)
                return
            # 防止重复弹窗
            if self._connect_dialog is not None:
                return
            self.show_connect_dialog(str(e))

    def show_connect_dialog(self, reason):
        """弹出连接失败对话框，含启动后端/重试/退出按钮。"""
        if self._connect_dialog is not None:
            return  # 防止重复弹窗
        dlg = tk.Toplevel(self.root)
        self._connect_dialog = dlg
        dlg.title("无法连接后端")
        dlg.configure(bg=CARD)
        dlg.transient(self.root)
        dlg.grab_set()
        dlg.protocol("WM_DELETE_WINDOW", lambda: self._dismiss_connect_dialog())
        w, h = 420, 190
        x = (dlg.winfo_screenwidth() - w) // 2
        y = (dlg.winfo_screenheight() - h) // 2
        dlg.geometry("%dx%d+%d+%d" % (w, h, x, y))
        dlg.resizable(False, False)

        tk.Label(dlg, text="无法连接后端服务", bg=CARD, fg=TEXT,
                 font=FONT_BOLD).pack(pady=(18, 6))
        tk.Label(dlg, text=reason, bg=CARD, fg=MUTED, font=FONT_SMALL,
                 wraplength=360).pack(pady=(0, 4))
        tk.Label(dlg, text="启动方式: command-line.exe --server %d" % self.port,
                 bg=CARD, fg=MUTED, font=FONT_SMALL).pack(pady=(0, 10))

        def start_backend():
            self._dismiss_connect_dialog()
            self.start_backend()

        def retry():
            self._dismiss_connect_dialog()
            self.connect_or_prompt()

        btns = tk.Frame(dlg, bg=CARD)
        btns.pack(pady=8)
        ttk.Button(btns, text="启动后端", style="Primary.TButton",
                   command=start_backend).pack(side="left", padx=6)
        ttk.Button(btns, text="重试", command=retry).pack(side="left", padx=6)
        ttk.Button(btns, text="退出", command=self.root.destroy).pack(side="left", padx=6)

    def _dismiss_connect_dialog(self):
        """关闭并清理连接对话框引用。"""
        if self._connect_dialog is not None:
            self._connect_dialog.destroy()
            self._connect_dialog = None

    def find_backend_exe(self):
        if self.backend_exe and os.path.isfile(self.backend_exe):
            return self.backend_exe
        here = os.path.dirname(os.path.abspath(__file__))
        candidates = [
            os.path.join(here, "..", "command-line", "x64", "Debug", "command-line.exe"),
            os.path.join(here, "..", "command-line", "x64", "Release", "command-line.exe"),
            os.path.join(here, "..", "command-line", "command-line.exe"),
        ]
        for c in candidates:
            if os.path.isfile(c):
                return c
        return None

    def start_backend(self):
        """启动后端exe，启动后自动多次重试连接。"""
        # 如果已经启动过且进程仍在运行，直接重试连接
        if self._backend_process is not None and self._backend_process.poll() is None:
            self.set_status("后端已在运行中，等待连接...")
            self._retry_connect(0)
            return
        self._backend_process = None

        exe = self.find_backend_exe()
        if not exe:
            messagebox.showerror("未找到后端", "没有找到 command-line.exe。\n"
                                 "请先编译项目，或通过 --backend 参数指定路径。")
            return
        try:
            self._backend_process = subprocess.Popen(
                [exe, "--server", str(self.port)],
                cwd=backend_cwd(exe))
        except Exception as e:
            messagebox.showerror("启动失败", "启动后端失败: %s" % e)
            return
        self.set_status("正在启动后端...")
        self._retry_connect(0)

    def _retry_connect(self, attempt):
        """多次重试连接后端，间隔递增（0.5s / 1s / 2s / 3s）。"""
        try:
            self.api.list_persons()
            self._set_connected(True)
            self.refresh_all()
            self.set_status("后端已连接")
            return
        except ApiError:
            pass
        delays = [500, 1000, 2000, 3000]
        if attempt < len(delays):
            self.set_status("等待后端启动... (%d/%d)" % (attempt + 1, len(delays)))
            self.root.after(delays[attempt], lambda: self._retry_connect(attempt + 1))
        else:
            self.set_status("后端启动超时，请确认端口 %d 未被占用" % self.port)
            self._backend_process = None
            self.show_connect_dialog("后端启动超时，请确认端口 %d 未被占用" % self.port)

    # ---------- 数据刷新 ----------
    def refresh_all(self, search=None):
        try:
            if search:
                self._persons = self.api.search(search)
                self.set_status("搜索结果 %d 条" % len(self._persons))
            else:
                self._persons = self.api.list_persons()
                self._all_persons = self._persons  # 全量列表，供关系页下拉框使用
                self.set_status("已刷新")
            self._refresh_member_tree()
            self._refresh_stats()
            self._refresh_relation_combos()
            self._refresh_relation_tree()
            self._draw_graph()
        except ApiError as e:
            self._set_connected(False)
            self.set_status("刷新失败: %s" % e)

    def _refresh_member_tree(self):
        tree = self.member_tree
        tree.delete(*tree.get_children())
        try:
            relations = self.api.list_relations()
        except ApiError:
            relations = []
        count_map = {}
        for rel in relations:
            count_map[rel["person_id"]] = count_map.get(rel["person_id"], 0) + 1
        for p in self._persons:
            age = calc_age(p.get("birthday", ""))
            age_text = "?" if age is None else str(age)
            tree.insert("", "end", iid=str(p["id"]), values=(
                p["id"], p["name"], p["birthday"], age_text,
                SEX_TEXT.get(p.get("sex"), "?"), count_map.get(p["id"], 0)))
        self._apply_sort()

    def _apply_sort(self):
        if not self._sort_col:
            return
        items = list(self.member_tree.get_children(""))
        col = self._sort_col

        def key(iid):
            vals = self.member_tree.item(iid, "values")
            idx = {"id": 0, "name": 1, "birthday": 2, "age": 3, "sex": 4, "relation_count": 5}[col]
            v = vals[idx]
            if col in ("id", "age", "relation_count"):
                try:
                    return int(v)
                except ValueError:
                    return 0
            return v

        items.sort(key=key, reverse=self._sort_desc)
        for i, iid in enumerate(items):
            self.member_tree.move(iid, "", i)

    def _sort_members(self, col):
        if self._sort_col == col:
            self._sort_desc = not self._sort_desc
        else:
            self._sort_col = col
            self._sort_desc = False
        self._apply_sort()

    def _refresh_stats(self):
        try:
            s = self.api.stats()
        except ApiError:
            return
        total = s.get("total", 0)
        self.stat_labels["total"].config(text="共 %d 人" % total)
        self.stat_labels["man"].config(text="男 %d" % s.get("man", 0))
        self.stat_labels["woman"].config(text="女 %d" % s.get("woman", 0))
        if total > 0:
            oldest = s.get("oldest", {})
            youngest = s.get("youngest", {})
            self.stat_labels["avg"].config(text="平均年龄 %.1f" % s.get("avg_age", 0))
            self.stat_labels["oldest"].config(
                text="最年长 %s(%d)" % (oldest.get("name", "--"), oldest.get("age", 0)))
            self.stat_labels["youngest"].config(
                text="最年幼 %s(%d)" % (youngest.get("name", "--"), youngest.get("age", 0)))
        else:
            self.stat_labels["avg"].config(text="平均年龄 --")
            self.stat_labels["oldest"].config(text="最年长 --")
            self.stat_labels["youngest"].config(text="最年幼 --")

    def _person_choices(self):
        return ["%d  %s" % (p["id"], p["name"]) for p in self._all_persons]

    def _refresh_relation_combos(self):
        choices = self._person_choices()
        for box in (self.rel_person_a, self.rel_person_b):
            current = box.get()
            box["values"] = choices
            if current in choices:
                box.set(current)
            elif choices:
                box.set(choices[0])
            else:
                box.set("")

    def _refresh_relation_tree(self):
        tree = self.rel_tree
        tree.delete(*tree.get_children())
        try:
            relations = self.api.list_relations()
        except ApiError:
            relations = []
        for rel in relations:
            tree.insert("", "end", values=(
                "%s (ID %d)" % (rel["person_name"], rel["person_id"]),
                rel["type_name"],
                "%s (ID %d)" % (rel["target_name"], rel["target_id"])))

    # ---------- 成员操作 ----------
    def _selected_person(self):
        sel = self.member_tree.selection()
        if not sel:
            return None
        pid = int(sel[0])
        for p in self._persons:
            if p["id"] == pid:
                return p
        return None

    def open_add_dialog(self):
        self._person_dialog(None)

    def open_edit_dialog(self):
        p = self._selected_person()
        if not p:
            messagebox.showinfo("提示", "请先在列表中选择一个成员。")
            return
        self._person_dialog(p)

    def _person_dialog(self, person):
        is_edit = person is not None
        dlg = tk.Toplevel(self.root)
        dlg.title("编辑成员" if is_edit else "添加成员")
        dlg.configure(bg=CARD)
        dlg.transient(self.root)
        dlg.grab_set()
        w, h = 380, 280
        x = (dlg.winfo_screenwidth() - w) // 2
        y = (dlg.winfo_screenheight() - h) // 2
        dlg.geometry("%dx%d+%d+%d" % (w, h, x, y))
        dlg.resizable(False, False)

        name_var = tk.StringVar(value=person["name"] if is_edit else "")
        birth_var = tk.StringVar(value=person["birthday"] if is_edit else "")
        sex_var = tk.StringVar(value=SEX_TEXT.get(person.get("sex"), "男") if is_edit else "男")

        # 使用grid布局，标签和输入框对齐整齐
        form = tk.Frame(dlg, bg=CARD)
        form.pack(fill="both", expand=True, padx=24, pady=(18, 6))

        tk.Label(form, text="姓名:", bg=CARD, fg=MUTED, font=FONT, width=6,
                 anchor="e").grid(row=0, column=0, padx=(0, 8), pady=10, sticky="e")
        ttk.Entry(form, textvariable=name_var, font=FONT, width=28).grid(
            row=0, column=1, sticky="ew", pady=10)

        tk.Label(form, text="生日:", bg=CARD, fg=MUTED, font=FONT, width=6,
                 anchor="e").grid(row=1, column=0, padx=(0, 8), pady=10, sticky="e")
        ttk.Entry(form, textvariable=birth_var, font=FONT, width=28).grid(
            row=1, column=1, sticky="ew", pady=10)

        tk.Label(form, text="性别:", bg=CARD, fg=MUTED, font=FONT, width=6,
                 anchor="e").grid(row=2, column=0, padx=(0, 8), pady=10, sticky="e")
        sex_frame = tk.Frame(form, bg=CARD)
        sex_frame.grid(row=2, column=1, sticky="w", pady=10)
        for text, val in (("男", "男"), ("女", "女")):
            tk.Radiobutton(sex_frame, text=text, variable=sex_var, value=val,
                           bg=CARD, fg=TEXT, font=FONT, selectcolor=CARD,
                           activebackground=CARD).pack(side="left", padx=(0, 14))

        form.columnconfigure(1, weight=1)

        err = tk.Label(form, text="", bg=CARD, fg=DANGER, font=FONT_SMALL)
        err.grid(row=3, column=0, columnspan=2, pady=(6, 0), sticky="w")

        def save():
            name = name_var.get().strip()
            birthday_raw = birth_var.get().strip()
            if not name:
                err.config(text="姓名不能为空")
                return
            birthday = normalize_birthday(birthday_raw)
            if birthday is None:
                err.config(text="生日格式不正确，请尝试 YYYY-MM-DD / YYYY/MM/DD / YYYYMMDD")
                return
            sex = "man" if sex_var.get() == "男" else "woman"
            try:
                if is_edit:
                    self.api.update_person(person["id"], name=name, birthday=birthday, sex=sex)
                    self.set_status("成员已更新: %s" % name)
                else:
                    created = self.api.add_person(name, birthday, sex)
                    self.set_status("成员已添加: %s (ID %d)" % (name, created["id"]))
            except ApiError as e:
                err.config(text=str(e))
                return
            dlg.destroy()
            self.refresh_all()

        btns = tk.Frame(dlg, bg=CARD)
        btns.pack(pady=(0, 14))
        ttk.Button(btns, text="保存", style="Primary.TButton", command=save).pack(side="left", padx=6)
        ttk.Button(btns, text="取消", command=dlg.destroy).pack(side="left", padx=6)

    def delete_selected(self):
        p = self._selected_person()
        if not p:
            messagebox.showinfo("提示", "请先在列表中选择一个成员。")
            return
        if not messagebox.askyesno("确认删除",
                                   "确定要删除成员 %s (ID %d) 吗？\n其亲属关系也会一并清理。" % (p["name"], p["id"])):
            return
        try:
            self.api.delete_person(p["id"])
            self.set_status("成员已删除: %s" % p["name"])
            self.refresh_all()
        except ApiError as e:
            messagebox.showerror("删除失败", str(e))

    def export_csv(self):
        try:
            csv_text = self.api.export_csv()
        except ApiError as e:
            messagebox.showerror("导出失败", str(e))
            return
        default_name = "family_data_%s.csv" % datetime.date.today().strftime("%Y%m%d")
        path = filedialog.asksaveasfilename(
            title="导出CSV", defaultextension=".csv", initialfile=default_name,
            filetypes=[("CSV文件", "*.csv")])
        if not path:
            return
        try:
            with open(path, "w", encoding="utf-8-sig", newline="") as f:
                f.write(csv_text)
            self.set_status("已导出到: %s" % path)
            messagebox.showinfo("导出成功", "已导出到:\n%s" % path)
        except OSError as e:
            messagebox.showerror("导出失败", str(e))

    # ---------- 关系操作 ----------
    def _parse_choice(self, box):
        text = box.get()
        if not text:
            return None
        try:
            return int(text.split()[0])
        except (ValueError, IndexError):
            return None

    def add_relation(self):
        id_a = self._parse_choice(self.rel_person_a)
        id_b = self._parse_choice(self.rel_person_b)
        rel_text = self.rel_type.get()
        if id_a is None or id_b is None or not rel_text:
            messagebox.showinfo("提示", "请选择成员A、关系类型和成员B。")
            return
        rel_type = [k for k, v in RELATION_TEXT.items() if v == rel_text][0]
        try:
            self.api.add_relation(id_a, rel_type, id_b)
            self.set_status("关系已添加")
            self.refresh_all()
        except ApiError as e:
            messagebox.showerror("添加失败", str(e))

    def delete_relation(self):
        sel = self.rel_tree.selection()
        if not sel:
            messagebox.showinfo("提示", "请先在关系列表中选择一条关系。")
            return
        values = self.rel_tree.item(sel[0], "values")
        person_id = int(values[0].rsplit("ID ", 1)[1].rstrip(")"))
        target_id = int(values[2].rsplit("ID ", 1)[1].rstrip(")"))
        rel_type = [k for k, v in RELATION_TEXT.items() if v == values[1]][0]
        if not messagebox.askyesno("确认删除", "确定要删除这条关系吗？"):
            return
        try:
            self.api.delete_relation(person_id, rel_type, target_id)
            self.set_status("关系已删除")
            self.refresh_all()
        except ApiError as e:
            messagebox.showerror("删除失败", str(e))

    # ---------- 状态 ----------
    def _set_connected(self, connected):
        self.connected = connected
        self.status_dot.delete("all")
        color = SUCCESS if connected else DANGER
        self.status_dot.create_oval(2, 2, 12, 12, fill=color, outline="")
        self.status_label.config(text="后端已连接" if connected else "后端未连接")

    def set_status(self, msg):
        self.status_msg.config(text=msg)

    def run(self):
        self.root.mainloop()
