"""
MiceCam SDK Python Bindings

High-speed camera acquisition library for scientific imaging.

Example:
    import micecam

    def on_frame(data, seq_id, timestamp):
        print(f"Frame {seq_id}: {len(data)} bytes @ {timestamp:.3f}s")

    with micecam.Pipeline(backend="usb", width=1920, height=1080, fps=30) as pipeline:
        pipeline.attach_callback(on_frame)
        time.sleep(5)  # Record for 5 seconds
"""

from ._micecam import Pipeline, PixelFormat, __version__

__all__ = ['Pipeline', 'PixelFormat', '__version__']
