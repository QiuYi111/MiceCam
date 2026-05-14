#!/usr/bin/env python3
"""
MiceCam Data Converter

将 MiceCam 采集的数据转换为常用格式（NumPy, HDF5, Video）
"""

import numpy as np
import sys
from pathlib import Path
from read_bin import MiceCamReader


def convert_to_numpy(session_path: str, output_path: str = None):
    """
    转换为 NumPy .npy 格式

    Args:
        session_path: 会话路径
        output_path: 输出文件路径（默认: session_path.npy）
    """
    print(f"Converting {session_path} to NumPy format...")

    reader = MiceCamReader(session_path)

    # 读取所有帧
    frames = reader.get_all_frames()
    print(f"Loaded {len(frames)} frames, shape: {frames.shape}")

    # 保存
    if output_path is None:
        output_path = Path(session_path).with_suffix('.npy')
    else:
        output_path = Path(output_path)

    np.save(output_path, frames)
    print(f"Saved to: {output_path}")


def convert_to_hdf5(session_path: str, output_path: str = None):
    """
    转换为 HDF5 格式（Stage 2 的轻量级实现）

    注意：需要安装 h5py: pip install h5py

    Args:
        session_path: 会话路径
        output_path: 输出文件路径（默认: session_path.h5）
    """
    try:
        import h5py
    except ImportError:
        print("Error: h5py not installed. Run: pip install h5py")
        sys.exit(1)

    print(f"Converting {session_path} to HDF5 format...")

    reader = MiceCamReader(session_path)

    if output_path is None:
        output_path = Path(session_path).with_suffix('.h5')
    else:
        output_path = Path(output_path)

    with h5py.File(output_path, 'w') as f:
        # 创建数据集
        frames = reader.get_all_frames()
        f.create_dataset('frames', data=frames, compression='gzip')

        # 保存元数据
        session_group = f.create_group('session')
        for key, value in reader.session_info.items():
            if isinstance(value, (str, int, float, bool)):
                session_group.attrs[key] = value

        # 保存帧元数据
        frames_group = f.create_group('frames_metadata')
        for frame_info in reader.frames:
            frame_id = frame_info['sequence_id']
            frame_dset = frames_group.create_group(f'frame_{frame_id:06d}')
            for key, value in frame_info.items():
                if isinstance(value, (str, int, float, bool)):
                    frame_dset.attrs[key] = value

    print(f"Saved to: {output_path}")
    print(f"  - Dataset: frames (shape: {frames.shape})")
    print(f"  - Metadata: session/*")
    print(f"  - Frame metadata: frames_metadata/frame_*")


def export_to_video(session_path: str, output_path: str = None, fps: float = None):
    """
    导出为视频文件

    注意：需要安装 opencv-python: pip install opencv-python

    Args:
        session_path: 会话路径
        output_path: 输出视频路径（默认: session_path.mp4）
        fps: 帧率（默认使用采集时的 fps）
    """
    try:
        import cv2
    except ImportError:
        print("Error: opencv-python not installed. Run: pip install opencv-python")
        sys.exit(1)

    print(f"Exporting {session_path} to video...")

    reader = MiceCamReader(session_path)

    if output_path is None:
        output_path = Path(session_path).with_suffix('.mp4')
    else:
        output_path = Path(output_path)

    if fps is None:
        fps = reader.fps

    # 获取所有帧
    frames = reader.get_all_frames()

    # 写入视频
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    writer = cv2.VideoWriter(
        str(output_path),
        fourcc,
        fps,
        (reader.width, reader.height)
    )

    for frame in frames:
        # 转换 RGB 到 BGR（OpenCV 格式）
        if len(frame.shape) == 3:
            frame_bgr = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
        else:
            frame_bgr = frame
        writer.write(frame_bgr)

    writer.release()

    print(f"Saved to: {output_path}")
    print(f"  - Resolution: {reader.width}x{reader.height}")
    print(f"  - FPS: {fps}")
    print(f"  - Duration: {len(frames) / fps:.2f} seconds")


def print_usage():
    """打印使用说明"""
    print("""
MiceCam Data Converter
======================

Usage:
    python convert_to_numpy.py <session_path> [output_path]
    python convert_to_hdf5.py <session_path> [output_path]
    python export_to_video.py <session_path> [output_path] [fps]

Examples:
    # 转换为 NumPy
    python convert_to_numpy.py output/session_001

    # 转换为 HDF5
    python convert_to_hdf5.py output/session_001

    # 导出为视频
    python export_to_video.py output/session_001

Requirements:
    - NumPy (always): pip install numpy
    - h5py (for HDF5): pip install h5py
    - opencv-python (for video): pip install opencv-python
    """)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print_usage()
        sys.exit(1)

    # 根据脚本名决定功能
    script_name = Path(sys.argv[0]).name
    session_path = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else None

    if 'numpy' in script_name:
        convert_to_numpy(session_path, output_path)
    elif 'hdf5' in script_name:
        convert_to_hdf5(session_path, output_path)
    elif 'video' in script_name:
        fps = float(sys.argv[3]) if len(sys.argv) > 3 else None
        export_to_video(session_path, output_path, fps)
    else:
        print("Unknown script name:", script_name)
        print_usage()
        sys.exit(1)
