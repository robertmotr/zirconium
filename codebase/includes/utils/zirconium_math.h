#pragma once

#include "pch.h"

namespace zirconium {

    // =========================================================================
    //  vector<N, T>
    // =========================================================================
    /**
     * @brief   Fixed-size numeric vector with guaranteed contiguous layout.
     * @tparam  N  Number of elements (> 0).
     * @tparam  T  Element type (must be arithmetic).
     *
     * @details The struct contains only a plain C array, so
     *          sizeof(vector<3,float>) == 3 * sizeof(float) with no padding.
     *          This makes it safe to cast directly from game memory pointers.
     */
    template<std::size_t N, class T>
    struct vector {
        static_assert(N > 0, "vector size must be > 0");
        static_assert(std::is_arithmetic_v<T>, "vector element type must be arithmetic");

        T elements[N];

        template<std::size_t M = N, class = std::enable_if_t<(M >= 1)>>
        constexpr T& x() { return elements[0]; }

        template<std::size_t M = N, class = std::enable_if_t<(M >= 1)>>
        constexpr const T& x() const { return elements[0]; }

        template<std::size_t M = N, class = std::enable_if_t<(M >= 2)>>
        constexpr T& y() { return elements[1]; }

        template<std::size_t M = N, class = std::enable_if_t<(M >= 2)>>
        constexpr const T& y() const { return elements[1]; }

        template<std::size_t M = N, class = std::enable_if_t<(M >= 3)>>
        constexpr T& z() { return elements[2]; }

        template<std::size_t M = N, class = std::enable_if_t<(M >= 3)>>
        constexpr const T& z() const { return elements[2]; }

        template<std::size_t M = N, class = std::enable_if_t<(M >= 4)>>
        constexpr T& w() { return elements[3]; }

        template<std::size_t M = N, class = std::enable_if_t<(M >= 4)>>
        constexpr const T& w() const { return elements[3]; }

        /**
         * @brief  Subscript operator for element access.
         * @param  idx  Zero-based element index.
         * @return Reference to the element at @p idx.
         */
        constexpr T& operator[](std::size_t idx) { return elements[idx]; }

        /** @copydoc operator[]() */
        constexpr const T& operator[](std::size_t idx) const { return elements[idx]; }

        // ----- element-wise arithmetic -----

        /**
         * @brief  Element-wise addition of two vectors.
         * @param  rhs  Right-hand operand.
         * @return New vector with each element summed.
         */
        constexpr vector operator+(const vector& rhs) const {
            vector out{};
            for (std::size_t i = 0; i < N; ++i) out[i] = elements[i] + rhs[i];
            return out;
        }

        /**
         * @brief  Element-wise subtraction of two vectors.
         * @param  rhs  Right-hand operand.
         * @return New vector with each element subtracted.
         */
        constexpr vector operator-(const vector& rhs) const {
            vector out{};
            for (std::size_t i = 0; i < N; ++i) out[i] = elements[i] - rhs[i];
            return out;
        }

        /**
         * @brief  Element-wise (Hadamard) multiplication of two vectors.
         * @param  rhs  Right-hand operand.
         * @return New vector with each element multiplied.
         */
        constexpr vector operator*(const vector& rhs) const {
            vector out{};
            for (std::size_t i = 0; i < N; ++i) out[i] = elements[i] * rhs[i];
            return out;
        }

        /**
         * @brief  Element-wise division of two vectors.
         * @param  rhs  Right-hand operand.
         * @return New vector with each element divided.
         */
        constexpr vector operator/(const vector& rhs) const {
            vector out{};
            for (std::size_t i = 0; i < N; ++i) out[i] = elements[i] / rhs[i];
            return out;
        }

        // ----- scalar arithmetic -----

        /**
         * @brief  Add a scalar to every element.
         * @param  scalar  Scalar value.
         * @return New vector with the scalar added to each element.
         */
        constexpr vector operator+(T scalar) const {
            vector out{};
            for (std::size_t i = 0; i < N; ++i) out[i] = elements[i] + scalar;
            return out;
        }

        /**
         * @brief  Subtract a scalar from every element.
         * @param  scalar  Scalar value.
         * @return New vector with the scalar subtracted from each element.
         */
        constexpr vector operator-(T scalar) const {
            vector out{};
            for (std::size_t i = 0; i < N; ++i) out[i] = elements[i] - scalar;
            return out;
        }

        /**
         * @brief  Multiply every element by a scalar.
         * @param  scalar  Scalar value.
         * @return New vector with each element scaled.
         */
        constexpr vector operator*(T scalar) const {
            vector out{};
            for (std::size_t i = 0; i < N; ++i) out[i] = elements[i] * scalar;
            return out;
        }

