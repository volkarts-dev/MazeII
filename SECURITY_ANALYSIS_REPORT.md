# Security Analysis Report - MazeII Codebase

**Date:** 2026-02-07  
**Scope:** Complete codebase under `/src` directory  
**Total Files Analyzed:** 93 C++ source files  
**Analysis Type:** Comprehensive security vulnerability assessment

---

## Executive Summary

A comprehensive security analysis was performed on the MazeII game engine codebase. The analysis identified **12 security vulnerabilities** across various severity levels:

- **Critical**: 3 vulnerabilities
- **High**: 5 vulnerabilities  
- **Medium**: 2 vulnerabilities
- **Low**: 2 vulnerabilities

The most severe issues involve integer overflow vulnerabilities in image and audio processing, unsafe type conversions, and assertions that are disabled in release builds allowing out-of-bounds memory access.

---

## Critical Severity Issues

### 1. Integer Overflow in Image Loading
**File:** `src/ngn/gfx/Image.cpp` (Lines 19, 56)  
**Type:** Integer Overflow / Buffer Overflow  
**CVE Category:** CWE-190 (Integer Overflow or Wraparound)

**Description:**
```cpp
const vk::DeviceSize imageSize = loader.width_ * loader.height_ * 4;
```

When loading images, the calculation of image buffer size performs multiplication of `width * height * 4` without overflow checking. For large dimensions (e.g., 65536 x 65536 x 4), this calculation can overflow, resulting in a smaller buffer allocation than required.

**Impact:**
- Buffer overflow leading to heap corruption
- Potential arbitrary code execution
- Denial of service through crash

**Recommendation:**
```cpp
// Check for overflow before multiplication
if (loader.width_ > 0 && loader.height_ > 0) {
    uint64_t imageSize64 = static_cast<uint64_t>(loader.width_) * 
                          static_cast<uint64_t>(loader.height_) * 4;
    if (imageSize64 > std::numeric_limits<vk::DeviceSize>::max())
        throw std::runtime_error("Image dimensions too large");
}
```

---

### 2. Sign Conversion Vulnerability in Audio Decoding
**File:** `src/ngn/audio/Audio.cpp` (Lines 49-50, 56, 68)  
**Type:** Unsafe Type Cast / Sign Conversion  
**CVE Category:** CWE-195 (Signed to Unsigned Conversion Error)

**Description:**
```cpp
reinterpret_cast<uint8_t*>(&a)[3 - i] = buffer[i];
```

The audio decoding functions perform unsafe array indexing with subtraction that could result in unsigned integer underflow when `i > 3`, potentially accessing memory outside the intended object.

**Impact:**
- Out-of-bounds memory read/write
- Information disclosure
- Potential code execution

**Recommendation:**
```cpp
if (i <= 3) {
    reinterpret_cast<uint8_t*>(&a)[3 - i] = buffer[i];
} else {
    throw std::runtime_error("Invalid buffer index");
}
```

---

### 3. Unchecked Assertions in StaticVector
**File:** `src/ngn/utils/StaticVector.hpp` (Lines 61-71)  
**Type:** Use-After-Free / Out-of-Bounds Access  
**CVE Category:** CWE-125 (Out-of-bounds Read), CWE-787 (Out-of-bounds Write)

**Description:**
```cpp
reference at(SizeT index) {
    assert(index < size_);  // Disabled in Release builds!
    return data_[index];
}
```

The `StaticVector` container uses assertions for bounds checking. Assertions are compiled out in release builds (when `NDEBUG` is defined), allowing unchecked out-of-bounds access.

**Impact:**
- Critical memory safety issue in production builds
- Out-of-bounds read/write operations
- Potential arbitrary code execution
- Information disclosure

**Recommendation:**
```cpp
reference at(SizeT index) {
    if (index >= size_)
        throw std::out_of_range("StaticVector index out of bounds");
    return data_[index];
}
```

---

## High Severity Issues

