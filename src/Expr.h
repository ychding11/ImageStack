#ifndef IMAGESTACK_EXPR_H
#define IMAGESTACK_EXPR_H

#include <immintrin.h>
#include <stdint.h>

#include "header.h"

// This file defines a set of image-like function objects. They
// represent pure functions over a 4-d integer domain. Their body is
// completely contained within their type, so that they compile to
// efficient code (with no dynamic dispatch).
//
// E.g., the function f(x, y, t, c) = x*3 + 4 has type Add<Mul<X, Const>, Const>
//
// They are all tagged with a nested type called Expr so that sfinae prevents unwanted constructions

class Image;

struct Region {
    int x, y, t, c, width, height, frames, channels;
};

namespace Expr {

    namespace Scalar {
        // Arithmetic binary operators
        struct Add {
            static float scalar_f(float a, float b) {return a + b;}
            static int scalar_i(int a, int b) {return a + b;}

            // Interval arithmetic versions
            template<typename T>
            static std::pair<T, T> interval(std::pair<T, T> a, std::pair<T, T> b) {
                return std::make_pair(a.first+b.first, a.second+b.second);
            }

        };
        struct Sub {
            static float scalar_f(float a, float b) {return a - b;}
            static int scalar_i(int a, int b) {return a - b;}

            template<typename T>
            static std::pair<T, T> interval(std::pair<T, T> a, std::pair<T, T> b) {
                return std::make_pair(a.first - b.second, a.second - b.first);
            }
        };
        struct Mul {
            static float scalar_f(float a, float b) {return a * b;}
            static int scalar_i(int a, int b) {return a * b;}

            template<typename T>
            static std::pair<T, T> interval(std::pair<T, T> a, std::pair<T, T> b) {
                T v1 = a.first * b.first;
                T v2 = a.first * b.second;
                T v3 = a.second * b.first;
                T v4 = a.second * b.second;
                return std::make_pair(
                    std::min(std::min(v1, v2), std::min(v3, v4)), 
                    std::max(std::max(v1, v2), std::max(v3, v4)));
            }

        };

        struct UnboundedDivisionException {};

        struct Div {
            static float scalar_f(float a, float b) {return a / b;}
            static int scalar_i(int a, int b) {return a / b;}

            template<typename T>
            static std::pair<T, T> interval(std::pair<T, T> a, std::pair<T, T> b) {
                if (b.first <= 0 && b.second >= 0) throw UnboundedDivisionException();
                T v1 = a.first / b.first;
                T v2 = a.first / b.second;
                T v3 = a.second / b.first;
                T v4 = a.second / b.second;
                return std::make_pair(
                    std::min(std::min(v1, v2), std::min(v3, v4)), 
                    std::max(std::max(v1, v2), std::max(v3, v4)));
            }

        };
        struct Min {
            static float scalar_f(float a, float b) {return std::min(a, b);}
            static int scalar_i(int a, int b) {return std::min(a, b);}

            template<typename T>
            static std::pair<T, T> interval(std::pair<T, T> a, std::pair<T, T> b) {
                return std::make_pair(std::min(a.first, b.first),
                                      std::min(a.second, b.second));
            }
        };
        struct Max {
            static float scalar_f(float a, float b) {return std::max(a, b);}
            static int scalar_i(int a, int b) {return std::max(a, b);}

            template<typename T>
            static std::pair<T, T> interval(std::pair<T, T> a, std::pair<T, T> b) {
                return std::make_pair(std::min(a.first, b.first),
                                      std::min(a.second, b.second));
            }
        };

        // Comparisons
        struct GT {
            static bool scalar_f(float a, float b) {return a > b;}
            static bool scalar_i(int a, int b) {return a > b;}
        };
        struct LT {
            static bool scalar_f(float a, float b) {return a < b;}
            static bool scalar_i(int a, int b) {return a < b;}
        };
        struct GE {
            static bool scalar_f(float a, float b) {return a >= b;}
            static bool scalar_i(int a, int b) {return a >= b;}
        };
        struct LE {
            static bool scalar_f(float a, float b) {return a <= b;}
            static bool scalar_i(int a, int b) {return a <= b;}
        };
        struct EQ {
            static bool scalar_f(float a, float b) {return a == b;}
            static bool scalar_i(int a, int b) {return a == b;}
        };
        struct NEQ {
            static bool scalar_f(float a, float b) {return a != b;}
            static bool scalar_i(int a, int b) {return a != b;}
        };        

        // Unary ops
        struct Ceil {
            static float scalar_f(float a) {return ceilf(a);}

            static std::pair<float, float> interval(std::pair<float, float> a) {
                return make_pair(scalar_f(a.first), scalar_f(a.second));
            }
        };

        struct Floor {
            static float scalar_f(float a) {return floorf(a);}

            static std::pair<float, float> interval(std::pair<float, float> a) {
                return make_pair(scalar_f(a.first), scalar_f(a.second));
            }
        };

        struct Sqrt {
            static float scalar_f(float a) {return sqrtf(a);}

            static std::pair<float, float> interval(std::pair<float, float> a) {
                return make_pair(scalar_f(a.first), scalar_f(a.second));
            }
        };
    };

    namespace Vec {
#ifdef __AVX__
        typedef __m256 type;
        const int width = 8;

        inline type broadcast(float v) {
            return _mm256_set1_ps(v);
        }

        inline type set(float a, float b, float c, float d = 0, float e = 0, float f = 0, float g = 0, float h = 0) {
            return _mm256_set_ps(h, g, f, e, d, c, b, a);
        }

        inline type zero() {
            return _mm256_setzero_ps();
        }

        // Arithmetic binary operators
        struct Add : public Scalar::Add {
            static type vec(type a, type b) {return _mm256_add_ps(a, b);}
        };
        struct Sub : public Scalar::Sub {
            static type vec(type a, type b) {return _mm256_sub_ps(a, b);}
        };
        struct Mul : public Scalar::Mul {
            static type vec(type a, type b) {return _mm256_mul_ps(a, b);}
        };
        struct Div : public Scalar::Div {
            static type vec(type a, type b) {return _mm256_div_ps(a, b);}
        };
        struct Min : public Scalar::Min {
            static type vec(type a, type b) {return _mm256_min_ps(a, b);}
        };
        struct Max : public Scalar::Max {
            static type vec(type a, type b) {return _mm256_max_ps(a, b);}
        };

        // Comparisons
        struct GT : public Scalar::GT {
            static type vec(type a, type b) {return _mm256_cmp_ps(a, b, _CMP_GT_OQ);}
        };
        struct LT : public Scalar::LT {
            static type vec(type a, type b) {return _mm256_cmp_ps(a, b, _CMP_LT_OQ);}
        };
        struct GE : public Scalar::GE {
            static type vec(type a, type b) {return _mm256_cmp_ps(a, b, _CMP_GE_OQ);}
        };
        struct LE : public Scalar::LE {
            static type vec(type a, type b) {return _mm256_cmp_ps(a, b, _CMP_LE_OQ);}
        };
        struct EQ : public Scalar::EQ {
            static type vec(type a, type b) {return _mm256_cmp_ps(a, b, _CMP_EQ_OQ);}
        };
        struct NEQ : public Scalar::NEQ {
            static type vec(type a, type b) {return _mm256_cmp_ps(a, b, _CMP_NEQ_OQ);}
        };

        // Logical ops
        inline type blend(type a, type b, type mask) {
            return _mm256_blendv_ps(a, b, mask);
        }

        inline type interleave(type a, type b) {
            // Given vectors a and b, return a[0] b[0] a[1] b[1] a[2] b[2] a[3] b[3]
            __m256 r_lo = _mm256_unpacklo_ps(a, b);
            __m256 r_hi = _mm256_unpackhi_ps(a, b);
            return _mm256_permute2f128_ps(r_lo, r_hi, 2<<4);
        }

        inline type subsample(type a, type b) {
            // Given vectors a and b, return a[0], a[2], a[4], a[6], b[1], b[3], b[5], b[7]
            type bodd = _mm256_shuffle_ps(b, b, (1 << 0) | (1 << 2) | (3 << 4) | (3 << 6));
            // bodd = b[1] b[1] b[3] b[3] b[5] b[5] b[7] b[7]
            type lo = _mm256_permute2f128_ps(a, bodd, (0 << 0) | (2 << 4));
            // lo = a[0] a[1] a[2] a[3] b[1] b[1] b[3] b[3]
            type hi = _mm256_permute2f128_ps(a, bodd, (1 << 0) | (3 << 4));
            // hi = a[4] a[5] a[6] a[7] b[5] b[5] b[7] b[7]
            type result = _mm256_shuffle_ps(lo, hi, (2 << 2) | (2 << 6));
            // result = a[0] a[2] a[4] a[6] b[1] b[3] b[5] b[7]
            return result;

        }
        
        inline type reverse(type a) {
            // reverse each half, the reverse the halves
            type b = _mm256_shuffle_ps(a, a, (3 << 0) | (2 << 2) | (1 << 4) | (0 << 6));
            return _mm256_permute2f128_ps(b, b, 1);
        }


        // Unary ops
        struct Floor : public Scalar::Floor {
            static type vec(type a) {return _mm256_floor_ps(a);}
        };
        struct Ceil : public Scalar::Ceil {
            static type vec(type a) {return _mm256_ceil_ps(a);}
        };
        struct Sqrt : public Scalar::Sqrt {
            static type vec(type a) {return _mm256_sqrt_ps(a);}
        };

        // Loads and stores
        inline type load(const float *f) {
            return _mm256_loadu_ps(f);
        }

        inline void store(type a, float *f) {
            _mm256_storeu_ps(f, a);
        }

#else
#ifdef __SSE__
        typedef __m128 type;
        const int width = 4;

        inline type broadcast(float v) {
            return _mm_set1_ps(v);
        }

        inline type set(float a, float b, float c, float d, float e = 0, float f = 0, float g = 0, float h = 0) {
            return _mm_set_ps(d, c, b, a);
        }

        inline type zero() {
            return _mm_setzero_ps();
        }

        // Arithmetic binary operators
        struct Add : public Scalar::Add {
            static type vec(type a, type b) {return _mm_add_ps(a, b);}
        };
        struct Sub : public Scalar::Sub {
            static type vec(type a, type b) {return _mm_sub_ps(a, b);}
        };
        struct Mul : public Scalar::Mul {
            static type vec(type a, type b) {return _mm_mul_ps(a, b);}
        };
        struct Div : public Scalar::Div {
            static type vec(type a, type b) {return _mm_div_ps(a, b);}
        };
        struct Min : public Scalar::Min {
            static type vec(type a, type b) {return _mm_min_ps(a, b);}
        };
        struct Max : public Scalar::Max {
            static type vec(type a, type b) {return _mm_max_ps(a, b);}
        };

