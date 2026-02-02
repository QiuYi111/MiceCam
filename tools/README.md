# MiceCam Tools
# Python 数据处理工具集

工具列表：
- read_bin.py         - 读取 .bin + JSON 元数据
- convert.py          - 转换格式（NumPy, HDF5, Video）
- visualize.py        - 数据可视化

## 依赖安装

```bash
# 基础依赖（必需）
pip install numpy

# HDF5 转换（可选）
pip install h5py

# 视频导出（可选）
pip install opencv-python

# 可视化（可选）
pip install matplotlib
```

## 使用示例

### 读取数据

```bash
# 查看会话摘要
python tools/read_bin.py test_output/session_001

# 在 Python 中使用
python
>>> from tools.read_bin import MiceCamReader
>>> reader = MiceCamReader('test_output/session_001')
>>> print(reader.summary())
>>> frame = reader.get_frame(1)
>>> print(frame.shape)
```

### 转换格式

```bash
# 转换为 NumPy
ln -s tools/convert.py tools/convert_to_numpy.py
python tools/convert_to_numpy.py test_output/session_001

# 转换为 HDF5
ln -s tools/convert.py tools/convert_to_hdf5.py
python tools/convert_to_hdf5.py test_output/session_001

# 导出为视频
ln -s tools/convert.py tools/export_to_video.py
python tools/export_to_video.py test_output/session_001
```

### 可视化

```bash
# 查看前几帧
python tools/visualize.py test_output/session_001 frames

# 时间戳分布
python tools/visualize.py test_output/session_001 timestamps

# 统计信息
python tools/visualize.py test_output/session_001 stats
```

## 快速开始

```python
# 1. 读取数据
from tools.read_bin import MiceCamReader

reader = MiceCamReader('test_output/session_001')
print(reader.summary())

# 2. 获取所有帧
frames = reader.get_all_frames()
print(f"Loaded {len(frames)} frames")
print(f"Shape: {frames.shape}")

# 3. 保存为 NumPy
import numpy as np
np.save('session_001.npy', frames)

# 4. 或直接导出为 HDF5
from tools.convert import convert_to_hdf5
convert_to_hdf5('test_output/session_001')
```

## 注意事项

- 确保已运行 MiceCam 采集并生成了 .bin 和 _metadata.json 文件
- HDF5 转换需要安装 h5py
- 视频导出需要安装 opencv-python
- 可视化需要安装 matplotlib
