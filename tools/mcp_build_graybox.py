#!/usr/bin/env python3
# 步驟 9 灰盒關卡：讀 Content/Data/Level_Layout.csv，透過 Epic MCP（127.0.0.1:8000）在開著的編輯器裡把 /Game/Maps/Lvl_Academy 整個重蓋並存檔。
"""用 MCP 把 Level_Layout.csv 蓋成灰盒地圖 /Game/Maps/Lvl_Academy。只用標準函式庫。

前提：編輯器已用 `-ExecCmds="ModelContextProtocol.StartServer"` 開著（MCP 只聽 127.0.0.1:8000；AGENTS 第 9 條）。
一次執行 = 一次 MCP 任務：組成一段 ProgrammaticToolset 的 `execute_tool_script`，在編輯器內一次做完：

1. 地圖還不存在 → 複製 /Engine/Maps/Templates/Template_Default（非 World Partition）成 /Game/Maps/Lvl_Academy 並存檔；
   已存在 → 直接載入。（AssetTools.delete 只會把它從記憶體卸掉，磁碟上的 .umap 還在，所以重跑改成「清空再重蓋」。）
2. 載入地圖，刪掉上次蓋的東西（資料夾 Graybox 底下的所有 Actor）和範本的地板（Floor）。
3. 照 CSV 放 Actor：Floor／Wall＝/Engine/BasicShapes/Cube（100 cm 方塊，用 Actor 縮放成 SX／SY／SZ）；
   Area＝AEscapeAreaVolume（AreaId）；Object／Door＝AEscapeObjectActor（ObjectId，Door 列 bIsDoor＝true）；
   Spawn＝範本原有的 PlayerStart 移到出生點（剛好一個）。每個 Actor 設 label 與資料夾。
4. 存檔 /Game/Maps/Lvl_Academy。

重跑結果一樣（同樣的 Actor 數量、位置、ID）。用法：python3 -I tools/mcp_build_graybox.py [--print-script]
"""
import csv
import json
import sys
import urllib.parse
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LAYOUT = ROOT / "Content" / "Data" / "Level_Layout.csv"
MCP_URL = "http://127.0.0.1:8000/mcp"  # 只能連本機（AGENTS 第 9 條）
MAP_PATH = "/Game/Maps/Lvl_Academy"
TEMPLATE_MAP = "/Engine/Maps/Templates/Template_Default"
TOOLS = "editor_toolset.toolsets."
PROGRAMMATIC = TOOLS + "programmatic.ProgrammaticToolset"