        // Comparisons
        struct GT : public Scalar::GT {
            static type vec(type a, type b) {return _mm_cmpgt_ps(a, b);}
        };
        struct LT : public Scalar::LT {
            static type vec(type a, type b) {return _mm_cmplt_ps(a, b);}
        };
        struct GE : public Scalar::GE {
            static type vec(type a, type b) {return _mm_cmpge_ps(a, b);}
        };
        struct LE : public Scalar::LE {
            static type vec(type a, type b) {return _mm_cmple_ps(a, b);}
        };
        struct EQ : public Scalar::EQ {
            static type vec(type a, type b) {return _mm_cmpeq_ps(a, b);}
        };
        struct NEQ : public Scalar::NEQ {
            static type vec(type a, type b) {return _mm_cmpneq_ps(a, b);}
        };

        inline type interleave(type a, type b) {
            return _mm_unpacklo_ps(a, b);
        }

        inline type subsample(type a, type b) {
            return _mm_shuffle_ps(a, b, (0 << 0) | (2 << 2) | (1 << 4) | (3 << 6));
        }
        
        inline type reverse(type a) {
            return _mm_shuffle_ps(a, a, (3 << 0) | (2 << 2) | (1 << 4) | (0 << 6));
        }

#ifdef __SSE4_1__
        // Logical ops
        inline type blend(type a, type b, type mask) {
            return _mm_blendv_ps(a, b, mask);
        }

        // Unary ops
        struct Floor : public Scalar::Floor {
            static type vec(type a) {return _mm_floor_ps(a);}
        };
        struct Ceil : public Scalar::Ceil {
            static type vec(type a) {return _mm_ceil_ps(a);}
        };
        struct Sqrt : public Scalar::Sqrt {
            static type vec(type a) {return _mm_sqrt_ps(a);}
        };
#else

        inline type blend(type a, type b, type mask) {
            return _mm_or_ps(_mm_and_ps(mask, b),
                             _mm_andnot_ps(mask, a));
        }

        struct Floor : public Scalar::Floor {
            static type vec(type a) {
                union {
                    float f[width];
                    type v;
                } v;
                v.v = a;
                return set(scalar_f(v.f[0]), scalar_f(v.f[1]), scalar_f(v.f[2]), scalar_f(v.f[3]));
            }
        };
        struct Ceil : public Scalar::Ceil {
            static type vec(type a) {
                union {
                    float f[width];
                    type v;
                } v;
                v.v = a;
                return set(scalar_f(v.f[0]), scalar_f(v.f[1]), scalar_f(v.f[2]), scalar_f(v.f[3]));
            }
        };
        struct Sqrt : public Scalar::Sqrt {
            static type vec(type a) {
                union {
                    float f[width];
                    type v;
                } v;
                v.v = a;
                return set(scalar_f(v.f[0]), scalar_f(v.f[1]), scalar_f(v.f[2]), scalar_f(v.f[3]));
            }
        };

#endif

        // Loads and stores
        inline type load(const float *f) {
            return _mm_loadu_ps(f);
        }

        inline void store(type a, float *f) {
            _mm_storeu_ps(f, a);
        }

#else
        // scalar fallback
        typedef float type;
        const int width = 1;

        inline type broadcast(float v) {
            return v;
        }

        inline type zero() {
            return 0;
        }

        // Arithmetic binary operators
        struct Add : public Scalar::Add {
            static type vec(type a, type b) {return scalar_f(a, b);}
        };
        struct Sub : public Scalar::Sub {
            static type vec(type a, type b) {return scalar_f(a, b);}
        };
        struct Mul : public Scalar::Mul {
            static type vec(type a, type b) {return scalar_f(a, b);}
        };
        struct Div : public Scalar::Div {
            static type vec(type a, type b) {return scalar_f(a, b);}
        };
        struct Min : public Scalar::Min {
            static type vec(type a, type b) {return scalar_f(a, b);}
        };
        struct Max : public Scalar::Max {
            static type vec(type a, type b) {return scalar_f(a, b);}
        };

        // Comparisons
        struct GT : public Scalar::GT {
            static bool vec(type a, type b) {return scalar_f(a, b);}
        };
        struct LT : public Scalar::Lt {
            static bool vec(type a, type b) {return scalar_f(a, b);}
        };
        struct GE : public Scalar::GE {
            static bool vec(type a, type b) {return scalar_f(a, b);}
        };
        struct LE : public Scalar::LE {
            static bool vec(type a, type b) {return scalar_f(a, b);}
        };
        struct EQ : public Scalar::EQ {
            static bool vec(type a, type b) {return scalar_f(a, b);}
        };
        struct NEQ : public Scalar::NEQ {
            static bool vec(type a, type b) {return scalar_f(a, b);}
        };

        // Logical ops
        inline type blend(bool mask, type a, type b) {
            return (mask ? b : a);
        }

        inline type interleave(type a, type b) {
            return a;
        }

        inline type subsample(type a, type b) {
            return a;
        }

        inline type reverse(type a) {
            return a;
        }

        // Unary ops
        struct Floor : public Scalar::Floor {
            static type vec(type a) {return scalar_f(a);}
        };
        struct Ceil : public Scalar::Ceil {
            static type vec(type a) {return scalar_f(a);}
        };
        struct Sqrt : public Scalar::Sqrt {
            static type vec(type a) {return scalar_f(a);}
        };

        // Loads and stores
        inline type load(const float *f) {
            return *f;
        }

        inline void store(type a, float *f) {
            *f = a;
        }


#endif
#endif
    }

    // A base class for things which do not depend on image data
    struct Unbounded {
        int getSize(int) const {return 0;}
        bool boundedVecX() const {return false;}
        int minVecX() const {return 0xa0000000;}
        int maxVecX() const {return 0x3fffffff;}
    };


    // Constants
    struct ConstFloat : public Unbounded {
        typedef ConstFloat FloatExpr;
        const float val;
        ConstFloat(const float val_) : val(val_) {}

        // State needed to iterate across a scanline
        struct Iter {
            const float val;
            const Vec::type vec_val;
            Iter() : val(0), vec_val(Vec::zero()) {}
            Iter(float v) : val(v), vec_val(Vec::broadcast(val)) {
            }
            float operator[](int x) const {return val;}
            Vec::type vec(int x) const {return vec_val;}
        };
        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter(val);
        }        
        void prepare(int phase, Region r) const {}

