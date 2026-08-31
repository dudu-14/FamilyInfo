/* 家庭信息管理系统 · 网页版逻辑
 * 通过 fetch 调用后端的 REST API（同源，无跨域问题）。
 */

"use strict";

// ---------- 全局状态 ----------
const state = {
    allPersons: [],      // 全量成员列表
    persons: [],         // 当前显示列表（搜索后）
    relations: [],       // 关系列表
    editingId: null,     // 正在编辑的成员ID（null表示新增）
};

// ---------- 工具函数 ----------
function pad(n, len) {
    return String(n).padStart(len, "0");
}

function normalizeBirthday(text) {
    text = String(text).trim();
    if (!text) return null;
    for (const sep of ["-", "/", "."]) {
        if (text.includes(sep)) {
            const parts = text.split(sep);
            if (parts.length === 3) {
                const y = parseInt(parts[0], 10), m = parseInt(parts[1], 10), d = parseInt(parts[2], 10);
                if (isNaN(y) || isNaN(m) || isNaN(d)) return null;
                const dt = new Date(y, m - 1, d);
                if (dt.getFullYear() === y && dt.getMonth() === m - 1 && dt.getDate() === d) {
                    return `${pad(y, 4)}-${pad(m, 2)}-${pad(d, 2)}`;
                }
            }
            return null;
        }
    }
    if (text.length === 8 && /^\d{8}$/.test(text)) {
        const y = parseInt(text.slice(0, 4), 10), m = parseInt(text.slice(4, 6), 10), d = parseInt(text.slice(6, 8), 10);
        const dt = new Date(y, m - 1, d);
        if (dt.getFullYear() === y && dt.getMonth() === m - 1 && dt.getDate() === d) {
            return `${pad(y, 4)}-${pad(m, 2)}-${pad(d, 2)}`;
        }
    }
    return null;
}

function calcAge(birthday) {
    const m = /^(\d{4})-(\d{2})-(\d{2})$/.exec(birthday);
    if (!m) return null;
    const b = new Date(+m[1], +m[2] - 1, +m[3]);
    if (isNaN(b)) return null;
    const today = new Date();
    let age = today.getFullYear() - b.getFullYear();
    if (today.getMonth() < b.getMonth() ||
        (today.getMonth() === b.getMonth() && today.getDate() < b.getDate())) {
        age--;
    }
    return age;
}

const SEX_TEXT = { man: "男", woman: "女" };
const REL_TEXT = { father: "父亲", mother: "母亲", spouse: "配偶", child: "子女", sibling: "兄弟姐妹" };

// ---------- API 封装 ----------
async function api(method, path, body) {
    const opts = { method, headers: {} };
    if (body !== undefined) {
        opts.body = JSON.stringify(body);
        opts.headers["Content-Type"] = "application/json";
    }
    const resp = await fetch(path, opts);
    const text = await resp.text();
    let data;
    try {
        data = JSON.parse(text);
    } catch (e) {
        data = text;
    }
    if (!resp.ok) {
        const msg = (data && data.error) ? data.error : ("请求失败 " + resp.status);
        throw new Error(msg);
    }
    return data;
}

// ---------- 提示与状态 ----------
let toastTimer = null;
function toast(msg) {
    const el = document.getElementById("toast");
    el.textContent = msg;
    el.classList.remove("hidden");
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => el.classList.add("hidden"), 2200);
}

function setConnected(ok) {
    const dot = document.getElementById("status-dot");
    const txt = document.getElementById("status-text");
    dot.className = "dot " + (ok ? "online" : "offline");
    txt.textContent = ok ? "后端已连接" : "后端未连接";
}

// ---------- 数据加载 ----------
async function refreshAll() {
    try {
        state.allPersons = await api("GET", "/api/persons");
        state.persons = state.allPersons;
        state.relations = await api("GET", "/api/relations");
        setConnected(true);
        renderMembers();
        renderStatsBar();
        renderRelationCombos();
        renderRelations();
        renderStats();
        refreshGraph();  // 图谱页激活时同步刷新
    } catch (e) {
        setConnected(false);
        toast("无法连接后端: " + e.message);
    }
}

