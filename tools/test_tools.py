#!/usr/bin/env python3
"""
MiceCam Tools Test

测试所有 Python 工具是否正常工作
"""

import sys
import os

# 添加 tools 目录到路径
sys.path.insert(0, 'tools')

def test_read_bin():
    """测试读取工具"""
    print("Testing read_bin.py...")
    from read_bin import MiceCamReader

    try:
        reader = MiceCamReader('test_output/buffer_size_test')
        summary = reader.summary()
        print("✓ read_bin.py works")
        return True
    except Exception as e:
        print(f"✗ read_bin.py failed: {e}")
        return False


def test_get_frame():
    """测试获取帧"""
    print("\nTesting get_frame()...")
    from read_bin import MiceCamReader

    try:
        reader = MiceCamReader('test_output/buffer_size_test')
        frame = reader.get_frame(1)
        assert frame is not None
        assert frame.shape == (240, 320, 3)
        print(f"✓ get_frame() works, frame shape: {frame.shape}")
        return True
    except Exception as e:
        print(f"✗ get_frame() failed: {e}")
        return False


def test_get_all_frames():
    """测试获取所有帧"""
    print("\nTesting get_all_frames()...")
    from read_bin import MiceCamReader

    try:
        reader = MiceCamReader('test_output/buffer_size_test')
        frames = reader.get_all_frames()
        assert len(frames) == 50
        assert frames.shape[0] == 50
        print(f"✓ get_all_frames() works, loaded {len(frames)} frames")
        return True
    except Exception as e:
        print(f"✗ get_all_frames() failed: {e}")
        return False


def test_numpy_conversion():
    """测试 NumPy 转换"""
    print("\nTesting NumPy conversion...")
    import numpy as np
    from read_bin import MiceCamReader

    try:
        reader = MiceCamReader('test_output/buffer_size_test')
        frames = reader.get_all_frames()

        # 保存
        output_path = 'test_output/session_test.npy'
        np.save(output_path, frames)

        # 验证
        loaded = np.load(output_path)
        assert loaded.shape == frames.shape
        assert np.array_equal(loaded, frames)

        # 清理
        os.remove(output_path)

        print(f"✓ NumPy conversion works")
        return True
    except Exception as e:
        print(f"✗ NumPy conversion failed: {e}")
        return False


def test_metadata():
    """测试元数据读取"""
    print("\nTesting metadata reading...")
    from read_bin import MiceCamReader

    try:
        reader = MiceCamReader('test_output/buffer_size_test')

        # 验证会话信息
        assert reader.session_info['session_name'] == 'buffer_size_test'
        assert reader.session_info['width'] == 320
        assert reader.session_info['height'] == 240
        assert reader.session_info['fps'] == 30.0
        assert reader.session_info['total_frames'] == 50

        # 验证帧元数据
        frame_info = reader.get_frame_info(1)
        assert frame_info['sequence_id'] == 1
        assert frame_info['size'] == 230400

        print("✓ Metadata reading works")
        return True
    except Exception as e:
        print(f"✗ Metadata reading failed: {e}")
        return False


def main():
    """运行所有测试"""
    print("=" * 60)
    print("MiceCam Tools Test Suite")
    print("=" * 60)

    tests = [
        test_read_bin,
        test_get_frame,
        test_get_all_frames,
        test_numpy_conversion,
        test_metadata,
    ]

    results = []
    for test in tests:
        results.append(test())

    print("\n" + "=" * 60)
    passed = sum(results)
    total = len(results)

    if passed == total:
        print(f"✓ All {total} tests passed!")
        return 0
    else:
        print(f"✗ {total - passed} / {total} tests failed")
        return 1


if __name__ == '__main__':
    sys.exit(main())