        std::pair<float, float> bounds(Region) const {
            return std::make_pair(val, val);
        }
    };    

    struct ConstInt {
        typedef ConstInt IntExpr;
        const int val;
        ConstInt(const int val_) : val(val_) {}

        int getSize(int) const {return 0;}

        struct Iter {
            const int val;
            Iter() : val(0) {}
            Iter(int v) : val(v) {}
            int operator[](int x) const {return val;}
        };
        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter(val);
        }
        void prepare(int phase, Region r) const {
        }

        std::pair<int, int> bounds(Region) const {
            return std::make_pair(val, val);
        }
    };

    // Casts from int <-> float
    template<typename A>
    struct IntToFloat {
        typedef IntToFloat<typename A::IntExpr> FloatExpr;

        IntToFloat(const A &a_) : a(a_) {}

        const A a;

        int getSize(int i) const {return a.getSize(i);}
        bool boundedVecX() const {return false;}
        int minVecX() const {return 0xa0000000;}
        int maxVecX() const {return 0x3fffffff;}

        struct Iter {
            const typename A::Iter a;
            Iter() {}
            Iter(const typename A::Iter &a_) : a(a_) {}
            float operator[](int x) const {return (float)a[x];}
            Vec::type vec(int x) const {
                if (Vec::width == 8) {
                    return Vec::set(a[x], a[x+1], a[x+2], a[x+3], a[x+4], a[x+5], a[x+6], a[x+7]);
                } else if (Vec::width == 4) {
                    return Vec::set(a[x], a[x+1], a[x+2], a[x+3]);
                } else {
                    union {
                        float f[Vec::width];
                        Vec::type v;
                    } v;
                    for (int i = 0; i < Vec::width; i++) {
                        v.f[i] = a[x+i];
                    }
                    return v.v;
                }                
            }
        };
        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter(a.scanline(x, y, t, c, width));
        }
        void prepare(int phase, Region r) const {
            a.prepare(phase, r);
        }        

        std::pair<float, float> bounds(Region r) const {
            std::pair<int, int> a_bounds = a.bounds(r);
            return std::make_pair((float)a_bounds.first, (float)a_bounds.second);
        }
    };

    template<typename A>
    struct FloatToInt {
        typedef FloatToInt<typename A::FloatExpr> IntExpr;

        const A a;

        FloatToInt(const A &a_) : a(a_) {}

        int getSize(int i) const {return a.getSize(i);}

        struct Iter {
            const typename A::Iter a;
            Iter() {}
            Iter(const typename A::Iter &a_) : a(a_) {}
            int operator[](int x) const {return (int)a[x];}
        };
        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter(a.scanline(x, y, t, c, width));
        }
        void prepare(int phase, Region r) const {
            a.prepare(phase, r);
        }        

        std::pair<int, int> bounds(Region r) const {
            std::pair<float, float> a_bounds = a.bounds(r);
            return std::make_pair((int)a_bounds.first, (int)a_bounds.second);
        }

    };

    template<typename A>
    IntToFloat<typename A::IntExpr> toFloat(const A &a) {
        return IntToFloat<A>(a);
    }

    template<typename A>
    FloatToInt<typename A::FloatExpr> toInt(const A &a) {
        return FloatToInt<A>(a);
    }

    // Coordinates
    struct X {
        typedef X IntExpr;

        int getSize(int) const {return 0;}

        // State needed to iterate across a scanline
        struct Iter {
            float operator[](int x) const {return x;}
        };

        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter();
        }        

        void prepare(int phase, Region r) const { 
        }

        std::pair<int, int> bounds(Region r) const {
            return std::make_pair(r.x, r.x + r.width - 1);
        }
    };

    struct Y {
        typedef Y IntExpr;

        int getSize(int) const {return 0;}

        typedef ConstInt::Iter Iter;

        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter(y);
        }        

        void prepare(int phase, Region r) const { 
        }

        std::pair<int, int> bounds(Region r) const {
            return std::make_pair(r.y, r.y + r.height - 1);
        }

    };

    struct T {
        typedef T IntExpr;

        int getSize(int) const {return 0;}

        typedef ConstInt::Iter Iter;

        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter(t);
        }
        
        void prepare(int phase, Region r) const { 
        }

        std::pair<int, int> bounds(Region r) const {
            return std::make_pair(r.t, r.t + r.frames - 1);
        }

    };

    struct C {
        typedef C IntExpr;

        int getSize(int) const {return 0;}

        typedef ConstInt::Iter Iter;

        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter(c);
        }
        
        void prepare(int phase, Region r) const { 
        }

        std::pair<int, int> bounds(Region r) const {
            return std::make_pair(r.c, r.c + r.channels - 1);
        }

    };

    // Arithmetic binary operators
    template<typename A, typename B, typename Op>
    struct FBinaryOp {
        typedef FBinaryOp<typename A::FloatExpr, typename B::FloatExpr, Op> FloatExpr;
        const A a;
        const B b;
        
        FBinaryOp(const A &a_, const B &b_) : a(a_), b(b_) {
            for (int i = 0; i < 4; i++) {
                if (a.getSize(i) && b.getSize(i)) {
                    assert(a.getSize(i) == b.getSize(i),
                           "Can only combine images with matching size\n");
                }
            }
        }
        
        int getSize(int i) const {
            if (a.getSize(i)) return a.getSize(i);
            return b.getSize(i);
        }
        
        struct Iter {
            const typename A::Iter a;
            const typename B::Iter b;
            Iter() {}
            Iter(const typename A::Iter &a_, const typename B::Iter &b_) : a(a_), b(b_) {}
            float operator[](int x) const {
                return Op::scalar_f(a[x], b[x]);
            }
            Vec::type vec(int x) const {
                return Op::vec(a.vec(x), b.vec(x));
            }
        };
        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter(a.scanline(x, y, t, c, width), b.scanline(x, y, t, c, width));
        }

        bool boundedVecX() const {return a.boundedVecX() || b.boundedVecX();}
        int minVecX() const {return std::max(a.minVecX(), b.minVecX());}
        int maxVecX() const {return std::min(a.maxVecX(), b.maxVecX());}
        
        void prepare(int phase, Region r) const { 
            a.prepare(phase, r);
            b.prepare(phase, r);
        }

        std::pair<float, float> bounds(Region r) const {
            return Op::interval(a.bounds(r), b.bounds(r));
        }

    };

    template<typename A, typename B, typename Op>
    struct IBinaryOp {
        typedef IBinaryOp<typename A::IntExpr, typename B::IntExpr, Op> IntExpr;
        const A a;
        const B b;
        
        IBinaryOp(const A &a_, const B &b_) : a(a_), b(b_) {
            for (int i = 0; i < 4; i++) {
                if (a.getSize(i) && b.getSize(i)) {
                    assert(a.getSize(i) == b.getSize(i),
                           "Can only combine images with matching size\n");
                }
            }
        }
        
        int getSize(int i) const {
            if (a.getSize(i)) return a.getSize(i);
            return b.getSize(i);
        }
        
        struct Iter {
            const typename A::Iter a;
            const typename B::Iter b;
            Iter() {}
            Iter(const typename A::Iter &a_, const typename B::Iter &b_) : a(a_), b(b_) {}
            float operator[](int x) const {
                return Op::scalar_i(a[x], b[x]);
            }
        };
        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter(a.scanline(x, y, t, c, width), b.scanline(x, y, t, c, width));
        }
        void prepare(int phase, Region r) const { 
            a.prepare(phase, r);
            b.prepare(phase, r);
        }

        std::pair<int, int> bounds(Region r) const {
            return Op::interval(a.bounds(r), b.bounds(r));
        }

    };
    
    // Comparison binary operators
    template<typename A, typename B, typename Op>
    struct FCmp {
        typedef FCmp<typename A::FloatExpr, typename B::FloatExpr, Op> BoolExpr;
        const A a;
        const B b;
        
        FCmp(const A &a_, const B &b_) : a(a_), b(b_) {
            for (int i = 0; i < 4; i++) {
                if (a.getSize(i) && b.getSize(i)) {
                    assert(a.getSize(i) == b.getSize(i),
                           "Can only combine images with matching size\n");
                }
            }
        }
        int getSize(int i) const {
            if (a.getSize(i)) return a.getSize(i);
            return b.getSize(i);
        }
        
        struct Iter {
            const typename A::Iter a;
            const typename B::Iter b;
            Iter() {}
            Iter(const typename A::Iter &a_, const typename B::Iter &b_) : a(a_), b(b_) {}
            bool operator[](int x) const {
                return Op::scalar_f(a[x], b[x]);
            }
            Vec::type vec(int x) const {
                return Op::vec(a.vec(x), b.vec(x));
            }
        };
        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter(a.scanline(x, y, t, c, width),
                        b.scanline(x, y, t, c, width));
        }
        bool boundedVecX() const {return a.boundedVecX() || b.boundedVecX();}
        int minVecX() const {return std::max(a.minVecX(), b.minVecX());}
        int maxVecX() const {return std::min(a.maxVecX(), b.maxVecX());}
        
        void prepare(int phase, Region r) const { 
            a.prepare(phase, r);
            b.prepare(phase, r);
        }
    };    

    // Comparison binary operators
    template<typename A, typename B, typename Op>
    struct ICmp {
        typedef ICmp<typename A::IntExpr, typename B::IntExpr, Op> BoolExpr;
        const A a;
        const B b;
        
        ICmp(const A &a_, const B &b_) : a(a_), b(b_) {
            for (int i = 0; i < 4; i++) {
                if (a.getSize(i) && b.getSize(i)) {
                    assert(a.getSize(i) == b.getSize(i),
                           "Can only combine images with matching size\n");
                }
            }
        }
        int getSize(int i) const {
            if (a.getSize(i)) return a.getSize(i);
            return b.getSize(i);
        }
        
        struct Iter {
            const typename A::Iter a;
            const typename B::Iter b;
            Iter() {}
            Iter(const typename A::Iter &a_, const typename B::Iter &b_) : a(a_), b(b_) {}
            bool operator[](int x) const {
                return Op::scalar_i(a[x], b[x]);
            }
            Vec::type vec(int x) const {
                if (Vec::width == 8) {
                    return Op::vec(Vec::set(a[x], a[x+1], a[x+2], a[x+3], 
                                            a[x+4], a[x+5], a[x+6], a[x+7]),
                                   Vec::set(b[x], b[x+1], b[x+2], b[x+3], 
                                            b[x+4], b[x+5], b[x+6], b[x+7]));
                } else if (Vec::width == 4) {
                    return Op::vec(Vec::set(a[x], a[x+1], a[x+2], a[x+3]),
                                   Vec::set(b[x], b[x+1], b[x+2], b[x+3]));
                } else {
                    union {
                        float f[Vec::width];
                        Vec::type v;
                    } va, vb;
                    for (int i = 0; i < Vec::width; i++) {
                        va.f[i] = a[x+i];
                        vb.f[i] = b[x+i];
                    }
                    return Op::vec(va.v, vb.v);
                }
            }
        };
        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter(a.scanline(x, y, t, c, width),
                        b.scanline(x, y, t, c, width));
        }
        bool boundedVecX() const {return false;}
        int minVecX() const {return 0xa0000000;}
        int maxVecX() const {return 0x3fffffff;}
        
        void prepare(int phase, Region r) const { 
            a.prepare(phase, r);
            b.prepare(phase, r);
        }
    };  

    template<typename A, typename B>
    FBinaryOp<typename A::FloatExpr, typename B::FloatExpr, Vec::Min>
    min(const A &a, const B &b) {
        return FBinaryOp<typename A::FloatExpr, typename B::FloatExpr, Vec::Min>(a, b);
    }
    template<typename A>
    FBinaryOp<typename A::FloatExpr, ConstFloat, Vec::Min>
    min(const A &a, float b) {
        return min(a, ConstFloat(b));
    }
    template<typename B>
    FBinaryOp<ConstFloat, typename B::FloatExpr, Vec::Min>
    min(float a, const B &b) {
        return min(ConstFloat(a), b);
    }
    template<typename A, typename B>
    FBinaryOp<typename A::FloatExpr, typename B::FloatExpr, Vec::Max>
    max(const A &a, const B &b) {
        return FBinaryOp<typename A::FloatExpr, typename B::FloatExpr, Vec::Max>(a, b);
    }
    template<typename A>
    FBinaryOp<typename A::FloatExpr, ConstFloat, Vec::Max>
    max(const A &a, float b) {
        return max(a, ConstFloat(b));
    }
    template<typename B>
    FBinaryOp<ConstFloat, typename B::FloatExpr, Vec::Max>
    max(float a, const B &b) {
        return max(ConstFloat(a), b);
    }

    template<typename A, typename B, typename C>
    FBinaryOp<FBinaryOp<typename A::FloatExpr, typename B::FloatExpr, Vec::Max>, typename C::FloatExpr, Vec::Min>
    clamp(const A &a, const B &b, const C &c) {
        return min(max(a, b), c);
    }
    template<typename B, typename C>
    FBinaryOp<FBinaryOp<ConstFloat, typename B::FloatExpr, Vec::Max>, typename C::FloatExpr, Vec::Min>
    clamp(float a, const B &b, const C &c) {
        return min(max(a, b), c);
    }
    template<typename A, typename C>
    FBinaryOp<FBinaryOp<typename A::FloatExpr, ConstFloat, Vec::Max>, typename C::FloatExpr, Vec::Min>
    clamp(const A &a, float b, const C &c) {
        return min(max(a, b), c);
    }
    template<typename A, typename B>
    FBinaryOp<FBinaryOp<typename A::FloatExpr, typename B::FloatExpr, Vec::Max>, ConstFloat, Vec::Min>
    clamp(const A &a, const B &b, float c) {
        return min(max(a, b), c);
    }
    template<typename A>
    FBinaryOp<FBinaryOp<typename A::FloatExpr, ConstFloat, Vec::Max>, ConstFloat, Vec::Min>
    clamp(const A &a, float b, float c) {
        return min(max(a, b), c);
    }

    // Lift a unary function over floats to the same function over an image (e.g. cosf)
    template<float(*fn)(float), typename A>
    struct Lift {
        typedef Lift<fn, typename A::FloatExpr> FloatExpr;
        const A a;
        Lift(const A &a_) : a(a_) {}

        int getSize(int i) const {return a.getSize(i);}

        struct Iter {
            const typename A::Iter a;
            Iter() {}
            Iter(const typename A::Iter &a_) : a(a_) {}
            float operator[](int x) const {return (*fn)(a[x]);}
            Vec::type vec(int x) const {
                union {
                    float f[Vec::width];
                    Vec::type v;
                } va;
                va.v = a.vec(x);
                for (int i = 0; i < Vec::width; i++) {
                    va.f[i] = (*fn)(va.f[i]);
                }
                return va.v;
            }
        };
        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter(a.scanline(x, y, t, c, width));
        }
        bool boundedVecX() const {return a.boundedVecX();}
        int minVecX() const {return a.minVecX();}
        int maxVecX() const {return a.maxVecX();}
        
        void prepare(int phase, Region r) const { 
            a.prepare(phase, r);
        }

        std::pair<float, float> bounds(Region r) const;
        
    };

    // Lift a vector function to the same function over an image (e.g. floor)
    template<typename A, typename Op>
    struct UnaryOp {
        typedef UnaryOp<typename A::FloatExpr, Op> FloatExpr;
        const A a;
        UnaryOp(const A &a_) : a(a_) {}

        int getSize(int i) const {return a.getSize(i);}

        struct Iter {
            const typename A::Iter a;
            Iter() {}
            Iter(const typename A::Iter &a_) : a(a_) {}
            float operator[](int x) const {return Op::scalar_f(a[x]);}
            Vec::type vec(int x) const {
                return Op::vec(a.vec(x));
            }
        };
        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter(a.scanline(x, y, t, c, width));
        }
        bool boundedVecX() const {return a.boundedVecX();}
        int minVecX() const {return a.minVecX();}
        int maxVecX() const {return a.maxVecX();}
        
        void prepare(int phase, Region r) const { 
            a.prepare(phase, r);
        }

        std::pair<float, float> bounds(Region r) const {
            return Op::interval(a.bounds(r));
        }
    };

    // Arithmetic binary operators
    template<float(*fn)(float, float), typename A, typename B>
    struct Lift2 {
        typedef Lift2<fn, typename A::FloatExpr, typename B::FloatExpr> FloatExpr;
        const A a;
        const B b;

        Lift2(const A &a_, const B &b_) : a(a_), b(b_) {
            for (int i = 0; i < 4; i++) {
                if (a.getSize(i) && b.getSize(i)) {
                    assert(a.getSize(i) == b.getSize(i),
                           "Can only combine images with matching size\n");
                }
            }
        }

        int getSize(int i) const {
            if (a.getSize(i)) return a.getSize(i);
            return b.getSize(i);
        }

        struct Iter {
            const typename A::Iter a;
            const typename B::Iter b;
            Iter() {}
            Iter(const typename A::Iter &a_, const typename B::Iter &b_) : a(a_), b(b_) {}
            float operator[](int x) const {
                return (*fn)(a[x], b[x]);
            }
            Vec::type vec(int x) const {
                union {
                    float f[Vec::width];
                    Vec::type v;
                } va, vb;
                va.v = a.vec(x);
                vb.v = b.vec(x);
                for (int i = 0; i < Vec::width; i++) {
                    vb.f[i] = (*fn)(va.f[i], vb.f[i]);
                }
                return vb.v;
            }
        };
        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter(a.scanline(x, y, t, c, width), b.scanline(x, y, t, c, width));
        }
        bool boundedVecX() const {return a.boundedVecX() || b.boundedVecX();}
        int minVecX() const {return std::max(a.minVecX(), b.minVecX());}
        int maxVecX() const {return std::min(a.maxVecX(), b.maxVecX());}
        
        void prepare(int phase, Region r) const { 
            a.prepare(phase, r);
            b.prepare(phase, r);
        }

        std::pair<float, float> bounds(Region r) const;
    };

    template<typename A>
    Lift<logf, typename A::FloatExpr> log(const A &a) {
        return Lift<logf, A>(a);
    }

    template<typename A>
    Lift<expf, typename A::FloatExpr> exp(const A &a) {
        return Lift<expf, A>(a);
    }

    template<typename A>
    Lift<cosf, typename A::FloatExpr> cos(const A &a) {
        return Lift<cosf, A>(a);
    }

    template<typename A>
    Lift<sinf, typename A::FloatExpr> sin(const A &a) {
        return Lift<sinf, A>(a);
    }

    template<typename A>
    Lift<tanf, typename A::FloatExpr> tan(const A &a) {
        return Lift<tanf, A>(a);
    }

    template<typename A>
    Lift<acosf, typename A::FloatExpr> acos(const A &a) {
        return Lift<acosf, A>(a);
    }

    template<typename A>
    Lift<asinf, typename A::FloatExpr> asin(const A &a) {
        return Lift<asinf, A>(a);
    }

    template<typename A>
    Lift<atanf, typename A::FloatExpr> atan(const A &a) {
        return Lift<atanf, A>(a);
    }

    template<typename A>
    Lift<fabsf, typename A::FloatExpr> abs(const A &a) {
        return Lift<fabsf, A>(a);
    }

    template<typename A>
    UnaryOp<typename A::FloatExpr, Vec::Sqrt> sqrt(const A &a) {
        return UnaryOp<A, Vec::Sqrt>(a);
    }

    template<typename A>
    UnaryOp<typename A::FloatExpr, Vec::Floor> floor(const A &a) {
        return UnaryOp<A, Vec::Floor>(a);
    }

    template<typename A>
    UnaryOp<typename A::FloatExpr, Vec::Ceil> ceil(const A &a) {
        return UnaryOp<A, Vec::Ceil>(a);
    }

    template<typename A, typename B>
    Lift2<powf, typename A::FloatExpr, typename B::FloatExpr> pow(const A &a, const B &b) {
        return Lift2<powf, A, B>(a, b);
    }
    template<typename A>
    Lift2<powf, typename A::FloatExpr, ConstFloat> pow(const A &a, float b) {
        return Lift2<powf, A, ConstFloat>(a, b);
    }
    template<typename B>
    Lift2<powf, ConstFloat, typename B::FloatExpr> pow(float a, const B &b) {
        return Lift2<powf, ConstFloat, B>(a, b);
    }

    template<typename A, typename B>
    Lift2<fmodf, typename A::FloatExpr, typename B::FloatExpr> fmod(const A &a, const B &b) {
        return Lift2<fmodf, A, B>(a, b);
    }
    template<typename A>
    Lift2<fmodf, typename A::FloatExpr, ConstFloat> fmod(const A &a, float b) {
        return Lift2<fmodf, A, ConstFloat>(a, b);
    }
    template<typename B>
    Lift2<fmodf, ConstFloat, typename B::FloatExpr> fmod(float a, const B &b) {
        return Lift2<fmodf, ConstFloat, B>(a, b);
    }

    template<typename A, typename B>
    Lift2<atan2f, typename A::FloatExpr, typename B::FloatExpr> atan2(const A &a, const B &b) {
        return Lift2<atan2f, A, B>(a, b);
    }

    template<typename A>
    Lift2<atan2f, typename A::FloatExpr, ConstFloat> atan2(const A &a, float b) {
        return Lift2<atan2f, A, ConstFloat>(a, b);
    }

    template<typename B>
    Lift2<atan2f, ConstFloat, typename B::FloatExpr> atan2(float a, const B &b) {
        return Lift2<atan2f, ConstFloat, B>(a, b);
    }


    template<typename A, typename B, typename C>
    struct _Select {
        typedef _Select<typename A::BoolExpr, typename B::FloatExpr, typename C::FloatExpr> FloatExpr;
        const A a;
        const B b;
        const C c;

        _Select(const A &a_, const B &b_, const C &c_) : a(a_), b(b_), c(c_) {
            for (int i = 0; i < 4; i++) {
                int s = a.getSize(i);
                if (!s) s = b.getSize(i);
                if (!s) s = c.getSize(i);
                assert((a.getSize(i) == s || a.getSize(i) == 0) &&
                       (b.getSize(i) == s || b.getSize(i) == 0) &&
                       (c.getSize(i) == s || c.getSize(i) == 0),
                       "Can only combine images with matching size\n");
            }
        }

        int getSize(int i) const {
            if (a.getSize(i)) return a.getSize(i);
            if (b.getSize(i)) return b.getSize(i);
            if (c.getSize(i)) return c.getSize(i);
            return 0;
        }

        struct Iter {
            const typename A::Iter a;
            const typename B::Iter b;
            const typename C::Iter c;
            Iter() {}
            Iter(const typename A::Iter &a_,
                 const typename B::Iter &b_,
                 const typename C::Iter &c_) : a(a_), b(b_), c(c_) {}
            float operator[](int x) const {
                return a[x] ? b[x] : c[x];
            }
            Vec::type vec(int x) const {
                const Vec::type va = a.vec(x);
                const Vec::type vb = b.vec(x);
                const Vec::type vc = c.vec(x);
                return Vec::blend(vc, vb, va);
            }
        };
        Iter scanline(int x, int y, int t, int c_, int width) const {
            return Iter(a.scanline(x, y, t, c_, width),
                        b.scanline(x, y, t, c_, width),
                        c.scanline(x, y, t, c_, width));
        }
        bool boundedVecX() const {return a.boundedVecX() || b.boundedVecX() || c.boundedVecX();}
        int minVecX() const {return std::max(std::max(a.minVecX(), b.minVecX()), c.minVecX());}
        int maxVecX() const {return std::min(std::min(a.maxVecX(), b.maxVecX()), c.maxVecX());}
        
        void prepare(int phase, Region r) const {
            a.prepare(phase, r);
            b.prepare(phase, r);
            c.prepare(phase, r);
        }


        std::pair<float, float> bounds(Region r) {
            std::pair<float, float> a_bounds = a.bounds(r);
            std::pair<float, float> b_bounds = b.bounds(r);
            return std::make_pair(std::min(a_bounds.first, b_bounds.first),
                                  std::max(a_bounds.second, b_bounds.second));
        }

    };

    template<typename A, typename B, typename C>
    _Select<typename A::BoolExpr, typename B::FloatExpr, typename C::FloatExpr>
    Select(const A &a, const B &b, const C &c) {
        return _Select<typename A::BoolExpr, typename B::FloatExpr, typename C::FloatExpr>(a, b, c);
    }

    template<typename A, typename C>
    _Select<typename A::BoolExpr, ConstFloat, typename C::FloatExpr>
    Select(const A &a, float b, const C &c) {
        return _Select<typename A::BoolExpr, ConstFloat, typename C::FloatExpr>(a, ConstFloat(b), c);
    }

    template<typename A, typename B>
    _Select<typename A::BoolExpr, typename B::FloatExpr, ConstFloat>
    Select(const A &a, const B &b, float c) {
        return _Select<typename A::BoolExpr, typename B::FloatExpr, ConstFloat>(a, b, ConstFloat(c));
    }

    template<typename A>
    _Select<typename A::BoolExpr, ConstFloat, ConstFloat>
    Select(const A &a, float b, float c) {
        return _Select<typename A::BoolExpr, ConstFloat, ConstFloat>(a, ConstFloat(b), ConstFloat(c));
    }

    template<typename A, typename B, typename C>
    struct _IfThenElse {
        typedef _IfThenElse<typename A::BoolExpr, typename B::FloatExpr, typename C::FloatExpr> FloatExpr;
        const A a;
        const B b;
        const C c;

        _IfThenElse(const A &a_, const B &b_, const C &c_) : a(a_), b(b_), c(c_) {
            for (int i = 0; i < 4; i++) {
                int s = a.getSize(i);
                if (!s) s = b.getSize(i);
                if (!s) s = c.getSize(i);
                assert((a.getSize(i) == s || a.getSize(i) == 0) &&
                       (b.getSize(i) == s || b.getSize(i) == 0) &&
                       (c.getSize(i) == s || c.getSize(i) == 0),
                       "Can only combine images with matching size\n");
            }
        }

        int getSize(int i) const {
            if (a.getSize(i)) return a.getSize(i);
            if (b.getSize(i)) return b.getSize(i);
            if (c.getSize(i)) return c.getSize(i);
            return 0;
        }

        struct Iter {
            const typename A::Iter a;
            const typename B::Iter b;
            const typename C::Iter c;
            Iter() {}
            Iter(const typename A::Iter &a_,
                 const typename B::Iter &b_,
                 const typename C::Iter &c_) : a(a_), b(b_), c(c_) {}
            float operator[](int x) const {
                return a[x] ? b[x] : c[x];
            }
            Vec::type vec(int x) const {
                union {
                    float f[Vec::width];
                    Vec::type v;
                } vres;
                for (int i = 0; i < Vec::width; i++) {
                    if (a[x+i]) vres.f[i] = b[x+i];
                    else vres.f[i] = c[x+i];
                }
                return vres.v;
            }
        };
        Iter scanline(int x, int y, int t, int c_, int width) const {
            return Iter(a.scanline(x, y, t, c_, width),
                        b.scanline(x, y, t, c_, width),
                        c.scanline(x, y, t, c_, width));
        }
        bool boundedVecX() const {return a.boundedVecX() || b.boundedVecX() || c.boundedVecX();}
        int minVecX() const {return std::max(std::max(a.minVecX(), b.minVecX()), c.minVecX());}
        int maxVecX() const {return std::min(std::min(a.maxVecX(), b.maxVecX()), c.maxVecX());}
        
        void prepare(int phase, Region r) const {
            a.prepare(phase, r);
            b.prepare(phase, r);
            c.prepare(phase, r);
        }

        std::pair<float, float> bounds(Region r) {
            std::pair<float, float> a_bounds = a.bounds(r);
            std::pair<float, float> b_bounds = b.bounds(r);
            return std::make_pair(std::min(a_bounds.first, b_bounds.first),
                                  std::max(a_bounds.second, b_bounds.second));
        }

    };

    template<typename A, typename B, typename C>
    _IfThenElse<typename A::BoolExpr, typename B::FloatExpr, typename C::FloatExpr>
    IfThenElse(const A &a, const B &b, const C &c) {
        return _IfThenElse<typename A::BoolExpr, typename B::FloatExpr, typename C::FloatExpr>(a, b, c);
    }

    template<typename A, typename C>
    _IfThenElse<typename A::BoolExpr, ConstFloat, typename C::FloatExpr>
    IfThenElse(const A &a, const float b, const C &c) {
        return _IfThenElse<typename A::BoolExpr, ConstFloat, typename C::FloatExpr>(a, ConstFloat(b), c);
    }

    template<typename A, typename B>
    _IfThenElse<typename A::BoolExpr, typename B::FloatExpr, ConstFloat>
    IfThenElse(const A &a, const B &b, const float c) {
        return _IfThenElse<typename A::BoolExpr, typename B::FloatExpr, ConstFloat>(a, b, ConstFloat(c));
    }

    template<typename A>
    _IfThenElse<typename A::BoolExpr, ConstFloat, ConstFloat>
    IfThenElse(const A &a, const float b, const float c) {
        return _IfThenElse<typename A::BoolExpr, ConstFloat, ConstFloat>(a, ConstFloat(b), ConstFloat(c));
    }

    // Translation (TODO: shift by static amount using int template params?)
    template<typename A>
    struct _Shift {
        typedef _Shift<typename A::FloatExpr> FloatExpr;
        const A a;
        const int xo, yo, to, co;

        _Shift(const A &a_, int xo_, int yo_, int to_ = 0, int co_ = 0) : 
            a(a_), xo(xo_), yo(yo_), to(to_), co(co_) {
            assert((xo == 0 || a.getSize(0) == 0) &&
                   (yo == 0 || a.getSize(1) == 0) &&
                   (to == 0 || a.getSize(2) == 0) &&
                   (co == 0 || a.getSize(3) == 0),
                   "Can't shift expressions in bounded dimensions");
        }
        
        int getSize(int i) const {
            return a.getSize(i);
        }
        
        struct Iter {
            const typename A::Iter a;
            const int xo;
            Iter() : xo(0) {}
            Iter(const typename A::Iter &a_, int xo_) : a(a_), xo(xo_) {}
            float operator[](int x) const {
                return a[x-xo];
            }
            Vec::type vec(int x) const {
                return a.vec(x-xo);
            }
        };
        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter(a.scanline(x-xo, y-yo, t-to, c-co, width), xo);
        }
        bool boundedVecX() const {return a.boundedVecX();}
        int minVecX() const {return a.minVecX() + xo;}
        int maxVecX() const {return a.maxVecX() + xo;}
        
        void prepare(int phase, Region r) const { 
            r.x -= xo;
            r.y -= yo;
            r.t -= to;
            r.c -= co;
            a.prepare(phase, r);
        }
        
        std::pair<float, float> bounds(Region r) const {
            r.x -= xo;
            r.y -= yo;
            r.t -= to;
            r.c -= co;            
            return a.bounds(r);
        }
    };
    
    template<typename A>
    _Shift<typename A::FloatExpr>
    shiftX(const A &a, int xo) {
        return _Shift<A>(a, xo, 0, 0, 0);
    }

    template<typename A>
    _Shift<typename A::FloatExpr>
    shiftY(const A &a, int yo) {
        return _Shift<A>(a, 0, yo, 0, 0);
    }

    template<typename A>
    _Shift<typename A::FloatExpr>
    shift(const A &a, int xo, int yo, int to = 0, int co = 0) {
        return _Shift<A>(a, xo, yo, to, co);
    }

    // Add a boundary condition
    template<typename A>
    struct _ZeroBoundary {
        typedef _ZeroBoundary<typename A::FloatExpr> FloatExpr;
        const A a;
        _ZeroBoundary(const A &a_) : a(a_) {}

        // Once you add a boundary condition, things are unbounded
        int getSize(int) const {return 0;}

        struct Iter {
            const typename A::Iter a;
            const bool outOfBounds;
            const int width;
            Iter() : outOfBounds(true), width(0) {}
            Iter(const typename A::Iter &a_, int w) : 
                a(a_), outOfBounds(false), width(w) {  
            }

            float operator[](int x) const {
                if (outOfBounds || x < 0 || x >= width) return 0;
                return a[x];
            }
            Vec::type vec(int x) const {
                // This only gets called if we're in-bounds in the x dimension
                if (outOfBounds) return Vec::broadcast(0);
                else {
                    #ifdef BOUNDS_CHECKING
                    assert (x >= 0 && x <= width - Vec::width,
                            "ZeroBoundary vec(%d) called:\n"
                            "This is not sufficiently within 0 - %d\n",
                            x, width);
                    #endif
                    return a.vec(x);
                }
                /*
                // Completely out-of-bounds
                if (outOfBounds || (x < 1-Vec::width) || (x >= width)) {
                    return Vec::broadcast(0);
                } else if (x >= 0 && x + Vec::width <= width) {
                    // Completely in-bounds
                    return a.vec(x);
                } else {
                    // Overlapping the start and/or end                    
                    union {
                        float f[Vec::width];
                        Vec::type v;
                    } v;
                    for (int i = 0; i < Vec::width; i++) {
                        v.f[i] = (x+i < 0 || x+i >= width) ? 0 : a[x+i];
                    }
                    return v.v;
                }
                */
            }
        };
        Iter scanline(int x, int y, int t, int c, int width) const {
            bool oob = false;
            if (a.getSize(1)) oob = (oob || y < 0 || y >= a.getSize(1));
            if (a.getSize(2)) oob = (oob || t < 0 || t >= a.getSize(2));
            if (a.getSize(3)) oob = (oob || c < 0 || c >= a.getSize(3));
            if (oob) return Iter();
            else {
                int xEnd = x+width;
                if (a.getSize(0)) xEnd = std::min(xEnd, a.getSize(0));
                int xStart = std::max(x, 0);
                return Iter(a.scanline(xStart, y, t, c, xEnd-xStart), a.getSize(0));
            }
        }

        bool boundedVecX() const {return a.getSize(0) != 0;}
        int minVecX() const {return 0;}
        int maxVecX() const {return a.getSize(0) - Vec::width;}


        Region transformRegion(Region r) const {
            int xEnd = r.x + r.width;
            int yEnd = r.y + r.height;
            int tEnd = r.t + r.frames;
            int cEnd = r.c + r.channels;
            if (a.getSize(0)) xEnd = std::min(xEnd, a.getSize(0));
            if (a.getSize(1)) yEnd = std::min(yEnd, a.getSize(1));
            if (a.getSize(2)) tEnd = std::min(tEnd, a.getSize(2));
            if (a.getSize(3)) cEnd = std::min(cEnd, a.getSize(3));
            r.x = std::max(r.x, 0);
            r.y = std::max(r.y, 0);
            r.t = std::max(r.t, 0);
            r.c = std::max(r.c, 0);            
            r.width    = xEnd - r.x;
            r.height   = yEnd - r.y;
            r.frames   = tEnd - r.t;
            r.channels = cEnd - r.c;
            return r;
        }
        
        void prepare(int phase, Region r) const { 
            a.prepare(phase, transformRegion(r));
        }

        std::pair<float, float> bounds(Region r) const {
            std::pair<float, float> a_bounds = a.bounds(transformRegion(r));
            return std::make_pair(std::min(0.0f, a_bounds.first),
                                  std::max(0.0f, a_bounds.second));
        }
        
    };

    template<typename A>
    _ZeroBoundary<typename A::FloatExpr>
    zeroBoundary(const A &a) {
        return _ZeroBoundary<A>(a);
    }

    // interleavings
    template<typename A, typename B>
    struct _InterleaveX {
        typedef _InterleaveX<typename A::FloatExpr, typename B::FloatExpr> FloatExpr;
        const A a;
        const B b;
        
        _InterleaveX(const A &a_, const B &b_) : a(a_), b(b_) {
            for (int i = 0; i < 4; i++) {
                if (a.getSize(i) && b.getSize(i)) {
                    assert(a.getSize(i) == b.getSize(i),
                           "Can only combine images with matching size\n");
                }
            }
        }

        int getSize(int i) const {
            if (i == 0) {
                return std::max(a.getSize(0), b.getSize(0))*2;
            } else {
                return std::max(a.getSize(i), b.getSize(i));
            }
        }

        struct Iter {
            const typename A::Iter a;
            const typename B::Iter b;
            Iter() {}
            Iter(const typename A::Iter &a_, const typename B::Iter &b_) : a(a_), b(b_) {}
            float operator[](int x) const {
                if (x & 1) return b[x/2];
                else return a[x/2];
            }
            Vec::type vec(int x) const {                
                const int y = x/2;
                Vec::type v1, v2;
                if (x & 1) {
                    v1 = b.vec(y);
                    v2 = a.vec(y+1);
                } else {
                    v1 = a.vec(y);
                    v2 = b.vec(y);
                }
                return Vec::interleave(v1, v2);
            }
        };
 
        Iter scanline(int x, int y, int t, int c, int width) const {
            return Iter(a.scanline((x+1)/2, y, t, c, width/2), 
                        b.scanline(x/2, y, t, c, width/2));
        }

        bool boundedVecX() const {return a.boundedVecX() || b.boundedVecX();}
        int minVecX() const {return std::max(2*a.minVecX(), 2*b.minVecX());}
        int maxVecX() const {return std::min(2*a.maxVecX(), 2*b.maxVecX()) - Vec::width*2;}
        
        Region transformRegionA(Region r) const {
            r.width = (r.width+1)/2;
            r.x = (r.x+1)/2;
            return r;
        }

        Region transformRegionB(Region r) const {
            r.width /= 2;
            r.x /= 2;
            return r;
        }

        void prepare(int phase, Region r) const { 
            a.prepare(phase, transformRegionA(r));
            b.prepare(phase, transformRegionB(r));
        }

        std::pair<float, float> bounds(Region r) const {
            std::pair<float, float> a_bounds = a.bounds(transformRegionA(r));
            std::pair<float, float> b_bounds = b.bounds(transformRegionB(r));
            return std::make_pair(std::min(a_bounds.first, b_bounds.first),
                                  std::max(a_bounds.second, b_bounds.second));
        }
        
    };

        
    template<typename A, typename B>
    _InterleaveX<typename A::FloatExpr, typename B::FloatExpr>
    interleaveX(const A &a, const B &b) {
        return _InterleaveX<A, B>(a, b);
    }

    template<typename B>
    _InterleaveX<ConstFloat, typename B::FloatExpr>
    interleaveX(float a, const B &b) {
        return _InterleaveX<ConstFloat, B>(ConstFloat(a), b);
    }

    template<typename A>
    _InterleaveX<typename A::FloatExpr, ConstFloat>
    interleaveX(const A &a, float b) {
        return _InterleaveX<A, ConstFloat>(a, ConstFloat(b));
    }

    inline _InterleaveX<ConstFloat, ConstFloat>
    interleaveX(float a, float b) {
        return _InterleaveX<ConstFloat, ConstFloat>(ConstFloat(a), ConstFloat(b));
    }

    // An iterator which is either one thing or another depending on a runtime bool
    template<typename A, typename B>
    struct AltIter {
        const typename A::Iter a;
        const typename B::Iter b;
        const bool useA;

        static AltIter<A, B> fromA(const typename A::Iter &a) {
            return AltIter<A, B>(a, typename B::Iter(), true);
        }        
        static AltIter<A, B> fromB(const typename B::Iter &b) {
            return AltIter<A, B>(typename A::Iter(), b, false);
        }
        AltIter() : useA(false) {}
        AltIter(const typename A::Iter &a_, const typename B::Iter &b_, bool useA_) : 
            a(a_), b(b_), useA(useA_) {
        }
        float operator[](int x) const {
            if (useA) return a[x];
            else return b[x];
        };
        Vec::type vec(int x) const {
            if (useA) return a.vec(x);
            else return b.vec(x);
        }
    };

    template<typename A, typename B>
    struct _InterleaveY {
        typedef _InterleaveY<typename A::FloatExpr, typename B::FloatExpr> FloatExpr;
        const A a;
        const B b;
        
        _InterleaveY(const A &a_, const B &b_) : a(a_), b(b_) {
            for (int i = 0; i < 4; i++) {
                if (a.getSize(i) && b.getSize(i)) {
                    assert(a.getSize(i) == b.getSize(i),
                           "Can only combine images with matching size\n");
                }
            }
        }

        int getSize(int i) const {
            if (i == 1) {
                return std::max(a.getSize(1), b.getSize(1))*2;
            } else {
                return std::max(a.getSize(i), b.getSize(i));
            }
        }

        typedef AltIter<A, B> Iter;

        Iter scanline(int x, int y, int t, int c, int width) const {
            if (y & 1) Iter::fromB(b.scanline(x, y/2, t, c, width));
            return Iter::fromA(a.scanline(x, y/2, t, c, width));
        }

        bool boundedVecX() const {return a.boundedVecX() || b.boundedVecX();}
        int minVecX() const {return std::max(a.minVecX(), b.minVecX());}
        int maxVecX() const {return std::min(a.maxVecX(), b.maxVecX());}
        
        Region transformRegionA(Region r) const {
            r.height = (r.height+1)/2;
            r.y = (r.y+1)/2;
            return r;
        }

        Region transformRegionB(Region r) const {
            r.height /= 2;
            r.y /= 2;
            return r;
        }

        void prepare(int phase, Region r) const { 
            a.prepare(phase, transformRegionA(r));
            b.prepare(phase, transformRegionB(r));
        }

        std::pair<float, float> bounds(Region r) const {
            std::pair<float, float> a_bounds = a.bounds(transformRegionA(r));
            std::pair<float, float> b_bounds = b.bounds(transformRegionB(r));
            return std::make_pair(std::min(a_bounds.first, b_bounds.first),
                                  std::max(a_bounds.second, b_bounds.second));
        }

    };

    template<typename A, typename B>
    _InterleaveY<typename A::FloatExpr, typename B::FloatExpr>
    interleaveY(const A &a, const B &b) {
        return _InterleaveY<A, B>(a, b);
    }

    template<typename B>
    _InterleaveY<ConstFloat, typename B::FloatExpr>
    interleaveY(float a, const B &b) {
        return _InterleaveY<ConstFloat, B>(ConstFloat(a), b);
    }

    template<typename A>
    _InterleaveY<typename A::FloatExpr, ConstFloat>
    interleaveY(const A &a, float b) {
        return _InterleaveY<A, ConstFloat>(a, ConstFloat(b));
    }

    inline _InterleaveY<ConstFloat, ConstFloat>
    interleaveY(float a, float b) {
        return _InterleaveY<ConstFloat, ConstFloat>(ConstFloat(a), ConstFloat(b));
    }

    template<typename A, typename B>
    struct _InterleaveT {
        typedef _InterleaveT<typename A::FloatExpr, typename B::FloatExpr> FloatExpr;
        const A a;
        const B b;
        
        _InterleaveT(const A &a_, const B &b_) : a(a_), b(b_) {
            for (int i = 0; i < 4; i++) {
                if (a.getSize(i) && b.getSize(i)) {
                    assert(a.getSize(i) == b.getSize(i),
                           "Can only combine images with matching size\n");
                }
            }
        }

        int getSize(int i) const {
            if (i == 2) {
                return std::max(a.getSize(2), b.getSize(2))*2;
            } else {
                return std::max(a.getSize(i), b.getSize(i));
            }
        }

        typedef AltIter<A, B> Iter;
        
        Iter scanline(int x, int y, int t, int c, int width) const {
            if (t & 1) return Iter::fromB(b.scanline(x, y, t/2, c, width));
            else return Iter::fromA(a.scanline(x, y, t/2, c, width));
        }

        bool boundedVecX() const {return a.boundedVecX() || b.boundedVecX();}
        int minVecX() const {return std::max(a.minVecX(), b.minVecX());}
        int maxVecX() const {return std::min(a.maxVecX(), b.maxVecX());}

        Region transformRegionA(Region r) const {
            r.frames = (r.frames+1)/2;
            r.t = (r.t+1)/2;
            return r;
        }

        Region transformRegionB(Region r) const {
            r.frames /= 2;
            r.t /= 2;
            return r;
        }

        void prepare(int phase, Region r) const { 
            a.prepare(phase, transformRegionA(r));
            b.prepare(phase, transformRegionB(r));
        }

        std::pair<float, float> bounds(Region r) const {
            std::pair<float, float> a_bounds = a.bounds(transformRegionA(r));
            std::pair<float, float> b_bounds = b.bounds(transformRegionB(r));
            return std::make_pair(std::min(a_bounds.first, b_bounds.first),
                                  std::max(a_bounds.second, b_bounds.second));
        }

    };



    template<typename A, typename B>
    _InterleaveT<typename A::FloatExpr, typename B::FloatExpr>
    interleaveT(const A &a, const B &b) {
        return _InterleaveT<A, B>(a, b);
    }
    template<typename B>
    _InterleaveT<ConstFloat, typename B::FloatExpr>
    interleaveT(float a, const B &b) {
        return _InterleaveT<ConstFloat, B>(ConstFloat(a), b);
    }

    template<typename A>
    _InterleaveT<typename A::FloatExpr, ConstFloat>
    interleaveT(const A &a, float b) {
        return _InterleaveT<A, ConstFloat>(a, ConstFloat(b));
    }

    inline _InterleaveT<ConstFloat, ConstFloat>
    interleaveT(float a, float b) {
        return _InterleaveT<ConstFloat, ConstFloat>(ConstFloat(a), ConstFloat(b));
    }

    template<typename A, typename B>
    struct _InterleaveC {
        typedef _InterleaveC<typename A::FloatExpr, typename B::FloatExpr> FloatExpr;
        const A a;
        const B b;
        
        _InterleaveC(const A &a_, const B &b_) : a(a_), b(b_) {
            for (int i = 0; i < 4; i++) {
                if (a.getSize(i) && b.getSize(i)) {
                    assert(a.getSize(i) == b.getSize(i),
                           "Can only combine images with matching size\n");
                }
            }
        }

        int getSize(int i) const {
            if (i == 3) {
                return std::max(a.getSize(3), b.getSize(3))*2;
            } else {
                return std::max(a.getSize(i), b.getSize(i));
            }
        }

        typedef AltIter<A, B> Iter;

        Iter scanline(int x, int y, int t, int c, int width) const {
            if (c & 1) return Iter::fromB(b.scanline(x, y, t, c/2, width));
            else return Iter::fromA(a.scanline(x, y, t, c/2, width));
        }

        bool boundedVecX() const {return a.boundedVecX() || b.boundedVecX();}
        int minVecX() const {return std::max(a.minVecX(), b.minVecX());}
        int maxVecX() const {return std::min(a.maxVecX(), b.maxVecX());}

        Region transformRegionA(Region r) const {
            r.channels = (r.channels+1)/2;
            r.c = (r.c+1)/2;
            return r;
        }

        Region transformRegionB(Region r) const {
            r.channels /= 2;
            r.c /= 2;
            return r;
        }

        void prepare(int phase, Region r) const { 
            a.prepare(phase, transformRegionA(r));
            b.prepare(phase, transformRegionB(r));
        }

        std::pair<float, float> bounds(Region r) const {
            std::pair<float, float> a_bounds = a.bounds(transformRegionA(r));
            std::pair<float, float> b_bounds = b.bounds(transformRegionB(r));
            return std::make_pair(std::min(a_bounds.first, b_bounds.first),
                                  std::max(a_bounds.second, b_bounds.second));
        }

    };

    template<typename A, typename B>
    _InterleaveC<typename A::FloatExpr, typename B::FloatExpr>
    interleaveC(const A &a, const B &b) {
        return _InterleaveC<A, B>(a, b);
    }

    template<typename B>
    _InterleaveC<ConstFloat, typename B::FloatExpr>
    interleaveC(float a, const B &b) {
        return _InterleaveC<ConstFloat, B>(ConstFloat(a), b);
    }

    template<typename A>
    _InterleaveC<typename A::FloatExpr, ConstFloat>
    interleaveC(const A &a, float b) {
        return _InterleaveC<A, ConstFloat>(a, ConstFloat(b));
    }

    inline _InterleaveC<ConstFloat, ConstFloat>
    interleaveC(float a, float b) {
        return _InterleaveC<ConstFloat, ConstFloat>(ConstFloat(a), ConstFloat(b));
    }

    // Samplings of the form im(stride*x + offset). 
    // Once avx2 becomes available, we can do some of these more efficiently with the new gather op.
    template<typename A>
    struct AffineSampleX {
        typedef AffineSampleX<typename A::FloatExpr> FloatExpr;
        const A a;
        const int stride, offset;
        AffineSampleX(const A &a_, int s, int o) : a(a_), stride(s), offset(o) {
            const int w = a.getSize(0);
            if (w != 0) {
                assert(offset >= 0 && offset < w,
                       "Sampling out of bounds\n");
            }
        }

        int getSize(int i) const {
            if (i == 0) {
                const int w = a.getSize(0);
                if (w) {
                    if (stride > 0) return (w-1-offset)/stride + 1;
                    else if (stride < 0) return offset/(-stride) + 1;
                } 
                return 0;
            }
            else return a.getSize(i);
        }

        struct Iter {
            const typename A::Iter a;
            const int stride, offset;
            Iter(const typename A::Iter &a_, int s, int o) : a(a_), stride(s), offset(o) {}
            
            float operator[](int x) const {
                return a[stride*x + offset];
            }

            Vec::type vec(int x) const {
                // Some special cases
                if (stride == 0) {
                    return Vec::broadcast(a[offset]);
                } else if (stride == 1) {
                    return a.vec(x + offset);
                } else if (stride == 2) {
                    const int x2 = 2*x+offset;
                    Vec::type va = a.vec(x2);
                    Vec::type vb = a.vec(x2 + Vec::width-1);
                    return Vec::subsample(va, vb);
                } else if (stride == -1) {
                    return Vec::reverse(a.vec(-x+offset-Vec::width+1));
                } else {
                    int off = stride * x + offset;
                    if (Vec::width == 8) {
                        return Vec::set(a[off+stride*0], a[off+stride*1], 
                                        a[off+stride*2], a[off+stride*3], 
                                        a[off+stride*4], a[off+stride*5], 
                                        a[off+stride*6], a[off+stride*7]);
                    } else if (Vec::width == 4) {
                        return Vec::set(a[off+stride*0], a[off+stride*1], 
                                        a[off+stride*2], a[off+stride*3]);
                    } else {
                        // In general we scalarize
                        union {
                            float f[Vec::width];
                            Vec::type v;
                        } v;           
                        
                        for (int i = 0; i < Vec::width; i++) {
                            v.f[i] = a[off];
                            off += stride;
                        }
                        return v.v;
                    }
                }
            }
        };
        Iter scanline(int x, int y, int t, int c, int w) const {
            return Iter(a.scanline(x*stride+offset, y, t, c, (w-1)*stride+1), stride, offset);
        }

        bool boundedVecX() const {
            return a.boundedVecX() && (stride == -1 || stride == 1 || stride == 2);
        }

        int minVecX() const {
            if (stride == -1) {
                return -a.maxVecX() + offset - Vec::width + 1;
            } else if (stride == 1) {
                return a.minVecX() - offset;
            } else if (stride == 2) {
                return (a.minVecX() - offset + 1)/2;
            } else {
                return 0xa0000000;
            }
        }

        int maxVecX() const {
            if (stride == -1) {
                return -a.minVecX() + offset - Vec::width + 1;
            } else if (stride == 1) {
                return a.maxVecX() - offset;
            } else if (stride == 2) {
                return (a.maxVecX() - offset - Vec::width + 1)/2;
            } else {
                return 0x3fffffff;
            }
        }
        
        Region transformRegion(Region r) const {
            int x1 = r.x * stride + offset;
            int x2 = (r.x + r.width-1) * stride + offset;
            if (x2 < x1) std::swap(x1, x2);
            r.x = x1;
            r.width = x2-x1+1;
            return r;
        }

        void prepare(int phase, Region r) const { 
            a.prepare(phase, transformRegion(r));
        }

        std::pair<float, float> bounds(Region r) const {
            return a.bounds(transformRegion(r));
        }
        
    };

    template<typename A>
    struct AffineSampleY {
        typedef AffineSampleY<typename A::FloatExpr> FloatExpr;
        const A a;
        const int stride, offset;
        AffineSampleY(const A &a_, int s, int o) : a(a_), stride(s), offset(o) {
            const int w = a.getSize(1);
            if (w != 0) {
                assert(offset >= 0 && offset < w,
                       "Sampling out of bounds\n");
            }
        }

        int getSize(int i) const {
            if (i == 1) {
                const int w = a.getSize(1);
                if (w) {
                    if (stride > 0) return (w-1-offset)/stride + 1;
                    else if (stride < 0) return offset/(-stride) + 1;
                } 
                return 0;
            }
            else return a.getSize(i);
        }

        typedef typename A::Iter Iter;
        Iter scanline(int x, int y, int t, int c, int w) const {
            return a.scanline(x, y*stride+offset, t, c, w);
        }

        bool boundedVecX() const {return a.boundedVecX();}
        int minVecX() const {return a.minVecX();}
        int maxVecX() const {return a.maxVecX();}

        Region transformRegion(Region r) const {
            int y1 = r.y * stride + offset;
            int y2 = (r.y + r.height-1) * stride + offset;
            if (y2 < y1) std::swap(y1, y2);
            r.y = y1;
            r.height = y2-y1+1;
            return r;
        }

        void prepare(int phase, Region r) const { 
            a.prepare(phase, transformRegion(r));
        }

        std::pair<float, float> bounds(Region r) const {
            return a.bounds(transformRegion(r));
        }

    };

    template<typename A>
    struct AffineSampleT {
        typedef AffineSampleT<typename A::FloatExpr> FloatExpr;
        const A a;
        const int stride, offset;
        AffineSampleT(const A &a_, int s, int o) : a(a_), stride(s), offset(o) {
            const int w = a.getSize(2);
            if (w != 0) {
                assert(offset >= 0 && offset < w,
                       "Sampling out of bounds\n");
            }
        }

        int getSize(int i) const {
            if (i == 2) {
                const int w = a.getSize(2);
                if (w) {
                    if (stride > 0) return (w-1-offset)/stride + 1;
                    else if (stride < 0) return offset/(-stride) + 1;
                } 
                return 0;
            }
            else return a.getSize(i);
        }

        typedef typename A::Iter Iter;
        Iter scanline(int x, int y, int t, int c, int w) const {
            return a.scanline(x, y, t*stride+offset, c, w);
        }

        bool boundedVecX() const {return a.boundedVecX();}
        int minVecX() const {return a.minVecX();}
        int maxVecX() const {return a.maxVecX();}

        Region transformRegion(Region r) const {
            int t1 = r.t * stride + offset;
            int t2 = (r.t + r.frames-1) * stride + offset;
            if (t2 < t1) std::swap(t1, t2);
            r.t = t1;
            r.frames = t2-t1+1;
            return r;
        }

        void prepare(int phase, Region r) const { 
            a.prepare(phase, transformRegion(r));
        }

        std::pair<float, float> bounds(Region r) const {
            return a.bounds(transformRegion(r));
        }

    };

    template<typename A>
    struct AffineSampleC {
        typedef AffineSampleC<typename A::FloatExpr> FloatExpr;
        const A a;
        const int stride, offset;
        AffineSampleC(const A &a_, int s, int o) : a(a_), stride(s), offset(o) {
            const int w = a.getSize(3);
            if (w != 0) {
                assert(offset >= 0 && offset < w,
                       "Sampling out of bounds\n");
            }
        }

        int getSize(int i) const {
            if (i == 3) {
                const int w = a.getSize(3);
                if (w) {
                    if (stride > 0) return (w-1-offset)/stride + 1;
                    else if (stride < 0) return offset/(-stride) + 1;
                } 
                return 0;
            }
            else return a.getSize(i);
        }

        typedef typename A::Iter Iter;
        Iter scanline(int x, int y, int t, int c, int w) const {
            return a.scanline(x, y, t, c*stride+offset, w);
        }

        bool boundedVecX() const {return a.boundedVecX();}
        int minVecX() const {return a.minVecX();}
        int maxVecX() const {return a.maxVecX();}
        
        Region transformRegion(Region r) const {
            int c1 = r.c * stride + offset;
            int c2 = (r.c + r.channels-1) * stride + offset;
            if (c2 < c1) std::swap(c1, c2);
            r.c = c1;
            r.channels = c2-c1+1;
            return r;
        }

        void prepare(int phase, Region r) const { 
            a.prepare(phase, transformRegion(r));
        }

        std::pair<float, float> bounds(Region r) const {
            return a.bounds(transformRegion(r));
        }

    };

    template<typename A>
    AffineSampleX<typename A::FloatExpr>
    subsampleX(const A &a, int stride, int offset) {
        return AffineSampleX<A>(a, stride, offset);
    }

    template<typename A>
    AffineSampleY<typename A::FloatExpr>
    subsampleY(const A &a, int stride, int offset) {
        return AffineSampleY<A>(a, stride, offset);
    }

    template<typename A>
    AffineSampleT<typename A::FloatExpr>
    subsampleT(const A &a, int stride, int offset) {
        return AffineSampleT<A>(a, stride, offset);
    }

    template<typename A>
    AffineSampleC<typename A::FloatExpr>
    subsampleC(const A &a, int stride, int offset) {
        return AffineSampleC<A>(a, stride, offset);
    }

    // Convert something to a float expr
    template<typename T, typename Check>
    struct AsFloatExpr;

    template<typename T>
    struct AsFloatExpr<T, typename T::FloatExpr> {
        typedef T t;        
    };

    template<typename T>
    struct AsFloatExpr<T, typename T::IntExpr> {
        typedef IntToFloat<T> t;        
    };

    template<>
    struct AsFloatExpr<float, float> {
        typedef ConstFloat t;
    };

    template<>
    struct AsFloatExpr<double, double> {
        typedef ConstFloat t;
    };

    template<>
    struct AsFloatExpr<int, int> {
        typedef ConstFloat t;
    };

    template<typename T>
    struct AsIntExpr {
        typedef typename T::IntExpr t;        
    };

    template<>
    struct AsIntExpr<int> {
        typedef ConstInt t;
    };

