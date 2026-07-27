# Bug: Zero Panel Visibility on AMD/Intel GPUs

## Symptom

When running the shading pipeline on any machine except the original developer's
(NVIDIA GPU), all panels report visibility = 0, regardless of satellite geometry
or flow direction.

## Root Cause

Both `BinaryShader` and `CoPShader` used an OpenGL compute shader to count how
many pixels each triangle ID occupied in the ID texture. The pipeline was:

1. Render satellite triangles (with per-triangle IDs as colours) into an FBO
   whose colour attachment is a `GL_R32UI` texture.
2. Issue a memory barrier.
3. Bind that same texture as a `uimage2D` image unit (`glBindImageTexture`).
4. Dispatch a compute shader that calls `imageLoad(framebuffer_texture, coord)`
   to read each pixel's ID and atomically increments an SSBO histogram.
5. Read the histogram back to the CPU to determine which triangle IDs had at
   least one visible pixel.

**The bug:** `imageLoad` from a `uimage2D` image unit returned **0 for every
pixel** on AMD and Intel GPUs, even though the texture clearly contained
non-zero data (confirmed by `glReadPixels` reading the same texture through the
framebuffer path).

NVIDIA GPUs happened to work because their driver is more lenient and flushes
all internal caches on any barrier call, accidentally hiding the issue.

### Why `imageLoad` returned zeros

The OpenGL specification treats framebuffer writes (fragment shader output →
FBO colour attachment) and image-unit reads (`imageLoad` in a compute shader)
as **different memory access domains**. A memory barrier must explicitly cover
the transition between these two domains.

`GL_FRAMEBUFFER_BARRIER_BIT` only guarantees that framebuffer writes are visible
to subsequent **framebuffer** reads/writes — not to `imageLoad` in compute
shaders.

`GL_SHADER_IMAGE_ACCESS_BARRIER_BIT` covers **image-unit** reads/writes that
follow prior **image-unit** writes — but the writes in this pipeline were done
via the **framebuffer**, not via `imageStore`.

No single standard barrier bit in OpenGL < 4.5 is specified to bridge
*framebuffer writes → imageLoad reads*. NVIDIA flushes everything anyway;
AMD and Intel honour the spec precisely and leave the image cache stale.

Even `GL_ALL_BARRIER_BITS` did not fix the issue, which indicates this is a
driver-level limitation: the AMD/Intel OpenGL driver simply does not expose
valid data through `imageLoad` for a texture that was written via an FBO
attachment in this configuration.

## Fix

Replace the compute-shader / `imageLoad` / SSBO histogram path with a
**CPU-side `glReadPixels`** readback. `glReadPixels` reads through the
framebuffer path, which is guaranteed by the spec to see the just-rendered
data after `GL_FRAMEBUFFER_BARRIER_BIT`.

### Files to change

#### `aero_sat/shading_pipeline/binary_shader/binary_shader.cpp`

**In `set_vertices`:** remove the histogram SSBO allocation and the
`ComputeShader` creation.

```cpp
// DELETE these lines:
GLCall(glGenBuffers(1, &m_histogramBuffer));
GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_histogramBuffer));
GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER,
       (m_numTriangles + 1) * sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW));
GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));

m_compute_shader.reset(new ComputeShader(Compute_shader, true));
m_compute_shader->Unbind();
```

**In `~BinaryShader`:** remove the histogram buffer and compute shader cleanup.

```cpp
// DELETE these lines:
m_compute_shader.reset();
if (m_histogramBuffer != 0) {
    GLCall(glDeleteBuffers(1, &m_histogramBuffer));
}
```

**In `shade_satellite`:** replace everything from the memory barrier onward with:

```cpp
// Replace the old barrier + image-unit + compute-shader block with:
GLCall(glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT));
GLCall(glReadBuffer(GL_COLOR_ATTACHMENT0));
std::vector<GLuint> pixel_ids(NUM_PIXEL * NUM_PIXEL, 0);
GLCall(glReadPixels(0, 0, NUM_PIXEL, NUM_PIXEL,
                   GL_RED_INTEGER, GL_UNSIGNED_INT, pixel_ids.data()));
m_frame_buffer->UnBind();

std::vector<bool> seen(m_numTriangles + 1, false);
for (GLuint id : pixel_ids) {
    if (id > 0 && id <= m_numTriangles) seen[id] = true;
}
for (size_t i = 0; i < triangle_visibility.size() && i < m_numTriangles; i++) {
    if (seen[i + 1]) triangle_visibility[i] = 1.0f;
}
```

#### `aero_sat/shading_pipeline/cop_shader/cop_shader.cpp`

Apply the identical replacement to the same section in `shade_satellite`.
The `CoPShader` uses a more refined rendering approach (renders triangles with
ID = 0 to set up depth, then renders triangle centroids as GL_POINTS with real
IDs), but the histogram readback is the same broken code path.

**In `~CoPShader`** and **`set_vertices`:** remove the histogram SSBO and
`ComputeShader` the same way as for `BinaryShader`.

**Secondary bug also fixed in `cop_shader.cpp`:** in the CoP rendering loop,
`m_shader->setUniformMat4f("u_MVP", u_MVP)` was called while `m_point_shader`
was the active program. Change to `m_point_shader->setUniformMat4f(...)`.

## Additional includes

`binary_shader.cpp` and `cop_shader.cpp` need `#include <vector>` for
`std::vector<GLuint>` and `std::vector<bool>` if not already present.

## Why only the developer's machine worked

The original developer uses an NVIDIA GPU. NVIDIA's OpenGL driver flushes all
GPU caches on every `glMemoryBarrier` call, regardless of which bits are set.
This accidentally made the stale image-unit data coherent on NVIDIA. On AMD and
Intel, the driver precisely honours the spec and only flushes the caches
indicated by the barrier bits — leaving the `imageLoad` path reading zeros.
