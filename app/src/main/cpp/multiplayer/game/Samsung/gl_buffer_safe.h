#ifndef GL_BUFFER_SAFE_H
#define GL_BUFFER_SAFE_H

#include <cstdint>

namespace GlBufferSafe {
    void BindArrayBufferFromRw(uint32_t buffer_ptr);
    void DeleteArrayBufferFromRw(uint32_t* buffer);
}

#endif // GL_BUFFER_SAFE_H
