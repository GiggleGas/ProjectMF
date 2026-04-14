"""
gif_to_png_sequence.py
将 GIF 的每一帧按从左到右、从上到下的顺序拼合成一张 Sprite Sheet 大图。

用法:
    python gif_to_png_sequence.py <gif文件> [输出文件] [--cols N]

示例:
    python gif_to_png_sequence.py anim.gif
    python gif_to_png_sequence.py anim.gif sheet.png --cols 8
"""

import argparse
import math
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("[错误] 需要安装 Pillow: pip install Pillow")
    sys.exit(1)


def extract_gif_frames(gif_path: Path) -> list[Image.Image]:
    frames = []
    with Image.open(gif_path) as gif:
        canvas = Image.new("RGBA", gif.size, (0, 0, 0, 0))
        try:
            while True:
                frame = gif.convert("RGBA")
                disposal = gif.disposal_method if hasattr(gif, "disposal_method") else 0
                composed = Image.new("RGBA", gif.size, (0, 0, 0, 0)) if disposal == 2 else canvas.copy()
                composed.paste(frame, (0, 0), frame)
                frames.append(composed.copy())
                if disposal != 3:
                    canvas = composed.copy()
                gif.seek(gif.tell() + 1)
        except EOFError:
            pass
    return frames


def build_sprite_sheet(gif_path: Path, output_file: Path, cols: int, mirror: bool):
    print(f"读取 GIF: {gif_path}")
    frames = extract_gif_frames(gif_path)
    if mirror:
        frames = [f.transpose(Image.FLIP_LEFT_RIGHT) for f in frames]
        print("已对所有帧进行左右镜像")

    total = len(frames)
    rows = math.ceil(total / cols)

    frame_w, frame_h = frames[0].size
    sheet_w = frame_w * cols
    sheet_h = frame_h * rows
    sheet = Image.new("RGBA", (sheet_w, sheet_h), (0, 0, 0, 0))

    print(f"帧数: {total}  |  帧尺寸: {frame_w}x{frame_h}  |  布局: {cols}列 x {rows}行")
    print(f"输出大图尺寸: {sheet_w}x{sheet_h}\n")

    for i, frame in enumerate(frames):
        x = (i % cols) * frame_w
        y = (i // cols) * frame_h
        sheet.paste(frame, (x, y))

    output_file.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(output_file, "PNG")
    print(f"完成 -> {output_file.resolve()}")


def interactive_mode():
    print("=== GIF → Sprite Sheet 拼图工具 ===\n")

    while True:
        input_str = input("GIF 文件路径: ").strip().strip('"')
        gif_path = Path(input_str)
        if gif_path.is_file() and gif_path.suffix.lower() == ".gif":
            break
        print("[错误] 请输入有效的 GIF 文件路径\n")

    default_output = gif_path.parent / (gif_path.stem + "_sheet.png")
    output_str = input(f"输出文件路径 [默认: {default_output}]: ").strip().strip('"')
    output_file = Path(output_str) if output_str else default_output

    cols_str = input("列数 [默认: 10]: ").strip()
    cols = int(cols_str) if cols_str.isdigit() and int(cols_str) > 0 else 10

    mirror_str = input("左右镜像 [0=否 / 1=是，默认: 0]: ").strip()
    mirror = mirror_str == "1"

    print()
    build_sprite_sheet(gif_path, output_file, cols, mirror)


def main():
    if len(sys.argv) == 1:
        interactive_mode()
        return

    parser = argparse.ArgumentParser(description="将 GIF 每帧拼合为 Sprite Sheet 大图")
    parser.add_argument("input", help="GIF 文件路径")
    parser.add_argument("output", nargs="?", help="输出文件路径（默认: 同目录下 <name>_sheet.png）")
    parser.add_argument("--cols", type=int, default=10, help="每行列数（默认: 10）")
    parser.add_argument("--mirror", type=int, default=0, choices=[0, 1], help="左右镜像 0=否 1=是（默认: 0）")
    args = parser.parse_args()

    gif_path = Path(args.input)
    if not gif_path.is_file() or gif_path.suffix.lower() != ".gif":
        print(f"[错误] 请提供有效的 GIF 文件: {args.input}")
        sys.exit(1)

    output_file = Path(args.output) if args.output else gif_path.parent / (gif_path.stem + "_sheet.png")
    build_sprite_sheet(gif_path, output_file, args.cols, args.mirror == 1)


if __name__ == "__main__":
    main()