#define FloatExprType(T) typename ImageStack::Expr::AsFloatExpr<T, T>::t
#define IntExprType(T) typename ImageStack::Expr::AsIntExpr<T>::t

    // Evaluated an expression into an array
    template<typename T>
    void setScanline(const T src, float *const dst, 
                     int x, const int maxX,
                     const bool boundedVX, const int minVX, const int maxVX) {

        if (Vec::width > 1 && (maxX - x) > Vec::width*2) {
            //printf("Warm up...\n");
            // Walk up to where we're allowed to start vectorizing
            while (boundedVX && x < std::min(minVX, maxX-1)) {
                dst[x] = src[x];
                x++;
            }

            // Walk a little further for better store alignment            
            while ((size_t)(dst+x) & (Vec::width*sizeof(float) - 1)) {
                dst[x] = src[x];
                x++;
            }

            //printf("Vectorized...\n");
            // Vectorized steady-state until we reach the end or until
            // we're no longer allowed to vectorize
            int lastX = maxX - Vec::width;
            if (boundedVX) lastX = std::min(lastX, maxVX);
            asm("# begin vector");
            while (x <= lastX) {
                Vec::store(src.vec(x), dst+x);
                x += Vec::width;
            }
            asm("# end vector");
        }
        //printf("Wind down...\n");
        // Scalar wind down
        while (x < maxX) {
            dst[x] = src[x];
            x++;
        }        
    }

}

