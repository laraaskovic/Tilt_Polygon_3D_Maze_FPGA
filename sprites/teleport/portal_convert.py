from PIL import Image

NUM_FRAMES = 41
FRAME_PREFIX = "Portal_100x100px"

all_arrays = []

for i in range(1, NUM_FRAMES + 1):
    filename = f"Portal/{FRAME_PREFIX}{i}.png"
    array_name = f"portal_frame_{i}"

    img = Image.open(filename).convert("RGBA")
    img = img.resize((16, 16), Image.LANCZOS) 
    w, h = img.size
    pixels = []

    for y in range(h):
        for x in range(w):
            r, g, b, a = img.getpixel((x, y))
            if a < 128:
                rgb565 = 0x0000  # transparent → black (will be skipped in draw_sprite)
            else:
                rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            pixels.append(rgb565)

    all_arrays.append((array_name, pixels))
    print(f"Converted {filename}")

# Write one single bundled header
with open("teleport_frames.h", "w") as f:
    f.write("#ifndef TELEPORT_FRAMES_H\n#define TELEPORT_FRAMES_H\n\n")
    f.write(f"#define PORTAL_W {w}\n")
    f.write(f"#define PORTAL_H {h}\n")
    f.write(f"#define PORTAL_FRAME_COUNT {NUM_FRAMES}\n\n")

    for name, pixels in all_arrays:
        f.write(f"unsigned short {name}[] = {{\n    ")
        f.write(", ".join(str(p) for p in pixels))
        f.write("\n};\n\n")

    f.write("unsigned short *teleport_frames[] = {\n")
    for name, _ in all_arrays:
        f.write(f"    {name},\n")
    f.write("};\n\n#endif\n")

print(f"\nDone! teleport_frames.h — {NUM_FRAMES} frames at {w}x{h}px")