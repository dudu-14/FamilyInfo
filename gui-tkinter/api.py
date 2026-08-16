# -*- coding: utf-8 -*-
"""与C++后端（command-line.exe --server）通信的REST API客户端。

只使用Python标准库，不依赖第三方包。
"""

import json
import urllib.error
import urllib.parse
import urllib.request


class ApiError(Exception):
    """接口调用失败时抛出，message为后端返回的错误信息。"""


class ApiClient:
    """封装后端REST接口，所有方法返回解析后的JSON对象。"""

    def __init__(self, base_url="http://127.0.0.1:8080", timeout=5):
        self.base_url = base_url.rstrip("/")
        self.timeout = timeout

    def _request(self, method, path, body=None):
        url = self.base_url + path
        data = None
        headers = {}
        if body is not None:
            data = json.dumps(body).encode("utf-8")
            headers["Content-Type"] = "application/json"
        req = urllib.request.Request(url, data=data, headers=headers, method=method)
        try:
            with urllib.request.urlopen(req, timeout=self.timeout) as resp:
                raw = resp.read()
                ctype = resp.headers.get("Content-Type", "")
                if "application/json" in ctype:
                    return json.loads(raw.decode("utf-8"))
                return raw.decode("utf-8-sig")  # CSV带UTF-8 BOM，用utf-8-sig去掉
        except urllib.error.HTTPError as e:
            # 后端返回的错误是JSON：{"error": "..."}
            try:
                msg = json.loads(e.read().decode("utf-8")).get("error", str(e))
            except Exception:
                msg = str(e)
            raise ApiError(msg)
        except urllib.error.URLError as e:
            raise ApiError("无法连接后端: %s" % e.reason)

    # ---------- 成员 ----------
    def list_persons(self):
        return self._request("GET", "/api/persons")

    def get_person(self, person_id):
        return self._request("GET", "/api/persons/%d" % person_id)

    def add_person(self, name, birthday, sex):
        return self._request("POST", "/api/persons",
                             {"name": name, "birthday": birthday, "sex": sex})

    def update_person(self, person_id, **fields):
        return self._request("PUT", "/api/persons/%d" % person_id, fields)

    def delete_person(self, person_id):
        return self._request("DELETE", "/api/persons/%d" % person_id)

    # ---------- 关系 ----------
    def list_relations(self, person_id=None):
        path = "/api/relations"
        if person_id is not None:
            path += "?person_id=%d" % person_id
        return self._request("GET", path)

    def add_relation(self, person_id, rel_type, target_id):
        return self._request("POST", "/api/relations",
                             {"person_id": person_id, "type": rel_type, "target_id": target_id})

    def delete_relation(self, person_id, rel_type, target_id):
        return self._request("DELETE", "/api/relations",
                             {"person_id": person_id, "type": rel_type, "target_id": target_id})

    # ---------- 统计 / 搜索 / 导出 ----------
    def stats(self):
        return self._request("GET", "/api/stats")

    def search(self, keyword):
        q = urllib.parse.quote(keyword)
        return self._request("GET", "/api/search?q=" + q)

    def export_csv(self):
        return self._request("GET", "/api/export")