### 4. Missing Null Pointer Validation in Audio Buffer
**File:** `src/ngn/audio/Audio.cpp` (Lines 272-283)  
**Type:** Null Pointer Dereference  
**CVE Category:** CWE-476 (NULL Pointer Dereference)

**Description:**
After `stb_vorbis_decode_memory()` returns, the `buffer` pointer may be uninitialized on certain error paths, but is used and freed regardless.

**Impact:**
- Null pointer dereference causing crash
- Use-after-free if buffer is freed incorrectly
- Denial of service

**Recommendation:**
```cpp
if (samples < 0 || buffer == nullptr)
    throw std::runtime_error("Invalid ogg file or decoder failure");
```

---

### 5. Integer Overflow in Audio Buffer Size
**File:** `src/ngn/audio/Audio.cpp` (Line 280)  
**Type:** Integer Overflow  
**CVE Category:** CWE-190 (Integer Overflow)

**Description:**
```cpp
const auto bufferSize = static_cast<std::size_t>(samples * channels) * sizeof(short);
```

Multiplication of `samples * channels` occurs before casting to `size_t`, potentially causing overflow.

**Impact:**
- Undersized buffer allocation
- Heap corruption
- Memory corruption

**Recommendation:**
```cpp
if (samples > 0 && channels > 0) {
    if (samples > std::numeric_limits<std::size_t>::max() / (channels * sizeof(short)))
        throw std::runtime_error("Audio buffer size too large");
}
const auto bufferSize = static_cast<std::size_t>(samples) * 
                       static_cast<std::size_t>(channels) * sizeof(short);
```

---

### 6. Unchecked Array Access in FontMaker
**File:** `src/ngn/gfx/FontMaker.cpp` (Line 153)  
**Type:** Out-of-Bounds Array Access  
**CVE Category:** CWE-129 (Improper Validation of Array Index)

**Description:**
```cpp
uint32_t destPos = ((state.posY + y) * state.imageDimension + (state.posX + x)) * 4;
imageData[destPos + 0] = 255;
```

No bounds checking on the calculated `destPos` value before writing to the `imageData` vector.

**Impact:**
- Out-of-bounds write
- Heap corruption
- Potential code execution

**Recommendation:**
```cpp
uint64_t destPos64 = (static_cast<uint64_t>(state.posY + y) * 
                     state.imageDimension + (state.posX + x)) * 4;
if (destPos64 + 3 >= imageData.size())
    throw std::runtime_error("Glyph write out of bounds");
```

---

### 7. Integer Overflow in MemoryArena
**File:** `src/ngn/Allocators.cpp` (Line 29)  
**Type:** Integer Overflow  
**CVE Category:** CWE-190 (Integer Overflow)

**Description:**
```cpp
const auto start = align(top_, alignment);
const auto end = start + size;  // Could overflow
```

If `start + size` overflows, the result wraps around, making `end < capacity_`, bypassing the out-of-memory check.

**Impact:**
- Allocator returns pointer beyond buffer
- Heap corruption
- Memory corruption leading to potential code execution

**Recommendation:**
```cpp
if (start > capacity_ - size)  // Safe overflow check
    throw std::runtime_error("Out of memory or integer overflow");
```

---

### 8. Unsafe Integer Cast from stbi_load
**File:** `src/ngn/gfx/Image.cpp` (Lines 49-51)  
**Type:** Unsafe Type Cast  
**CVE Category:** CWE-681 (Incorrect Conversion between Numeric Types)

**Description:**
```cpp
const auto width = static_cast<uint32_t>(texWidth);  // texWidth is int
```

Casting negative `int` values to `uint32_t` produces very large unsigned values that can lead to overflow in subsequent calculations.

**Impact:**
- Integer overflow in buffer size calculation
- Heap corruption
- Potential code execution

**Recommendation:**
```cpp
if (texWidth <= 0 || texHeight <= 0 || texChannels <= 0)
    throw std::runtime_error("Invalid image dimensions");
if (texWidth > 65536 || texHeight > 65536)
    throw std::runtime_error("Image dimensions too large");
```