        /**
         * @brief  Divide every element by a scalar.
         * @param  scalar  Scalar value.
         * @return New vector with each element divided.
         */
        constexpr vector operator/(T scalar) const {
            vector out{};
            for (std::size_t i = 0; i < N; ++i) out[i] = elements[i] / scalar;
            return out;
        }

        // ----- compound assignment -----

        /** @brief  In-place element-wise addition. */
        constexpr vector& operator+=(const vector& rhs) { return *this = *this + rhs; }

        /** @brief  In-place element-wise subtraction. */
        constexpr vector& operator-=(const vector& rhs) { return *this = *this - rhs; }

        /** @brief  In-place element-wise multiplication. */
        constexpr vector& operator*=(const vector& rhs) { return *this = *this * rhs; }

        /** @brief  In-place element-wise division. */
        constexpr vector& operator/=(const vector& rhs) { return *this = *this / rhs; }

        /** @brief  In-place scalar addition. */
        constexpr vector& operator+=(T scalar) { return *this = *this + scalar; }

        /** @brief  In-place scalar subtraction. */
        constexpr vector& operator-=(T scalar) { return *this = *this - scalar; }

        /** @brief  In-place scalar multiplication. */
        constexpr vector& operator*=(T scalar) { return *this = *this * scalar; }

        /** @brief  In-place scalar division. */
        constexpr vector& operator/=(T scalar) { return *this = *this / scalar; }

        // ----- comparison -----

        /**
         * @brief  Exact element-wise equality test.
         * @param  rhs  Vector to compare against.
         * @return true if every element matches.
         */
        constexpr bool operator==(const vector& rhs) const {
            for (std::size_t i = 0; i < N; ++i)
                if (elements[i] != rhs[i]) return false;
            return true;
        }

        /**
         * @brief  Exact element-wise inequality test.
         * @param  rhs  Vector to compare against.
         * @return true if any element differs.
         */
        constexpr bool operator!=(const vector& rhs) const { return !(*this == rhs); }

        // ----- unary negate -----

        /**
         * @brief  Negate every element.
         * @return New vector with each element negated.
         */
        constexpr vector operator-() const {
            vector out{};
            for (std::size_t i = 0; i < N; ++i) out[i] = -elements[i];
            return out;
        }

        // ----- print -----

        /**
         * @brief  Print the vector contents to stdout for debugging.
         * @param  label  Optional label printed before the values (may be nullptr).
         *
         * @details Floating-point elements are printed to 4 decimal places.
         *          Integer elements are printed as signed 64-bit.
         *          Output format: "vec3(1.0000, 2.0000, 3.0000)\n"
         */
        inline void math_print(const char* label = nullptr) const {
            if (label) std::printf("%s: ", label);
            std::printf("vec%zu(", N);
            for (std::size_t i = 0; i < N; ++i) {
                if constexpr (std::is_floating_point_v<T>)
                    std::printf("%.4f", static_cast<double>(elements[i]));
                else
                    std::printf("%lld", static_cast<long long>(elements[i]));
                if (i + 1 < N) std::printf(", ");
            }
            std::printf(")\n");
        }

        // ----- read / write (raw memcpy) -----

        /**
         * @brief  Copy raw bytes from @p src into this vector's elements.
         * @param  src  Pointer to at least sizeof(elements) bytes.
         */
        inline void math_readFrom(const void* src) {
            std::memcpy(elements, src, sizeof(elements));
        }

        /**
         * @brief  Copy this vector's raw bytes into @p dst.
         * @param  dst  Pointer to at least sizeof(elements) writable bytes.
         */
        inline void math_writeTo(void* dst) const {
            std::memcpy(dst, elements, sizeof(elements));
        }

        /**
         * @brief  Construct a vector by copying raw bytes from @p src.
         * @param  src  Pointer to at least sizeof(vector) bytes.
         * @return New vector with contents copied from the source.
         */
        static inline vector math_read(const void* src) {
            vector result{};
            std::memcpy(&result, src, sizeof(result));
            return result;
        }
    };

    /**
     * @brief  Left-hand scalar * vector multiplication.
     * @param  scalar  Scalar value.
     * @param  rhs     Vector operand.
     * @return New vector with each element scaled.
     */
    template<std::size_t N, class T>
    constexpr vector<N, T> operator*(T scalar, const vector<N, T>& rhs) { return rhs * scalar; }

    // ----- vector layout guarantees -----