// ---------- 成员表格 ----------
function renderMembers() {
    const tbody = document.querySelector("#member-table tbody");
    tbody.innerHTML = "";
    const relCount = {};
    state.relations.forEach(r => { relCount[r.person_id] = (relCount[r.person_id] || 0) + 1; });
    state.persons.forEach(p => {
        const age = calcAge(p.birthday);
        const tr = document.createElement("tr");
        const sexCls = p.sex === "man" ? "sex-man" : "sex-woman";
        tr.innerHTML = `
            <td>${p.id}</td>
            <td>${escapeHtml(p.name)}</td>
            <td>${p.birthday}</td>
            <td>${age === null ? "?" : age}</td>
            <td class="${sexCls}">${SEX_TEXT[p.sex] || "?"}</td>
            <td>${relCount[p.id] || 0}</td>
            <td>
                <button class="btn small" data-act="edit" data-id="${p.id}">编辑</button>
                <button class="btn small danger" data-act="del" data-id="${p.id}">删除</button>
            </td>`;
        tbody.appendChild(tr);
    });
    if (!state.persons.length) {
        tbody.innerHTML = `<tr><td colspan="7" style="color:#94A3B8">暂无成员，点击"添加成员"开始。</td></tr>`;
    }
}

function escapeHtml(s) {
    return String(s).replace(/[&<>"']/g, c => ({
        "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
    }[c]));
}

function renderStatsBar() {
    const bar = document.getElementById("stats-bar");
    if (!state.allPersons.length) {
        bar.innerHTML = "暂无成员数据";
        return;
    }
    const man = state.allPersons.filter(p => p.sex === "man").length;
    const woman = state.allPersons.length - man;
    bar.innerHTML = `
        <span>共 <b>${state.allPersons.length}</b> 人</span>
        <span>男 <b>${man}</b></span>
        <span>女 <b>${woman}</b></span>`;
}

// ---------- 关系下拉框 ----------
function renderRelationCombos() {
    for (const id of ["rel-person-a", "rel-person-b"]) {
        const sel = document.getElementById(id);
        const cur = sel.value;
        sel.innerHTML = state.allPersons.map(p =>
            `<option value="${p.id}">${escapeHtml(p.name)} (ID ${p.id})</option>`).join("");
        if (cur) sel.value = cur;
    }
}

// ---------- 关系表格 ----------
function renderRelations() {
    const tbody = document.querySelector("#rel-table tbody");
    tbody.innerHTML = "";
    state.relations.forEach(r => {
        const tr = document.createElement("tr");
        tr.innerHTML = `
            <td>${escapeHtml(r.person_name)} (ID ${r.person_id})</td>
            <td>${REL_TEXT[r.type] || r.type_name || r.type}</td>
            <td>${escapeHtml(r.target_name)} (ID ${r.target_id})</td>
            <td>
                <button class="btn small danger" data-act="delrel"
                    data-person="${r.person_id}" data-type="${r.type}" data-target="${r.target_id}">删除</button>
            </td>`;
        tbody.appendChild(tr);
    });
    if (!state.relations.length) {
        tbody.innerHTML = `<tr><td colspan="4" style="color:#94A3B8">暂无关系数据。</td></tr>`;
    }
}

// ---------- 统计 ----------
async function renderStats() {
    let s;
    try {
        s = await api("GET", "/api/stats");
    } catch (e) {
        return;
    }
    const cards = document.getElementById("stats-cards");
    if (!s.total) {
        cards.innerHTML = `<div class="stat-card"><div class="num">0</div><div class="label">暂无成员</div></div>`;
        return;
    }
    const oldest = s.oldest || {};
    const youngest = s.youngest || {};
    const items = [
        ["总人数", s.total],
        ["男", s.man],
        ["女", s.woman],
        ["平均年龄", (s.avg_age ?? 0).toFixed(1)],
        ["最年长", oldest.name ? `${oldest.name}(${oldest.age})` : "--"],
        ["最年幼", youngest.name ? `${youngest.name}(${youngest.age})` : "--"],
    ];
    cards.innerHTML = items.map(([label, num]) =>
        `<div class="stat-card"><div class="num">${num}</div><div class="label">${label}</div></div>`).join("");
}

// ---------- 成员增删改 ----------
function openAddModal() {
    state.editingId = null;
    document.getElementById("modal-title").textContent = "添加成员";
    document.getElementById("p-name").value = "";
    document.getElementById("p-birthday").value = "";
    document.getElementById("p-sex").value = "man";
    document.getElementById("modal-error").textContent = "";
    document.getElementById("person-modal").classList.remove("hidden");
}

