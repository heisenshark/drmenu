#!/usr/bin/env python3
import socket
import os
import json
import sys

def get_socket_path():
    his = os.environ.get("HYPRLAND_INSTANCE_SIGNATURE")
    if not his:
        dirs = os.listdir("/run/user/1000/hypr")
        if dirs:
            his = dirs[0]
    return f"/run/user/1000/hypr/{his}/.socket.sock"

def send_hypr_cmd(cmd):
    sock_path = get_socket_path()
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.connect(sock_path)
    s.sendall(cmd.encode("utf-8"))
    resp = s.recv(4096)
    s.close()
    return resp.decode("utf-8")

def get_focused_monitor():
    raw = send_hypr_cmd("j/monitors")
    monitors = json.loads(raw)
    for m in monitors:
        if m.get("focused"):
            return m
    return monitors[0] if monitors else {"x": 0, "y": 0, "width": 1920, "height": 1080}

def generate_test_pills():
    mon = get_focused_monitor()
    mx = mon["x"]
    my = mon["y"]
    mw = mon["width"]
    mh = mon["height"]

    print(f"Generating test pills on monitor '{mon.get('name')}' at offset ({mx}, {my})...")

    pills = []

    # Row 1: Blur sweep (y = 150) - from 0 blur to milky 80 blur
    blurs = [0.0, 12.0, 28.0, 50.0, 85.0]
    for i, b in enumerate(blurs):
        px = mx + 100 + i * 340
        py = my + 150
        pw = 280
        ph = 70
        rad = 22
        refr = 0.8
        chrom = 1.6
        spec = 0.75
        pills.append(f"{px} {py} {pw} {ph} {rad} {b} {refr} {chrom} {spec} 1.0 1.0 1.0 0.15 1.0 1.0 1.0 0.45 1.5")

    # Row 2: Chromatic Aberration sweep (y = 320) - from 0.0 to 14.0 RGB dispersion
    chroms = [0.0, 1.5, 3.5, 7.0, 14.0]
    for i, c in enumerate(chroms):
        px = mx + 100 + i * 340
        py = my + 320
        pw = 280
        ph = 70
        rad = 22
        blur = 28.0
        refr = 0.8
        spec = 0.75
        pills.append(f"{px} {py} {pw} {ph} {rad} {blur} {refr} {c} {spec} 1.0 1.0 1.0 0.15 1.0 1.0 1.0 0.55 1.5")

    # Row 3: Convex Lens Refraction sweep (y = 490) - from 0.0 to 2.8 refraction
    refrs = [0.0, 0.4, 0.9, 1.6, 2.8]
    for i, r in enumerate(refrs):
        px = mx + 100 + i * 340
        py = my + 490
        pw = 280
        ph = 70
        rad = 22
        blur = 28.0
        chrom = 2.0
        spec = 0.75
        pills.append(f"{px} {py} {pw} {ph} {rad} {blur} {r} {chrom} {spec} 1.0 1.0 1.0 0.15 1.0 1.0 1.0 0.45 1.5")

    # Row 4: Showcase center pills & circular lens (y = 660)
    pills.append(f"{mx + 150} {my + 660} 140 140 70 35.0 1.2 3.0 0.85 1.0 1.0 1.0 0.18 1.0 1.0 1.0 0.60 2.0")
    pills.append(f"{mx + 360} {my + 680} 380 90 45 40.0 0.8 2.0 0.80 1.0 1.0 1.0 0.28 1.0 1.0 1.0 0.50 1.5")
    pills.append(f"{mx + 810} {my + 680} 380 90 24 30.0 0.9 2.5 0.90 0.1 0.1 0.15 0.35 1.0 1.0 1.0 0.45 1.5")
    pills.append(f"{mx + 1260} {my + 660} 450 140 28 45.0 1.5 5.0 0.95 1.0 1.0 1.0 0.12 1.0 1.0 1.0 0.75 2.0")

    pill_data = ";".join(pills)
    cmd = f"/repl return hl.plugin.liquid_glass.set_pills([[{pill_data}]])"
    res = send_hypr_cmd(cmd)
    print(f"Sent {len(pills)} test pills to Hyprland plugin. Result: {res.strip()}")

def clear_pills():
    res = send_hypr_cmd("/repl return hl.plugin.liquid_glass.clear()")
    print(f"Cleared all pills. Result: {res.strip()}")

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "clear":
        clear_pills()
    else:
        generate_test_pills()