// We need to generate a stupid number of operators to overload.
// Traits can help here, but msvc has quirky behaviour with sfinae, so we'll generate them with macros

// First arg is class to use (FBinaryOp or FCmp or ICmp)
// Second arg is the Symbol (e.g. +)
// Third arg is the Expr::Vec struct that does the operation (e.g. Add)
// There are three macros - the one that takes two expr args,
// and the ones where one of the args is a numeric const (float, int, double).
// In this second case, the fourth arg is the type of the numeric const.
#define MAKE_OP_FF(T, S, N)                                             \
    template<typename A, typename B>                                    \
    Expr::T<typename A::FloatExpr, typename B::FloatExpr, Expr::Vec::N> \
    operator S(const A &a, const B &b) {                                \
        return Expr::T<A, B, Expr::Vec::N>(a, b);                       \
    }

#define MAKE_OP_CF(T, S, N, CT)                                         \
    template<typename B>                                                \
    Expr::T<Expr::ConstFloat, typename B::FloatExpr, Expr::Vec::N>      \
    operator S(CT a, const B &b) {                                      \
        return Expr::T<Expr::ConstFloat, B, Expr::Vec::N>(Expr::ConstFloat(a), b); \
    }

#define MAKE_OP_FC(T, S, N, CT)                                         \
    template<typename A>                                                \
    Expr::T<typename A::FloatExpr, Expr::ConstFloat, Expr::Vec::N>      \
    operator S(const A &a, CT b) {                                      \
        return Expr::T<A, Expr::ConstFloat, Expr::Vec::N>(a, Expr::ConstFloat(b)); \
    }