    static_assert(sizeof(vector<2, float>)    == 2 * sizeof(float),    "vec2f layout broken");
    static_assert(sizeof(vector<3, float>)    == 3 * sizeof(float),    "vec3f layout broken");
    static_assert(sizeof(vector<4, float>)    == 4 * sizeof(float),    "vec4f layout broken");
    static_assert(sizeof(vector<2, int32_t>)  == 2 * sizeof(int32_t),  "vec2i layout broken");
    static_assert(sizeof(vector<3, int32_t>)  == 3 * sizeof(int32_t),  "vec3i layout broken");
    static_assert(sizeof(vector<4, int32_t>)  == 4 * sizeof(int32_t),  "vec4i layout broken");

    static_assert(std::is_standard_layout_v<vector<3, float>>,      "vec3f must be standard layout");
    static_assert(std::is_trivially_copyable_v<vector<3, float>>,   "vec3f must be trivially copyable");
    static_assert(std::is_standard_layout_v<vector<4, float>>,      "vec4f must be standard layout");
    static_assert(std::is_trivially_copyable_v<vector<4, float>>,   "vec4f must be trivially copyable");

    // =========================================================================
    //  matrix<R, C, T>  --  row-major storage
    // =========================================================================

    /**
     * @brief   Fixed-size row-major matrix with guaranteed contiguous layout.
     * @tparam  R  Number of rows (must be > 0).
     * @tparam  C  Number of columns (must be > 0).
     * @tparam  T  Element type (must be arithmetic).
     *
     * @details Storage is T[R][C] with no padding.
     *          sizeof(matrix<4,4,float>) == 64.
     */
    template<std::size_t R, std::size_t C, class T>
    struct matrix {
        static_assert(R > 0 && C > 0, "matrix dimensions must be > 0");
        static_assert(std::is_arithmetic_v<T>, "matrix element type must be arithmetic");

        T elements[R][C];

        // ----- row / element access -----

        /**
         * @brief  Row subscript -- returns a pointer to the first column in
         *         the given row, allowing m[r][c] syntax.
         * @param  row  Zero-based row index.
         * @return Pointer to the start of the requested row.
         */
        constexpr T* operator[](std::size_t row) { return elements[row]; }

        /** @copydoc operator[]() */
        constexpr const T* operator[](std::size_t row) const { return elements[row]; }

        /**
         * @brief  Named element accessor.
         * @param  row  Zero-based row index.
         * @param  col  Zero-based column index.
         * @return Reference to the element at (row, col).
         */
        constexpr T& at(std::size_t row, std::size_t col) { return elements[row][col]; }

        /** @copydoc at() */
        constexpr const T& at(std::size_t row, std::size_t col) const { return elements[row][col]; }

        // ----- element-wise arithmetic -----

        /**
         * @brief  Element-wise matrix addition.
         * @param  rhs  Right-hand operand (same dimensions).
         * @return New matrix with each element summed.
         */
        constexpr matrix operator+(const matrix& rhs) const {
            matrix out{};
            for (std::size_t r = 0; r < R; ++r)
                for (std::size_t c = 0; c < C; ++c)
                    out[r][c] = elements[r][c] + rhs[r][c];
            return out;
        }

        /**
         * @brief  Element-wise matrix subtraction.
         * @param  rhs  Right-hand operand (same dimensions).
         * @return New matrix with each element subtracted.
         */
        constexpr matrix operator-(const matrix& rhs) const {
            matrix out{};
            for (std::size_t r = 0; r < R; ++r)
                for (std::size_t c = 0; c < C; ++c)
                    out[r][c] = elements[r][c] - rhs[r][c];
            return out;
        }

        /**
         * @brief  Multiply every element by a scalar.
         * @param  scalar  Scalar value.
         * @return New matrix with each element scaled.
         */
        constexpr matrix operator*(T scalar) const {
            matrix out{};
            for (std::size_t r = 0; r < R; ++r)
                for (std::size_t c = 0; c < C; ++c)
                    out[r][c] = elements[r][c] * scalar;
            return out;
        }

        /**
         * @brief  Divide every element by a scalar.
         * @param  scalar  Scalar value.
         * @return New matrix with each element divided.
         */
        constexpr matrix operator/(T scalar) const {
            matrix out{};
            for (std::size_t r = 0; r < R; ++r)
                for (std::size_t c = 0; c < C; ++c)
                    out[r][c] = elements[r][c] / scalar;
            return out;
        }

        // ----- compound assignment -----