function openEditModal(id) {
    const p = state.allPersons.find(x => x.id === id);
    if (!p) return;
    state.editingId = id;
    document.getElementById("modal-title").textContent = "编辑成员";
    document.getElementById("p-name").value = p.name;
    document.getElementById("p-birthday").value = p.birthday;
    document.getElementById("p-sex").value = p.sex;
    document.getElementById("modal-error").textContent = "";
    document.getElementById("person-modal").classList.remove("hidden");
}

async function savePerson() {
    const err = document.getElementById("modal-error");
    const name = document.getElementById("p-name").value.trim();
    const birthdayRaw = document.getElementById("p-birthday").value.trim();
    const sex = document.getElementById("p-sex").value;
    if (!name) { err.textContent = "姓名不能为空"; return; }
    const birthday = normalizeBirthday(birthdayRaw);
    if (birthday === null) { err.textContent = "生日格式不正确，请尝试 YYYY-MM-DD / YYYY/MM/DD / YYYYMMDD"; return; }
    try {
        if (state.editingId === null) {
            await api("POST", "/api/persons", { name, birthday, sex });
            toast("成员已添加: " + name);
        } else {
            await api("PUT", "/api/persons/" + state.editingId, { name, birthday, sex });
            toast("成员已更新: " + name);
        }
        document.getElementById("person-modal").classList.add("hidden");
        await refreshAll();
    } catch (e) {
        err.textContent = e.message;
    }
}

async function deletePerson(id) {
    const p = state.allPersons.find(x => x.id === id);
    if (!p) return;
    if (!confirm(`确定要删除成员 ${p.name} (ID ${id}) 吗？\n其亲属关系也会一并清理。`)) return;
    try {
        await api("DELETE", "/api/persons/" + id);
        toast("成员已删除: " + p.name);
        await refreshAll();
    } catch (e) {
        toast("删除失败: " + e.message);
    }
}

// ---------- 关系增删 ----------
async function addRelation() {
    const person_id = parseInt(document.getElementById("rel-person-a").value, 10);
    const target_id = parseInt(document.getElementById("rel-person-b").value, 10);
    const type = document.getElementById("rel-type").value;
    if (!person_id || !target_id) { toast("请选择成员"); return; }
    try {
        await api("POST", "/api/relations", { person_id, type, target_id });
        toast("关系已添加");
        await refreshAll();
    } catch (e) {
        toast("添加失败: " + e.message);
    }
}

async function deleteRelation(person_id, type, target_id) {
    if (!confirm("确定要删除这条关系吗？")) return;
    try {
        await api("DELETE", "/api/relations", { person_id, type, target_id });
        toast("关系已删除");
        await refreshAll();
    } catch (e) {
        toast("删除失败: " + e.message);
    }
}

// ---------- 导出CSV ----------
async function exportCsv() {
    try {
        const csv = await api("GET", "/api/export");
        const blob = new Blob(["\uFEFF" + csv], { type: "text/csv;charset=utf-8" });
        const url = URL.createObjectURL(blob);
        const a = document.createElement("a");
        a.href = url;
        a.download = "family_data_" + new Date().toISOString().slice(0, 10) + ".csv";
        a.click();
        URL.revokeObjectURL(url);
        toast("已导出CSV");
    } catch (e) {
        toast("导出失败: " + e.message);
    }
}

// ================= 关系图谱 =================
const graph = {
    byId: {}, parents: {}, children: {}, spouses: [], siblings: [],
    level: {}, pos: {},
    scale: 1, ox: 0, oy: 0,
    dragging: false, lastX: 0, lastY: 0,
    selected: null,
};

const NODE_W = 120, NODE_H = 52, GAP_X = 170, GAP_Y = 120;

function buildGraph() {
    const byId = {};
    state.allPersons.forEach(p => { byId[p.id] = p; });
    const parents = {}, children = {};
    state.allPersons.forEach(p => {
        parents[p.id] = new Set();
        children[p.id] = new Set();
    });
    const spouses = [], siblings = [];
    state.allPersons.forEach(p => {
        (p.relations || []).forEach(r => {
            const t = r.type, target = r.target_id;
            if (t === "father" || t === "mother" || t === "parent") {
                (parents[target] = parents[target] || new Set()).add(p.id);
                children[p.id].add(target);
            } else if (t === "child") {
                parents[p.id].add(target);
                (children[target] = children[target] || new Set()).add(p.id);
            } else if (t === "spouse") {
                spouses.push([p.id, target]);
            } else if (t === "sibling") {
                siblings.push([p.id, target]);
            }
        });
    });
    graph.byId = byId;
    graph.parents = parents;
    graph.children = children;
    graph.spouses = spouses;
    graph.siblings = siblings;
}

