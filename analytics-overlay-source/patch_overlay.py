from pathlib import Path

p = Path("deepstream_app_main.c")
s = p.read_text()

print("Source file:", p.resolve())
print("Has overlay_graphics:", "overlay_graphics" in s)
print("Has main function:", "main (" in s or "main(" in s)
print("Has analytics include:", "nvds_analytics_meta.h" in s)

for i, line in enumerate(s.splitlines(), 1):
    if "overlay_graphics" in line or line.strip() in ("int", "main ("):
        print(f"{i}: {line}")