        /** @brief  In-place element-wise addition. */
        constexpr matrix& operator+=(const matrix& rhs) { return *this = *this + rhs; }

        /** @brief  In-place element-wise subtraction. */
        constexpr matrix& operator-=(const matrix& rhs) { return *this = *this - rhs; }

        /** @brief  In-place scalar multiplication. */
        constexpr matrix& operator*=(T scalar) { return *this = *this * scalar; }

        /** @brief  In-place scalar division. */
        constexpr matrix& operator/=(T scalar) { return *this = *this / scalar; }

        // ----- comparison -----

        /**
         * @brief  Exact element-wise equality test.
         * @param  rhs  Matrix to compare against.
         * @return true if every element matches.
         */
        constexpr bool operator==(const matrix& rhs) const {
            for (std::size_t r = 0; r < R; ++r)
                for (std::size_t c = 0; c < C; ++c)
                    if (elements[r][c] != rhs[r][c]) return false;
            return true;
        }

        /**
         * @brief  Exact element-wise inequality test.
         * @param  rhs  Matrix to compare against.
         * @return true if any element differs.
         */
        constexpr bool operator!=(const matrix& rhs) const { return !(*this == rhs); }

        // ----- identity (square matrices only) -----

        /**
         * @brief  Construct an identity matrix (ones on the diagonal).
         * @return Identity matrix.
         * @note   Only available when R == C (square matrix).
         */
        template<std::size_t R_ = R, std::size_t C_ = C, class = std::enable_if_t<R_ == C_>>
        static constexpr matrix math_identity() {
            matrix result{};
            for (std::size_t i = 0; i < R; ++i) result[i][i] = T{1};
            return result;
        }

        // ----- transpose -----

        /**
         * @brief  Return the transpose of this matrix.
         * @return New matrix<C, R, T> with rows and columns swapped.
         */
        constexpr matrix<C, R, T> math_transposed() const {
            matrix<C, R, T> out{};
            for (std::size_t r = 0; r < R; ++r)
                for (std::size_t c = 0; c < C; ++c)
                    out[c][r] = elements[r][c];
            return out;
        }

        // ----- row / column extraction -----

        /**
         * @brief  Extract a single row as a vector.
         * @param  row  Zero-based row index.
         * @return vector<C, T> containing the requested row.
         */
        constexpr vector<C, T> math_row(std::size_t row) const {
            vector<C, T> out{};
            for (std::size_t c = 0; c < C; ++c) out[c] = elements[row][c];
            return out;
        }

        /**
         * @brief  Extract a single column as a vector.
         * @param  col  Zero-based column index.
         * @return vector<R, T> containing the requested column.
         */
        constexpr vector<R, T> math_col(std::size_t col) const {
            vector<R, T> out{};
            for (std::size_t r = 0; r < R; ++r) out[r] = elements[r][col];
            return out;
        }

        // ----- print -----

        /**
         * @brief  Print the matrix contents to stdout for debugging.
         * @param  label  Optional label printed before the values (may be nullptr).
         *
         * @details Floating-point elements are printed to 4 decimal places
         *          in a 10-character-wide field.  Integer elements are printed
         *          in an 8-character-wide field.
         *          Output is wrapped in "mat4x4 { ... }".
         */
        inline void math_print(const char* label = nullptr) const {
            if (label) std::printf("%s:\n", label);
            std::printf("mat%zux%zu {\n", R, C);
            for (std::size_t r = 0; r < R; ++r) {
                std::printf("  ");
                for (std::size_t c = 0; c < C; ++c) {
                    if constexpr (std::is_floating_point_v<T>)
                        std::printf("%10.4f", static_cast<double>(elements[r][c]));
                    else
                        std::printf("%8lld", static_cast<long long>(elements[r][c]));
                    if (c + 1 < C) std::printf("  ");
                }
                std::printf("\n");
            }
            std::printf("}\n");
        }

        // ----- read / write (raw memcpy) -----

        /**
         * @brief  Copy raw bytes from @p src into this matrix's elements.
         * @param  src  Pointer to at least sizeof(elements) bytes.
         */
        inline void math_readFrom(const void* src) {
            std::memcpy(elements, src, sizeof(elements));
        }

        /**
         * @brief  Copy this matrix's raw bytes into @p dst.
         * @param  dst  Pointer to at least sizeof(elements) writable bytes.
         */
        inline void math_writeTo(void* dst) const {
            std::memcpy(dst, elements, sizeof(elements));
        }