#define MAKE_OP_II(T, S, N)                                             \
    template<typename A, typename B>                                    \
    Expr::T<typename A::IntExpr, typename B::IntExpr, Expr::Vec::N>     \
    operator S(const A &a, const B &b) {                                \
        return Expr::T<A, B, Expr::Vec::N>(a, b);                       \
    }

#define MAKE_OP_FI(T, S, N)                                             \
    template<typename A, typename B>                                    \
    Expr::T<typename A::FloatExpr, Expr::IntToFloat<typename B::IntExpr>, Expr::Vec::N> \
    operator S(const A &a, const B &b) {                                \
        return a S Expr::IntToFloat<B>(b);                              \
    }

#define MAKE_OP_IF(T, S, N)                                             \
    template<typename A, typename B>                                    \
    Expr::T<Expr::IntToFloat<typename A::IntExpr>, typename B::FloatExpr, Expr::Vec::N> \
    operator S(const A &a, const B &b) {                                \
        return Expr::IntToFloat<A>(a) S b;                              \
    }

#define MAKE_OP_CIF(T, S, N, CT)                                         \
    template<typename B>                                                \
    Expr::T<Expr::ConstFloat, Expr::IntToFloat<typename B::IntExpr>, Expr::Vec::N> \
    operator S(CT a, const B &b) {                                      \
        return a S Expr::IntToFloat<B>(b);                              \
    }