# 在編輯器裡跑的腳本（沙盒 Python：只能 import json／math／re 等；用 execute_tool 呼叫其他工具）。
# 下面的 ROWS 由本檔在前面接上。
SCRIPT_BODY = r'''
import json

TOOLS = "editor_toolset.toolsets."
MAP_PATH = "/Game/Maps/Lvl_Academy"
TEMPLATE_MAP = "/Engine/Maps/Templates/Template_Default"
CUBE = "/Engine/BasicShapes/Cube"
GRAYBOX_FOLDER = "Graybox"
AREA_CLASS = "/Script/EscapeGame.EscapeAreaVolume"
OBJECT_CLASS = "/Script/EscapeGame.EscapeObjectActor"
PLAYER_START_CLASS = "/Script/Engine.PlayerStart"


def call(tool, **kwargs):
    return execute_tool(TOOLS + tool, json.dumps(kwargs))


def transform(row):
    return {
        "location": {"x": row["X"], "y": row["Y"], "z": row["Z"]},
        "rotation": {"pitch": 0, "yaw": row["Yaw"], "roll": 0},
        "scale": {"x": row["SX"] / 100.0, "y": row["SY"] / 100.0, "z": row["SZ"] / 100.0},
    }


def decorate(actor, label, folder):
    call("actor.ActorTools.set_label", actor=actor, label=label)
    call("scene.SceneTools.set_actor_folder", actor=actor, folder_path=folder)


def set_props(actor, values):
    call("object.ObjectTools.set_properties", instance=actor, values=json.dumps(values))


def place_cube(row, label, folder):
    actor = call("scene.SceneTools.add_to_scene_from_asset", asset_path=CUBE, name=label, xform=transform(row))["returnValue"]
    decorate(actor, label, folder)
    return actor


def place_class(class_path, row, label, folder, values):
    actor = call("scene.SceneTools.add_to_scene_from_class", actor_type={"refPath": class_path}, name=label, xform=transform(row))["returnValue"]
    set_props(actor, values)
    decorate(actor, label, folder)
    return actor


def run():
    report = {}

    # 1. 地圖：沒有就從範本複製並存檔；然後載入
    if not call("asset.AssetTools.exists", path=MAP_PATH)["returnValue"]:
        report["duplicated_template"] = call("asset.AssetTools.duplicate", path=TEMPLATE_MAP, new_path=MAP_PATH)["returnValue"]
        report["saved_copy"] = call("asset.AssetTools.save_assets", asset_paths=[MAP_PATH])["returnValue"]
    if call("scene.SceneTools.get_current_level")["returnValue"] != MAP_PATH:
        call("scene.SceneTools.load_level", level_path=MAP_PATH)
    report["level"] = call("scene.SceneTools.get_current_level")["returnValue"]

    # 2. 清掉上次蓋的（Graybox 資料夾）和範本的地板
    removed = 0
    folders = call("scene.SceneTools.get_folders")["returnValue"]
    if GRAYBOX_FOLDER in folders:  # 第一次跑還沒有這個資料夾；查不存在的資料夾會丟錯
        for actor in call("scene.SceneTools.get_actors_in_folder", folder_path=GRAYBOX_FOLDER, recursive=True)["returnValue"]:
            if call("scene.SceneTools.remove_from_scene", actor=actor)["returnValue"]:
                removed += 1
    for actor in call("scene.SceneTools.find_actors", name="Floor", tag="", collision_channels=[])["returnValue"]:
        if call("actor.ActorTools.get_label", actor=actor)["returnValue"] == "Floor":
            if call("scene.SceneTools.remove_from_scene", actor=actor)["returnValue"]:
                removed += 1
    report["removed"] = removed

    # 3. 照 CSV 放 Actor
    counts = {"Area": 0, "Floor": 0, "Wall": 0, "Door": 0, "Object": 0, "Spawn": 0}
    placed_ids = []
    s11_index = 0
    for row in ROWS:
        kind = row["Kind"]
        rid = row["Id"]
        area = row["Area"] or "None"
        if kind == "Area":
            place_class(AREA_CLASS, row, "Area_" + rid, GRAYBOX_FOLDER + "/Areas", {"AreaId": rid})
        elif kind == "Floor":
            place_cube(row, rid, GRAYBOX_FOLDER + "/Floors")
        elif kind == "Wall":
            place_cube(row, rid, GRAYBOX_FOLDER + "/Walls")
        elif kind == "Door":
            place_class(OBJECT_CLASS, row, rid, GRAYBOX_FOLDER + "/Doors", {"ObjectId": rid, "bIsDoor": True})
        elif kind == "Object":
            label = rid
            if rid == "S11":
                s11_index += 1
                label = "S11_" + str(s11_index)
            place_class(OBJECT_CLASS, row, label, GRAYBOX_FOLDER + "/Objects/" + area, {"ObjectId": rid, "bIsDoor": False})
        elif kind == "Spawn":
            starts = call("scene.SceneTools.find_actors", name="", actor_type={"refPath": PLAYER_START_CLASS}, tag="", collision_channels=[])["returnValue"]
            if len(starts) == 0:
                start = call("scene.SceneTools.add_to_scene_from_class", actor_type={"refPath": PLAYER_START_CLASS}, name="PlayerStart", xform=transform(row))["returnValue"]
            else:
                start = starts[0]
                for extra in starts[1:]:
                    call("scene.SceneTools.remove_from_scene", actor=extra)
            call("actor.ActorTools.set_actor_transform", actor=start, xform=transform(row), worldspace=True)
            decorate(start, "PlayerStart", GRAYBOX_FOLDER + "/Spawn")
        counts[kind] += 1
        placed_ids.append(rid)
    report["placed"] = counts

    # 4. 檢查剛好一個 PlayerStart、存檔
    report["player_starts"] = len(call("scene.SceneTools.find_actors", name="", actor_type={"refPath": PLAYER_START_CLASS}, tag="", collision_channels=[])["returnValue"])
    report["saved"] = call("asset.AssetTools.save_assets", asset_paths=[MAP_PATH])["returnValue"]
    return report
'''