function computeLevels() {
    const level = {};
    const visited = new Set();
    const queue = [];
    // 根：没有父母的人
    const roots = state.allPersons.filter(p => graph.parents[p.id].size === 0).map(p => p.id);
    (roots.length ? roots : state.allPersons.map(p => p.id)).forEach(id => {
        if (!visited.has(id)) { level[id] = 0; visited.add(id); queue.push(id); }
    });
    while (queue.length) {
        const id = queue.shift();
        const lv = level[id];
        graph.children[id].forEach(c => {
            if (!visited.has(c)) { level[c] = lv + 1; visited.add(c); queue.push(c); }
        });
        graph.spouses.forEach(([a, b]) => {
            if (a === id && !visited.has(b)) { level[b] = lv; visited.add(b); queue.push(b); }
            else if (b === id && !visited.has(a)) { level[a] = lv; visited.add(a); queue.push(a); }
        });
    }
    state.allPersons.forEach(p => { if (!(p.id in level)) level[p.id] = 0; });
    graph.level = level;
}

function computeLayout() {
    const byLevel = {};
    state.allPersons.forEach(p => {
        const lv = graph.level[p.id];
        (byLevel[lv] = byLevel[lv] || []).push(p);
    });
    const pos = {};
    function parentX(p) {
        const xs = [...graph.parents[p.id]].map(pid => pos[pid]).filter(o => o).map(o => o.x);
        if (!xs.length) return null;
        return xs.reduce((s, x) => s + x, 0) / xs.length;
    }
    Object.keys(byLevel).map(Number).sort((a, b) => a - b).forEach(lv => {
        const people = byLevel[lv];
        people.sort((a, b) => {
            const pa = parentX(a), pb = parentX(b);
            if (pa !== null && pb !== null) return pa - pb;
            if (pa !== null) return -1;
            if (pb !== null) return 1;
            return a.id - b.id;
        });
        const n = people.length;
        const totalWidth = (n - 1) * GAP_X;
        const startX = -totalWidth / 2;
        people.forEach((p, i) => { pos[p.id] = { x: startX + i * GAP_X, y: lv * GAP_Y }; });
    });
    graph.pos = pos;
}

function fitGraph() {
    const c = document.getElementById("graph-canvas");
    if (!state.allPersons.length) { graph.scale = 1; graph.ox = 0; graph.oy = 0; return; }
    // 计算图形边界
    let minX = Infinity, maxX = -Infinity, minY = Infinity, maxY = -Infinity;
    state.allPersons.forEach(p => {
        const o = graph.pos[p.id];
        if (!o) return;
        minX = Math.min(minX, o.x); maxX = Math.max(maxX, o.x + NODE_W);
        minY = Math.min(minY, o.y); maxY = Math.max(maxY, o.y + NODE_H);
    });
    const gW = maxX - minX, gH = maxY - minY;
    const pad = 60;
    const scale = Math.min((c.clientWidth - pad) / gW, (c.clientHeight - pad) / gH, 1.5);
    graph.scale = Math.max(0.2, scale);
    graph.ox = (c.clientWidth - gW * graph.scale) / 2 - minX * graph.scale;
    graph.oy = (c.clientHeight - gH * graph.scale) / 2 - minY * graph.scale;
}

function toWorld(sx, sy) {
    return [(sx - graph.ox) / graph.scale, (sy - graph.oy) / graph.scale];
}