#define MAKE_OP_ICF(T, S, N, CT)                                         \
    template<typename A>                                                \
    Expr::T<Expr::IntToFloat<typename A::IntExpr>, Expr::ConstFloat, Expr::Vec::N> \
    operator S(const A &a, CT b) {                                      \
        return Expr::IntToFloat<A>(a) S b;                              \
    }

#define MAKE_OP_ICI(T, S, N, CT)                                        \
    template<typename A>                                                \
    Expr::T<typename A::IntExpr, Expr::ConstInt, Expr::Vec::N>          \
    operator S(const A &a, CT b) {                                      \
        return Expr::T<A, Expr::ConstInt, Expr::Vec::N>(a, Expr::ConstInt(b)); \
    }

#define MAKE_OP_CII(T, S, N, CT)                                        \
    template<typename B>                                                \
    Expr::T<Expr::ConstInt, typename B::IntExpr, Expr::Vec::N>          \
    operator S(CT a, const B &b) {                                      \
        return Expr::T<Expr::ConstInt, B, Expr::Vec::N>(Expr::ConstInt(a), b); \
    }


// Make the full set of operator overloads for a given operator
#define MAKE_FOP(T, S, N)                        \
    MAKE_OP_FF(T, S, N)                          \
    MAKE_OP_FC(T, S, N, float)                   \
    MAKE_OP_FC(T, S, N, double)                  \
    MAKE_OP_FC(T, S, N, int)                     \
    MAKE_OP_CF(T, S, N, float)                   \
    MAKE_OP_CF(T, S, N, double)                  \
    MAKE_OP_CF(T, S, N, int)                     \
    MAKE_OP_ICF(T, S, N, float)                   \
    MAKE_OP_ICF(T, S, N, double)                  \
    MAKE_OP_CIF(T, S, N, float)                   \
    MAKE_OP_CIF(T, S, N, double)                      

