#!/usr/bin/env python3
"""
Interactive test for the Hyprland liquid-glass compositor plugin.
Writes triangle data directly to a binary file that the plugin reads each frame.
"""
import socket, os, json, random, sys, struct, math

TRI_FILE = "/tmp/hypr_liquid_glass_tris.bin"

def get_socket_path():
    his = os.environ.get("HYPRLAND_INSTANCE_SIGNATURE")
    if not his:
        dirs = os.listdir("/run/user/1000/hypr")
        if dirs: his = dirs[0]
    return f"/run/user/1000/hypr/{his}/.socket.sock"

def send_hypr_cmd(cmd):
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.connect(get_socket_path())
    s.sendall(cmd.encode("utf-8"))
    resp = s.recv(4096)
    s.close()
    return resp.decode("utf-8", errors="replace")

def get_focused_monitor():
    monitors = json.loads(send_hypr_cmd("j/monitors"))
    for m in monitors:
        if m.get("focused"): return m
    return monitors[0] if monitors else {"width": 1920, "height": 1080}

def write_triangles_file(tris):
    """Binary: uint32 count + N*(x1,y1,x2,y2,x3,y3,r,g,b,a) float32"""
    with open(TRI_FILE, "wb") as f:
        f.write(struct.pack("<I", len(tris)))
        for tri in tris:
            f.write(struct.pack("<10f", *tri))

def kick_plugin():
    send_hypr_cmd("/repl return hl.plugin.liquid_glass.clear()")

def spawn_random_triangles(count=6, white_only=False):
    mon = get_focused_monitor()
    mw, mh = mon.get("width", 1920), mon.get("height", 1080)
    tris = []
    for _ in range(count):
        size = random.randint(120, 450)
        cx = random.randint(size, max(size+1, mw-size))
        cy = random.randint(size, max(size+1, mh-size))
        a1 = random.uniform(0, 2*math.pi)
        a2 = a1 + random.uniform(1.8, 2.5)
        a3 = a2 + random.uniform(1.8, 2.5)
        def v(a): return random.uniform(0.7, 1.0) * size * 0.5 * a
        x1 = cx + v(math.cos(a1)); y1 = cy + v(math.sin(a1))
        x2 = cx + v(math.cos(a2)); y2 = cy + v(math.sin(a2))
        x3 = cx + v(math.cos(a3)); y3 = cy + v(math.sin(a3))
        if white_only:
            r,g,b,a = 1.0, 1.0, 1.0, random.uniform(0.85, 1.0)
        else:
            r = random.uniform(0.3, 1.0); g = random.uniform(0.3, 1.0)
            b = random.uniform(0.3, 1.0); a = random.uniform(0.85, 1.0)
        tris.append((x1,y1,x2,y2,x3,y3,r,g,b,a))
    write_triangles_file(tris)
    kick_plugin()
    print(f"Spawned {count} random {'white' if white_only else 'colored'} triangles on {mon.get('name')}.")

def clear_all():
    with open(TRI_FILE, "wb") as f: f.write(struct.pack("<I", 0))
    kick_plugin()
    print("Cleared all shapes.")

def main():
    print("========================================")
    print("  Hyprland Compositor Random Triangles  ")
    print("========================================")
    print("  [t] white triangles  [c] colored  [x] clear  [q] quit")
    if len(sys.argv) > 1:
        if sys.argv[1] == "random":
            spawn_random_triangles(int(sys.argv[2]) if len(sys.argv)>2 else 6, white_only=True); return
        elif sys.argv[1] == "clear":
            clear_all(); return
    spawn_random_triangles(5, white_only=True)
    while True:
        try:
            ch = input("\n[t/c/x/q]: ").strip().lower()
            if ch=='t': spawn_random_triangles(6, white_only=True)
            elif ch=='c': spawn_random_triangles(6, white_only=False)
            elif ch=='x': clear_all()
            elif ch=='q': clear_all(); break
        except (KeyboardInterrupt, EOFError): clear_all(); break

if __name__ == "__main__": main()
