/**
 * @file nv12_to_rgba.cu
 * @brief CUDA kernel for NV12 to RGBA color space conversion
 *
 * This kernel converts NV12 (YUV 4:2:0) frames from NVDEC output
 * to RGBA format suitable for OpenGL rendering.
 *
 * NV12 Layout:
 *   - Y plane: W x H bytes (luma)
 *   - UV plane: W x H/2 bytes (interleaved chroma)
 */

#include <cuda_runtime.h>
#include <cstdint>

namespace micecam {

/**
 * @brief CUDA kernel for NV12 to RGBA conversion.
 *
 * Each thread processes one pixel. The UV values are shared between
 * 2x2 pixel blocks due to 4:2:0 subsampling.
 *
 * @param nv12_y Pointer to Y plane
 * @param nv12_uv Pointer to interleaved UV plane
 * @param rgba Output RGBA buffer
 * @param width Frame width
 * @param height Frame height
 * @param y_pitch Pitch of Y plane in bytes
 * @param uv_pitch Pitch of UV plane in bytes
 */
__global__ void NV12toRGBA(
    const uint8_t* __restrict__ nv12_y,
    const uint8_t* __restrict__ nv12_uv,
    uint8_t* __restrict__ rgba,
    int width,
    int height,
    int y_pitch,
    int uv_pitch)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= width || y >= height) return;

    // Read Y value
    int y_idx = y * y_pitch + x;
    float Y = static_cast<float>(nv12_y[y_idx]);

    // Read UV values (subsampled 2:1 in both dimensions)
    int uv_x = x / 2;
    int uv_y = y / 2;
    int uv_idx = uv_y * uv_pitch + uv_x * 2;

    float U = static_cast<float>(nv12_uv[uv_idx]) - 128.0f;
    float V = static_cast<float>(nv12_uv[uv_idx + 1]) - 128.0f;

    // BT.601 YUV to RGB conversion
    // R = Y + 1.402 * V
    // G = Y - 0.344 * U - 0.714 * V
    // B = Y + 1.772 * U

    float R = Y + 1.402f * V;
    float G = Y - 0.344f * U - 0.714f * V;
    float B = Y + 1.772f * U;

    // Clamp to [0, 255]
    R = fminf(fmaxf(R, 0.0f), 255.0f);
    G = fminf(fmaxf(G, 0.0f), 255.0f);
    B = fminf(fmaxf(B, 0.0f), 255.0f);

    // Write RGBA output
    int rgba_idx = (y * width + x) * 4;
    rgba[rgba_idx + 0] = static_cast<uint8_t>(R);
    rgba[rgba_idx + 1] = static_cast<uint8_t>(G);
    rgba[rgba_idx + 2] = static_cast<uint8_t>(B);
    rgba[rgba_idx + 3] = 255; // Alpha
}

/**
 * @brief Launch NV12 to RGBA conversion kernel.
 *
 * @param nv12_y Device pointer to Y plane
 * @param nv12_uv Device pointer to UV plane
 * @param rgba Device pointer to output RGBA buffer
 * @param width Frame width
 * @param height Frame height
 * @param y_pitch Y plane pitch
 * @param uv_pitch UV plane pitch
 * @param stream CUDA stream for async execution
 */
extern "C" void launchNV12toRGBA(
    const uint8_t* nv12_y,
    const uint8_t* nv12_uv,
    uint8_t* rgba,
    int width,
    int height,
    int y_pitch,
    int uv_pitch,
    cudaStream_t stream)
{
    dim3 block(16, 16);
    dim3 grid((width + block.x - 1) / block.x, (height + block.y - 1) / block.y);

    NV12toRGBA<<<grid, block, 0, stream>>>(
        nv12_y, nv12_uv, rgba, width, height, y_pitch, uv_pitch
    );
}

} // namespace micecam