def read_rows():
    rows = []
    with open(LAYOUT, encoding="utf-8", newline="") as f:
        for r in csv.DictReader(f):
            for col in ("X", "Y", "Z", "SX", "SY", "SZ", "Yaw"):
                r[col] = float(r[col])
            rows.append(r)
    return rows


def build_script(rows):
    return "import json\nROWS = json.loads(" + json.dumps(json.dumps(rows, ensure_ascii=False)) + ")\n" + SCRIPT_BODY


class McpClient:
    """最小的 MCP Streamable HTTP 客戶端：initialize → initialized → tools/call"""

    def __init__(self, url):
        host = urllib.parse.urlparse(url).hostname
        if host != "127.0.0.1":
            raise SystemExit("MCP 只能連 127.0.0.1（AGENTS 第 9 條），不是 " + str(host))
        self.url = url
        self.session = None
        self.next_id = 1
        _, headers = self._post({"jsonrpc": "2.0", "id": self._id(), "method": "initialize",
                                 "params": {"protocolVersion": "2025-03-26", "capabilities": {},
                                            "clientInfo": {"name": "mcp_build_graybox", "version": "1"}}})
        self.session = headers.get("Mcp-Session-Id")
        self._post({"jsonrpc": "2.0", "method": "notifications/initialized"})

    def _id(self):
        self.next_id += 1
        return self.next_id

    def _post(self, body):
        headers = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}
        if self.session:
            headers["Mcp-Session-Id"] = self.session
        req = urllib.request.Request(self.url, data=json.dumps(body).encode("utf-8"), headers=headers)
        with urllib.request.urlopen(req, timeout=900) as resp:
            raw = resp.read().decode("utf-8")
            resp_headers = dict(resp.headers)
        if not raw.strip():
            return None, resp_headers
        if raw.lstrip().startswith(("event:", "data:")):
            for line in raw.splitlines():
                if line.startswith("data:"):
                    raw = line[5:].strip()
                    break
        return json.loads(raw), resp_headers

    def call_tool(self, toolset, tool, arguments):
        reply, _ = self._post({"jsonrpc": "2.0", "id": self._id(), "method": "tools/call",
                               "params": {"name": "call_tool",
                                          "arguments": {"toolset_name": toolset, "tool_name": tool, "arguments": arguments}}})
        if "error" in reply:
            raise RuntimeError(json.dumps(reply["error"], ensure_ascii=False))
        result = reply["result"]
        text = "".join(c.get("text", "") for c in result.get("content", []))
        if result.get("isError"):
            raise RuntimeError(text)
        return text


def main(argv):
    script = build_script(read_rows())
    if "--print-script" in argv:
        print(script)
        return 0
    client = McpClient(MCP_URL)
    text = client.call_tool(PROGRAMMATIC, "execute_tool_script", {"script": script})
    outer = json.loads(text)  # {"returnValue": "<JSON 字串>"}
    report = json.loads(outer["returnValue"])
    print(json.dumps(report, ensure_ascii=False, indent=2))
    ok = report.get("saved") is True and report.get("player_starts") == 1
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