function redrawGraph() {
    const c = document.getElementById("graph-canvas");
    const ctx = c.getContext("2d");
    ctx.clearRect(0, 0, c.width, c.height);
    ctx.fillStyle = "#FFFFFF";
    ctx.fillRect(0, 0, c.width, c.height);

    if (!state.allPersons.length) {
        ctx.fillStyle = "#94A3B8";
        ctx.font = "16px 'Microsoft YaHei'";
        ctx.textAlign = "center";
        ctx.fillText("暂无成员数据，请先添加成员并建立关系", c.width / 2, c.height / 2);
        return;
    }

    ctx.save();
    ctx.translate(graph.ox, graph.oy);
    ctx.scale(graph.scale, graph.scale);

    // 1. 画边（线）
    const seen = new Set();
    // 父母-子女连线
    state.allPersons.forEach(p => {
        graph.parents[p.id].forEach(par => {
            const a = graph.pos[par], b = graph.pos[p.id];
            if (!a || !b) return;
            const key = par + "-" + p.id;
            if (seen.has(key)) return;
            seen.add(key);
            ctx.strokeStyle = "#94A3B8";
            ctx.lineWidth = 1.6 / graph.scale;
            ctx.beginPath();
            ctx.moveTo(a.x + NODE_W / 2, a.y + NODE_H);
            ctx.lineTo(b.x + NODE_W / 2, b.y);
            ctx.stroke();
        });
    });
    // 配偶连线（水平虚线）
    graph.spouses.forEach(([aId, bId]) => {
        const a = graph.pos[aId], b = graph.pos[bId];
        if (!a || !b) return;
        const key = Math.min(aId, bId) + "-" + Math.max(aId, bId);
        if (seen.has(key)) return;
        seen.add(key);
        const y = a.y + NODE_H / 2;
        ctx.strokeStyle = "#F59E0B";
        ctx.lineWidth = 1.4 / graph.scale;
        ctx.setLineDash([5 / graph.scale, 4 / graph.scale]);
        ctx.beginPath();
        ctx.moveTo(Math.min(a.x, b.x) + NODE_W, y);
        ctx.lineTo(Math.max(a.x, b.x), y);
        ctx.stroke();
        ctx.setLineDash([]);
    });

    // 2. 画节点
    state.allPersons.forEach(p => {
        const o = graph.pos[p.id];
        if (!o) return;
        const x = o.x, y = o.y;
        const isSel = graph.selected === p.id;
        const isMan = p.sex === "man";
        const age = calcAge(p.birthday);

        // 圆角矩形
        const r = 10;
        ctx.beginPath();
        ctx.moveTo(x + r, y);
        ctx.arcTo(x + NODE_W, y, x + NODE_W, y + NODE_H, r);
        ctx.arcTo(x + NODE_W, y + NODE_H, x, y + NODE_H, r);
        ctx.arcTo(x, y + NODE_H, x, y, r);
        ctx.arcTo(x, y, x + NODE_W, y, r);
        ctx.closePath();
        ctx.fillStyle = isSel ? "#DBEAFE" : (isMan ? "#EFF6FF" : "#FDF2F8");
        ctx.fill();
        ctx.strokeStyle = isSel ? "#2563EB" : (isMan ? "#2563EB" : "#DB2777");
        ctx.lineWidth = (isSel ? 3 : 2) / graph.scale;
        ctx.stroke();

        // 文本
        ctx.fillStyle = "#1E293B";
        ctx.textAlign = "center";
        ctx.font = "bold 14px 'Microsoft YaHei'";
        ctx.fillText(p.name, x + NODE_W / 2, y + 20);
        ctx.fillStyle = "#64748B";
        ctx.font = "11px 'Microsoft YaHei'";
        ctx.fillText((age === null ? "" : age + "岁 · ") + (SEX_TEXT[p.sex] || "?"), x + NODE_W / 2, y + 38);
    });

    ctx.restore();
}

function setupGraphCanvas() {
    const container = document.getElementById("graph-container");
    const c = document.getElementById("graph-canvas");
    const dpr = window.devicePixelRatio || 1;
    c.width = container.clientWidth * dpr;
    c.height = container.clientHeight * dpr;
    c.style.width = container.clientWidth + "px";
    c.style.height = container.clientHeight + "px";
    const ctx = c.getContext("2d");
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    buildGraph();
    computeLevels();
    computeLayout();
    fitGraph();
    redrawGraph();
}

function refreshGraph() {
    if (document.getElementById("tab-graph").classList.contains("active")) {
        setupGraphCanvas();
    }
}

function hitTest(sx, sy) {
    const [wx, wy] = toWorld(sx, sy);
    for (const p of state.allPersons) {
        const o = graph.pos[p.id];
        if (!o) continue;
        if (wx >= o.x && wx <= o.x + NODE_W && wy >= o.y && wy <= o.y + NODE_H) {
            return p.id;
        }
    }
    return null;
}