#define MAKE_IOP(T, S, N)                        \
    MAKE_OP_IF(T, S, N)                          \
    MAKE_OP_FI(T, S, N)                          \
    MAKE_OP_II(T, S, N)                          \
    MAKE_OP_ICI(T, S, N, int)                    \
    MAKE_OP_CII(T, S, N, int)

MAKE_FOP(FBinaryOp, +, Add)
MAKE_FOP(FBinaryOp, -, Sub)
MAKE_FOP(FBinaryOp, *, Mul)
MAKE_IOP(IBinaryOp, +, Add)
MAKE_IOP(IBinaryOp, -, Sub)
MAKE_IOP(IBinaryOp, *, Mul)
MAKE_FOP(FCmp, >, GT)
MAKE_FOP(FCmp, <, LT)
MAKE_FOP(FCmp, >=, GE)
MAKE_FOP(FCmp, <=, LE)
MAKE_FOP(FCmp, ==, EQ)
MAKE_FOP(FCmp, !=, NEQ)
MAKE_IOP(ICmp, >, GT)
MAKE_IOP(ICmp, <, LT)
MAKE_IOP(ICmp, >=, GE)
MAKE_IOP(ICmp, <=, LE)
MAKE_IOP(ICmp, ==, EQ)
MAKE_IOP(ICmp, !=, NEQ)

// Unary negation is a special case
template<typename A>
Expr::FBinaryOp<Expr::ConstFloat, typename A::FloatExpr, Expr::Vec::Sub>
operator-(const A &a) {
    return Expr::ConstFloat(0.0f) - a;
}

template<typename A>
Expr::FBinaryOp<Expr::ConstInt, typename A::IntExpr, Expr::Vec::Sub>
operator-(const A &a) {
    return Expr::ConstInt(0) - a;
}

// Division by a scalar is another special case
MAKE_OP_FF(FBinaryOp, /, Div)
MAKE_OP_IF(FBinaryOp, /, Div)
MAKE_OP_FI(FBinaryOp, /, Div)
MAKE_OP_CF(FBinaryOp, /, Div, float)
MAKE_OP_CF(FBinaryOp, /, Div, double)
MAKE_OP_CF(FBinaryOp, /, Div, int)
MAKE_OP_CIF(FBinaryOp, /, Div, float)
MAKE_OP_CIF(FBinaryOp, /, Div, double)
MAKE_OP_II(IBinaryOp, /, Div)
MAKE_OP_CII(IBinaryOp, /, Div, int)
MAKE_OP_ICI(IBinaryOp, /, Div, int)

template<typename A>
Expr::FBinaryOp<typename A::FloatExpr, Expr::ConstFloat, Expr::Vec::Mul>
operator/(const A &a, float b) {
    return a * (1.0f/b);
}
template<typename A>
Expr::FBinaryOp<typename A::FloatExpr, Expr::ConstFloat, Expr::Vec::Mul>
operator/(const A &a, int b) {
    return a * (1.0f/b);
}
template<typename A>
Expr::FBinaryOp<typename A::FloatExpr, Expr::ConstFloat, Expr::Vec::Mul>
operator/(const A &a, double b) {
    return a * (1.0f/b);
}

template<typename A>
Expr::FBinaryOp<Expr::IntToFloat<typename A::IntExpr>, Expr::ConstFloat, Expr::Vec::Mul>
operator/(const A &a, float b) {
    return a * (1.0f/b);
}

template<typename A>
Expr::FBinaryOp<Expr::IntToFloat<typename A::IntExpr>, Expr::ConstFloat, Expr::Vec::Mul>
operator/(const A &a, double b) {
    return a * (1.0f/b);
}

#include "footer.h"

#endif