#!/usr/bin/env python3
"""
MiceCam Bin to Images Converter

将 .bin 文件中的每一帧提取出来并保存到同名文件夹中。
支持直接提取 JPEG (如果 bin 中原有的就是 MJPEG) 或保存为 PNG。
"""

import os
import sys
from pathlib import Path
from read_bin import MiceCamReader

def bin_to_images(session_path: str, output_dir: str = None):
    """
    将 bin 文件转换并提取各帧为图片文件
    """
    try:
        reader = MiceCamReader(session_path)
    except Exception as e:
        print(f"Error: 无法初始化读取器: {e}")
        return

    # 确定输出目录：如果未指定，则使用 session 同名文件夹
    if output_dir is None:
        output_dir = reader.session_path.parent / reader.session_path.name
    else:
        output_dir = Path(output_dir)

    # 创建输出目录
    output_dir.mkdir(parents=True, exist_ok=True)
    print(f"提取帧到目录: {output_dir}")
    print(f"共计 {reader.total_frames} 帧...")

    for i in range(1, reader.total_frames + 1):
        raw_data = reader.get_raw_frame_bytes(i)
        if raw_data is None:
            continue

        # 获取该帧的时间戳 (纳秒)
        timestamp = reader.get_frame_timestamp(i)
        # 如果获取失败，回退到 sequence_id
        if timestamp == 0:
            basename = f"frame_{i:06d}"
        else:
            basename = f"{timestamp}"

        # 检查是否是 JPEG 格式 (MJPEG 的帧通常以 FF D8 开头)
        # 精确检查前两个字节
        is_jpeg = len(raw_data) > 2 and raw_data[0] == 0xFF and raw_data[1] == 0xD8

        if is_jpeg:
            # 直接保存为 .jpg，不经过重新编码，速度最快且无损
            filename = f"{basename}.jpg"
            file_path = output_dir / filename
            with open(file_path, "wb") as f:
                f.write(raw_data)
        else:
            # 如果不是 JPEG，则尝试解码并保存为 PNG
            try:
                import cv2
                frame = reader.get_frame(i)
                if frame is not None:
                    # 如果是读取器解码出来的，通常是 BGR
                    filename = f"{basename}.png"
                    file_path = output_dir / filename
                    cv2.imwrite(str(file_path), frame)
                else:
                    # 如果读取器也无法解码，保存为原始 bin 块供分析
                    filename = f"{basename}.raw"
                    file_path = output_dir / filename
                    with open(file_path, "wb") as f:
                        f.write(raw_data)
                    print(f"Warning: 无法解码第 {i} 帧，已保存为原始数据 {filename}")
            except ImportError:
                print(f"Error: 无法解码非 JPEG 帧 (需要 opencv-python)，第 {i} 帧跳过")
                break

        if i % 100 == 0 or i == reader.total_frames:
            print(f"已处理: {i}/{reader.total_frames}", end="\r")

    print(f"\n提取完成！图片保存在: {output_dir}")

def main():
    if len(sys.argv) < 2:
        print("用法: python bin_to_images.py <session_path> [output_dir]")
        print("示例: python bin_to_images.py ../test_output/session_001")
        sys.exit(1)

    session_path = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else None

    bin_to_images(session_path, output_dir)

if __name__ == "__main__":
    main()
