#!/usr/bin/env python3
"""
MiceCam Binary Data Reader

读取 MiceCam 采集的 .bin 文件和 JSON 元数据。
"""

import json
import numpy as np
from pathlib import Path
from typing import Tuple, Dict, List, Optional


class MiceCamReader:
    """MiceCam 数据读取器"""

    def __init__(self, session_path: str):
        """
        初始化读取器

        Args:
            session_path: 会话路径（不含扩展名或包含扩展名均可）
                          例如: "output/session_001" 或 "output/session_001.bin"
        """
        self.session_path = Path(session_path)

        # 处理路径：移除 .bin 后缀（如果有）
        if self.session_path.suffix == '.bin':
            self.session_path = self.session_path.with_suffix('')

        self.bin_path = self.session_path.with_suffix('.bin')
        # 元数据文件是 session_name_metadata.json
        self.metadata_path = self.session_path.parent / (self.session_path.name + '_metadata.json')

        # 验证文件存在
        if not self.bin_path.exists():
            raise FileNotFoundError(f"Binary file not found: {self.bin_path}")
        if not self.metadata_path.exists():
            raise FileNotFoundError(f"Metadata file not found: {self.metadata_path}")

        # 加载元数据
        with open(self.metadata_path, 'r') as f:
            self.metadata = json.load(f)

        self.session_info = self.metadata['session']
        self.frames = self.metadata['frames']

        # 相机参数
        self.width = self.session_info['width']
        self.height = self.session_info['height']
        self.fps = self.session_info['fps']
        self.total_frames = self.session_info['total_frames']
        self.total_bytes = self.session_info['total_bytes']

        # 验证数据完整性
        self._validate()

    def _validate(self):
        """验证数据完整性"""
        if len(self.frames) != self.total_frames:
            print(f"Warning: Metadata says {self.total_frames} frames, "
                  f"but JSON contains {len(self.frames)} records")

        # 检查校验和（如果启用）
        if self.session_info.get('session_checksum', 0) != 0:
            print(f"Session checksum: {self.session_info['session_checksum']}")

    def get_frame(self, frame_id: int) -> Optional[np.ndarray]:
        """
        获取指定帧（1-based index）

        Args:
            frame_id: 帧序号（从 1 开始）

        Returns:
            numpy array (height, width, channels) 或 None
        """
        if frame_id < 1 or frame_id > len(self.frames):
            print(f"Error: Frame {frame_id} out of range [1, {len(self.frames)}]")
            return None

        frame_info = self.frames[frame_id - 1]

        # 读取帧数据
        with open(self.bin_path, 'rb') as f:
            f.seek(frame_info['offset'])
            frame_data = f.read(frame_info['size'])

        # 转换为 numpy array
        frame = np.frombuffer(frame_data, dtype=np.uint8)

        # 重塑为图像 (假设 RGB)
        # TODO: 根据 channels 参数调整
        if frame.size == self.width * self.height * 3:
            frame = frame.reshape(self.height, self.width, 3)
        elif frame.size == self.width * self.height:
            frame = frame.reshape(self.height, self.width)
        else:
            print(f"Warning: Unexpected frame size {frame.size}")
            frame = frame.reshape(-1)

        return frame

    def get_frames(self, start: int = 1, end: Optional[int] = None) -> List[np.ndarray]:
        """
        获取多帧

        Args:
            start: 起始帧序号（从 1 开始）
            end: 结束帧序号（包含），None 表示到最后一帧

        Returns:
            帧列表
        """
        if end is None:
            end = len(self.frames)

        frames = []
        for i in range(start, end + 1):
            frame = self.get_frame(i)
            if frame is not None:
                frames.append(frame)

        return frames

    def get_all_frames(self) -> np.ndarray:
        """
        获取所有帧（返回 numpy array）

        Returns:
            numpy array (total_frames, height, width, channels)
        """
        frames = self.get_frames(1, len(self.frames))

        if len(frames) == 0:
            return np.array([])

        # 堆叠为 4D array
        return np.stack(frames)

    def get_frame_timestamp(self, frame_id: int) -> int:
        """获取帧时间戳（纳秒）"""
        if frame_id < 1 or frame_id > len(self.frames):
            return 0
        return self.frames[frame_id - 1]['timestamp_ns']

    def get_frame_info(self, frame_id: int) -> Dict:
        """获取帧元数据"""
        if frame_id < 1 or frame_id > len(self.frames):
            return {}
        return self.frames[frame_id - 1]

    def summary(self) -> str:
        """打印会话摘要"""
        return f"""
MiceCam Session Summary
=======================
Name: {self.session_info['session_name']}
Camera: {self.session_info['camera_backend']}
Resolution: {self.width}x{self.height}
FPS: {self.fps}
Total Frames: {self.total_frames}
Total Bytes: {self.total_bytes:,}
Duration: {self.total_frames / self.fps:.2f} seconds
Start: {self.session_info['start_timestamp_ns']}
End: {self.session_info['end_timestamp_ns']}
Checksum: {self.session_info.get('session_checksum', 'N/A')}
"""


def main():
    """命令行接口"""
    import sys

    if len(sys.argv) < 2:
        print("Usage: python read_bin.py <session_path>")
        print("Example: python read_bin.py output/session_001")
        sys.exit(1)

    reader = MiceCamReader(sys.argv[1])

    # 打印摘要
    print(reader.summary())

    # 读取第一帧
    frame = reader.get_frame(1)
    if frame is not None:
        print(f"\nFrame 1 shape: {frame.shape}")
        print(f"Frame 1 dtype: {frame.dtype}")
        print(f"Frame 1 timestamp: {reader.get_frame_timestamp(1)}")
        print(f"Frame 1 info: {reader.get_frame_info(1)}")


if __name__ == '__main__':
    main()
