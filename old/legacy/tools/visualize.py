#!/usr/bin/env python3
"""
MiceCam 数据可视化工具

展示采集的帧序列、时间戳分布、帧率统计等
"""

import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
from read_bin import MiceCamReader


def visualize_frames(session_path: str, num_frames: int = 10):
    """
    可视化前几帧

    Args:
        session_path: 会话路径
        num_frames: 显示的帧数
    """
    try:
        import cv2
    except ImportError:
        print("Error: opencv-python not installed. Run: pip install opencv-python")
        return

    print(f"Visualizing {session_path}...")

    reader = MiceCamReader(session_path)

    # 读取前几帧
    frames = reader.get_frames(1, min(num_frames, reader.total_frames))

    # 创建子图
    cols = min(5, len(frames))
    rows = (len(frames) + cols - 1) // cols

    fig, axes = plt.subplots(rows, cols, figsize=(15, 3 * rows))
    if rows == 1:
        axes = axes.reshape(1, -1)

    for i, frame in enumerate(frames):
        ax = axes[i // cols, i % cols]
        # 转换 RGB 到 BGR
        frame_bgr = frame
        if len(frame.shape) == 3:
            frame_bgr = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
        ax.imshow(cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2RGB))
        ax.set_title(f"Frame {i + 1}")
        ax.axis('off')

    plt.suptitle(f"MiceCam Session: {reader.session_info['session_name']}")
    plt.tight_layout()

    output_path = Path(session_path).parent / f"{Path(session_path).name}_frames.png"
    plt.savefig(output_path, dpi=150)
    print(f"Saved visualization to: {output_path}")
    plt.close()


def plot_timestamps(session_path: str):
    """
    绘制时间戳分布

    Args:
        session_path: 会话路径
    """
    print(f"Plotting timestamps for {session_path}...")

    reader = MiceCamReader(session_path)

    # 收集时间戳
    timestamps = []
    for frame_info in reader.frames:
        # 转换为毫秒（相对于第一帧）
        ts_ns = frame_info['timestamp_ns']
        timestamps.append(ts_ns / 1_000_000)  # ns -> ms

    # 相对于第一帧
    start_time = timestamps[0]
    timestamps = [t - start_time for t in timestamps]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))

    # 时间戳分布
    ax1.plot(timestamps, 'o-', markersize=3)
    ax1.set_xlabel('Frame Index')
    ax1.set_ylabel('Time (ms)')
    ax1.set_title('Frame Timestamps')
    ax1.grid(True)

    # 帧间隔
    intervals = np.diff(timestamps)
    ax2.plot(intervals, 'o-', markersize=3)
    ax2.axhline(y=1000 / reader.fps, color='r', linestyle='--',
                label=f'Expected (1/{reader.fps}s = {1000/reader.fps:.2f}ms)')
    ax2.set_xlabel('Frame Index')
    ax2.set_ylabel('Interval (ms)')
    ax2.set_title('Frame Intervals')
    ax2.legend()
    ax2.grid(True)

    plt.suptitle(f"MiceCam Session: {reader.session_info['session_name']}")
    plt.tight_layout()

    output_path = Path(session_path).parent / f"{Path(session_path).name}_timestamps.png"
    plt.savefig(output_path, dpi=150)
    print(f"Saved plot to: {output_path}")
    plt.close()


def plot_statistics(session_path: str):
    """
    绘制统计信息

    Args:
        session_path: 会话路径
    """
    print(f"Analyzing statistics for {session_path}...")

    reader = MiceCamReader(session_path)

    # 收集帧大小
    frame_sizes = [f['size'] for f in reader.frames]

    fig, axes = plt.subplots(2, 2, figsize=(12, 8))

    # 帧大小分布
    axes[0, 0].hist(frame_sizes, bins=50, edgecolor='black')
    axes[0, 0].set_xlabel('Frame Size (bytes)')
    axes[0, 0].set_ylabel('Count')
    axes[0, 0].set_title('Frame Size Distribution')
    axes[0, 0].grid(True)

    # 帧大小趋势
    axes[0, 1].plot(frame_sizes, '-')
    axes[0, 1].set_xlabel('Frame Index')
    axes[0, 1].set_ylabel('Frame Size (bytes)')
    axes[0, 1].set_title('Frame Size Trend')
    axes[0, 1].grid(True)

    # 累积数据量
    cumulative = np.cumsum(frame_sizes)
    axes[1, 0].plot(cumulative / (1024 * 1024), '-')
    axes[1, 0].set_xlabel('Frame Index')
    axes[1, 0].set_ylabel('Cumulative Size (MB)')
    axes[1, 0].set_title('Cumulative Data Volume')
    axes[1, 0].grid(True)

    # 数据速率
    # 假设时间戳均匀分布
    total_time = reader.total_frames / reader.fps
    data_rate = (reader.total_bytes / (1024 * 1024)) / total_time  # MB/s
    axes[1, 1].bar(['Data Rate'], [data_rate])
    axes[1, 1].set_ylabel('MB/s')
    axes[1, 1].set_title(f'Average Data Rate: {data_rate:.2f} MB/s')
    axes[1, 1].grid(True)

    plt.suptitle(f"MiceCam Session: {reader.session_info['session_name']}")
    plt.tight_layout()

    output_path = Path(session_path).parent / f"{Path(session_path).name}_stats.png"
    plt.savefig(output_path, dpi=150)
    print(f"Saved plot to: {output_path}")
    plt.close()


def main():
    """命令行接口"""
    import sys

    if len(sys.argv) < 2:
        print("Usage: python visualize.py <session_path> [mode]")
        print("\nModes:")
        print("  frames    - Visualize first 10 frames")
        print("  timestamps - Plot timestamp distribution")
        print("  stats     - Plot statistics")
        print("\nExample:")
        print("  python visualize.py output/session_001 frames")
        sys.exit(1)

    session_path = sys.argv[1]
    mode = sys.argv[2] if len(sys.argv) > 2 else 'frames'

    try:
        import matplotlib
        matplotlib.use('Agg')  # 非交互式后端
    except ImportError:
        print("Error: matplotlib not installed. Run: pip install matplotlib")
        sys.exit(1)

    if mode == 'frames':
        visualize_frames(session_path)
    elif mode == 'timestamps':
        plot_timestamps(session_path)
    elif mode == 'stats':
        plot_statistics(session_path)
    else:
        print(f"Unknown mode: {mode}")
        print("Available modes: frames, timestamps, stats")
        sys.exit(1)


if __name__ == '__main__':
    main()