---

## Medium Severity Issues

### 9. Unsigned Underflow in remain()
**File:** `src/ngn/audio/Audio.cpp` (Line 27)  
**Type:** Integer Underflow  
**CVE Category:** CWE-191 (Integer Underflow)

**Description:**
```cpp
std::size_t remain() const {
    return data.size() - readPos;  // Underflow if readPos > data.size()
}
```

**Impact:**
- Returns incorrect large value
- Could lead to buffer over-read

**Recommendation:**
```cpp
return readPos >= data.size() ? 0 : (data.size() - readPos);
```

---

### 10. Off-by-One Error in read()
**File:** `src/ngn/audio/Audio.cpp` (Line 33)  
**Type:** Off-by-One Error  
**CVE Category:** CWE-193 (Off-by-one Error)

**Description:**
```cpp
if (readPos + bytes >= data.size())  // Should be >
    return {};
```

**Impact:**
- Prevents reading the last byte of buffer
- Logic error in boundary condition

**Recommendation:**
```cpp
if (readPos + bytes > data.size())
    return {};
```

---

## Low Severity Issues

### 11. Resource Leak on Exception
**File:** `src/ngn/audio/Audio.cpp` (Lines 272-295)  
**Type:** Memory Leak  
**CVE Category:** CWE-401 (Missing Release of Memory)

**Description:**
If an exception is thrown after `stb_vorbis_decode_memory()` allocates `buffer` but before it's freed, the memory leaks.

**Recommendation:**
Use RAII with `std::unique_ptr`:
```cpp
std::unique_ptr<short[], decltype(&std::free)> bufferPtr(buffer, &std::free);
```

---

### 12. Unsafe Pointer Arithmetic
**File:** `src/ngn/audio/Audio.cpp` (Line 68)  
**Type:** Unsafe Pointer Arithmetic  
**CVE Category:** CWE-129 (Improper Validation of Array Index)

**Description:**
```cpp
reinterpret_cast<uint8_t*>(&a)[3 - i] = buffer[i];
```

**Recommendation:**
Add bounds checking before array access.

---

## Remediation Priority

### Immediate (Critical)
1. Replace assertions with exceptions in `StaticVector.hpp`
2. Add overflow checks in `Image.cpp` image size calculations
3. Fix sign conversion issues in `Audio.cpp`

### Urgent (High)
4. Add null pointer validation in audio decoding
5. Fix integer overflow in audio buffer size calculation
6. Implement bounds checking in `FontMaker.cpp`
7. Add overflow check in `MemoryArena` allocation
8. Validate image dimensions in `Image.cpp` before casting

### Important (Medium)
9. Fix unsigned underflow in `remain()` method
10. Correct off-by-one error in `read()` method

### Routine (Low)
11. Implement RAII for audio buffer management
12. Add bounds checking for pointer arithmetic

---

## General Recommendations

1. **Enable compiler warnings**: Use `-Wall -Wextra -Werror` for stricter compilation
2. **Use static analysis tools**: Integrate clang-tidy and cppcheck
3. **Sanitizers**: Enable AddressSanitizer and UndefinedBehaviorSanitizer in debug builds
4. **Safe integer operations**: Consider using SafeInt library for checked arithmetic
5. **RAII**: Replace all manual memory management with smart pointers
6. **Bounds checking**: Enable bounds checking for STL containers in debug mode
7. **Code review**: Require security-focused code reviews for critical components

---

## Conclusion

The MazeII codebase contains several security vulnerabilities that require immediate attention, particularly in image and audio processing components. The most critical issues involve integer overflow conditions that could lead to buffer overflows and potential code execution. All critical and high severity issues should be addressed before production deployment.

**Risk Assessment**: **HIGH** - Multiple critical vulnerabilities affecting memory safety  
**Recommended Action**: Address all critical and high severity issues immediately

---

*Report generated by automated security analysis on 2026-02-07*