function initGraphEvents() {
    const c = document.getElementById("graph-canvas");
    c.addEventListener("mousedown", e => {
        graph.dragging = true;
        graph.lastX = e.clientX;
        graph.lastY = e.clientY;
    });
    c.addEventListener("mousemove", e => {
        if (!graph.dragging) return;
        graph.ox += e.clientX - graph.lastX;
        graph.oy += e.clientY - graph.lastY;
        graph.lastX = e.clientX;
        graph.lastY = e.clientY;
        redrawGraph();
    });
    window.addEventListener("mouseup", () => { graph.dragging = false; });
    c.addEventListener("click", e => {
        const rect = c.getBoundingClientRect();
        const sx = e.clientX - rect.left, sy = e.clientY - rect.top;
        const id = hitTest(sx, sy);
        graph.selected = id;
        redrawGraph();
        if (id !== null) {
            const p = graph.byId[id];
            const age = calcAge(p.birthday);
            toast(`${p.name} · ${SEX_TEXT[p.sex] || "?"} · 生日 ${p.birthday} · ${age === null ? "?" : age + "岁"}`);
        }
    });
    c.addEventListener("wheel", e => {
        e.preventDefault();
        const rect = c.getBoundingClientRect();
        const sx = e.clientX - rect.left, sy = e.clientY - rect.top;
        const [wx, wy] = toWorld(sx, sy);
        const factor = e.deltaY < 0 ? 1.1 : 0.9;
        const ns = Math.max(0.1, Math.min(4, graph.scale * factor));
        graph.scale = ns;
        graph.ox = sx - wx * ns;
        graph.oy = sy - wy * ns;
        redrawGraph();
    }, { passive: false });
}

// ================= 标签页切换 =================
function switchTab(name) {
    document.querySelectorAll(".tab").forEach(t => t.classList.toggle("active", t.dataset.tab === name));
    document.querySelectorAll(".panel").forEach(p => p.classList.toggle("active", p.id === "tab-" + name));
    if (name === "graph") refreshGraph();
    if (name === "stats") renderStats();
}

// ================= 事件绑定 =================
function bindEvents() {
    // 标签页
    document.querySelectorAll(".tab").forEach(t =>
        t.addEventListener("click", () => switchTab(t.dataset.tab)));

    // 成员
    document.getElementById("btn-add").addEventListener("click", openAddModal);
    document.getElementById("btn-refresh").addEventListener("click", () => refreshAll());
    document.getElementById("btn-export").addEventListener("click", exportCsv);
    document.getElementById("btn-search").addEventListener("click", doSearch);
    document.getElementById("search-input").addEventListener("keydown", e => {
        if (e.key === "Enter") doSearch();
    });
    document.querySelector("#member-table tbody").addEventListener("click", e => {
        const btn = e.target.closest("button[data-act]");
        if (!btn) return;
        const id = parseInt(btn.dataset.id, 10);
        if (btn.dataset.act === "edit") openEditModal(id);
        else if (btn.dataset.act === "del") deletePerson(id);
    });

    // 弹窗
    document.getElementById("btn-save").addEventListener("click", savePerson);
    document.getElementById("btn-cancel").addEventListener("click", () =>
        document.getElementById("person-modal").classList.add("hidden"));
    document.getElementById("person-modal").addEventListener("click", e => {
        if (e.target.id === "person-modal") e.target.classList.add("hidden");
    });

    // 关系
    document.getElementById("btn-add-rel").addEventListener("click", addRelation);
    document.querySelector("#rel-table tbody").addEventListener("click", e => {
        const btn = e.target.closest("button[data-act='delrel']");
        if (!btn) return;
        deleteRelation(
            parseInt(btn.dataset.person, 10),
            btn.dataset.type,
            parseInt(btn.dataset.target, 10));
    });

    // 图谱
    document.getElementById("btn-graph-refresh").addEventListener("click", refreshGraph);
    initGraphEvents();
}

function doSearch() {
    const kw = document.getElementById("search-input").value.trim();
    if (!kw) { state.persons = state.allPersons; }
    else {
        state.persons = state.allPersons.filter(p => p.name.toLowerCase().includes(kw.toLowerCase()));
    }
    renderMembers();
    toast(kw ? `搜索结果 ${state.persons.length} 条` : "已显示全部");
}

// ================= 启动 =================
async function init() {
    bindEvents();
    await refreshAll();
    // 初始切换到成员页时图谱未显示，切换时再布局
}

document.addEventListener("DOMContentLoaded", init);
