# -*- coding: utf-8 -*-
"""家庭信息管理系统 - 图形界面入口。

用法:
    python main.py                  # 连接默认后端 http://127.0.0.1:8080
    python main.py --url http://127.0.0.1:9000
    python main.py --backend 路径/command-line.exe   # 指定后端exe（用于一键启动）
    python main.py --smoke          # 自检模式：启动1.5秒后自动关闭，验证界面正常
"""

import argparse
import sys

from api import ApiClient
from gui import FamilyInfoApp


def parse_port(url):
    try:
        from urllib.parse import urlparse
        port = urlparse(url).port
        return port if port else 8080
    except ValueError:
        return 8080


def main():
    parser = argparse.ArgumentParser(description="家庭信息管理系统 - 图形界面")
    parser.add_argument("--url", default="http://127.0.0.1:8080", help="后端地址")
    parser.add_argument("--backend", default=None, help="后端exe路径（用于一键启动）")
    parser.add_argument("--smoke", action="store_true", help="自检模式，启动后自动关闭")
    args = parser.parse_args()

    api = ApiClient(args.url)
    app = FamilyInfoApp(api, backend_exe=args.backend,
                        port=parse_port(args.url), smoke=args.smoke)
    app.run()
    return 0


if __name__ == "__main__":
    sys.exit(main())