        /**
         * @brief  Construct a matrix by copying raw bytes from @p src.
         * @param  src  Pointer to at least sizeof(matrix) bytes.
         * @return New matrix with contents copied from the source.
         */
        static inline matrix math_read(const void* src) {
            matrix result{};
            std::memcpy(&result, src, sizeof(result));
            return result;
        }
    };

    /**
     * @brief  Left-hand scalar * matrix multiplication.
     * @param  scalar  Scalar value.
     * @param  rhs     Matrix operand.
     * @return New matrix with each element scaled.
     */
    template<std::size_t R, std::size_t C, class T>
    constexpr matrix<R, C, T> operator*(T scalar, const matrix<R, C, T>& rhs) { return rhs * scalar; }

    /**
     * @brief  Matrix-matrix multiplication.
     * @tparam R  Rows of left matrix / result.
     * @tparam K  Shared inner dimension (cols of left == rows of right).
     * @tparam C  Columns of right matrix / result.
     * @tparam T  Element type.
     * @param  a  Left-hand matrix  (R x K).
     * @param  b  Right-hand matrix (K x C).
     * @return Result matrix (R x C).
     */
    template<std::size_t R, std::size_t K, std::size_t C, class T>
    constexpr matrix<R, C, T> operator*(const matrix<R, K, T>& a, const matrix<K, C, T>& b) {
        matrix<R, C, T> out{};
        for (std::size_t r = 0; r < R; ++r)
            for (std::size_t c = 0; c < C; ++c)
                for (std::size_t k = 0; k < K; ++k)
                    out[r][c] += a[r][k] * b[k][c];
        return out;
    }

    /**
     * @brief  Matrix-vector multiplication (vector treated as a column).
     * @tparam R  Rows of the matrix / size of the result vector.
     * @tparam C  Columns of the matrix / size of the input vector.
     * @tparam T  Element type.
     * @param  lhs  Matrix (R x C).
     * @param  rhs  Column vector (C).
     * @return Result vector (R).
     */
    template<std::size_t R, std::size_t C, class T>
    constexpr vector<R, T> operator*(const matrix<R, C, T>& lhs, const vector<C, T>& rhs) {
        vector<R, T> out{};
        for (std::size_t r = 0; r < R; ++r)
            for (std::size_t c = 0; c < C; ++c)
                out[r] += lhs[r][c] * rhs[c];
        return out;
    }

    // ----- matrix layout guarantees -----

    static_assert(sizeof(matrix<4, 4, float>) == 16 * sizeof(float), "mat4x4f layout broken");
    static_assert(sizeof(matrix<3, 3, float>) ==  9 * sizeof(float), "mat3x3f layout broken");

    static_assert(std::is_standard_layout_v<matrix<4, 4, float>>,    "mat4x4f must be standard layout");
    static_assert(std::is_trivially_copyable_v<matrix<4, 4, float>>, "mat4x4f must be trivially copyable");

    // ----- common type aliases -----

    using vec2f   = vector<2, float>;
    using vec3f   = vector<3, float>;
    using vec4f   = vector<4, float>;
    using mat4f   = matrix<4, 4, float>;

    // =========================================================================
    //  W2S / NDC / screen-space utilities
    // =========================================================================

    inline vec4f math_transformToClip(const vec3f& worldPos, const mat4f& viewProj) {
        vec4f clip{};
        for (std::size_t r = 0; r < 4; ++r) {
            clip[r] = viewProj[r][0] * worldPos[0]
                    + viewProj[r][1] * worldPos[1]
                    + viewProj[r][2] * worldPos[2]
                    + viewProj[r][3];
        }
        return clip;
    }

    inline bool math_clipToNdc(const vec4f& clip, vec3f& ndcOut) {
        if (clip.w() <= 0.0f) return false;
        float invW = 1.0f / clip.w();
        ndcOut = vec3f{ clip.x() * invW, clip.y() * invW, clip.z() * invW };
        return true;
    }

    inline vec2f math_ndcToScreen(const vec3f& ndc, float screenWidth, float screenHeight) {
        return vec2f{
            (ndc.x() + 1.0f) * 0.5f * screenWidth,
            (1.0f - ndc.y()) * 0.5f * screenHeight
        };
    }

    inline bool math_worldToScreen(
        const vec3f& worldPos,
        const mat4f& viewProj,
        float screenWidth,
        float screenHeight,
        vec2f& screenOut
    ) {
        vec4f clip = math_transformToClip(worldPos, viewProj);

        vec3f ndc{};
        if (!math_clipToNdc(clip, ndc)) return false;

        screenOut = math_ndcToScreen(ndc, screenWidth, screenHeight);
        return true;
    }

} // namespace zirconium