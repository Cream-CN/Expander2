//copyriht(c) Fish,kotetian,kyodaisuu 2018-2026
//Cream-CN Changed
#ifndef OMEGAY_NOTATION_BMS_FAMILY_HPP
#define OMEGAY_NOTATION_BMS_FAMILY_HPP

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "Header/common/Matrix.hpp"

namespace omegay::notation {
    namespace detail {

        using common::Matrix;

        inline int getParent(const Matrix& S, int r, int c, int nr) {
            for (int i = c; i >= 0; --i)
                if (S(r, i) < S(r, c)) return i;
            (void)nr;
            return -1;
        }

        inline int getParentIB(const Matrix& S, int r, int c, int nr) {
            int x = c, y = r;
            while (x > 0) {
                if (y == 0) x = x - 1;
                else        x = getParentIB(S, y - 1, x, nr);
                if (x < 0) return -1;
                if (S(y, x) < S(y, c)) return x;
            }
            return -1;
        }

        inline int getConcestor(const Matrix& S, int m, int c, int nr) {
            std::vector<int> P(static_cast<std::size_t>(m));
            for (int r = 0; r < m; ++r) P[r] = getParent(S, r, c, nr);
            while (std::find(P.begin(), P.end(), -1) == P.end()) {
                bool all_eq = true;
                for (int i = 1; i < m; ++i) if (P[i] != P[0]) { all_eq = false; break; }
                if (all_eq) return P[0];
                int maxr = m - 1;
                for (int i = 0; i < m; ++i) if (P[i] > P[maxr]) maxr = i;
                P[maxr] = getParent(S, maxr, P[maxr], nr);
            }
            return -1;
        }

        inline int getBadSequence(const Matrix& S, std::vector<int>& Delta,
            std::vector<int>& C, int ver, long n, int nr) {
            const int row = nr - 1;
            int  bad = 0;
            int  found = 0, j = 0, l = 0, m = 0;
            long k = 0, p = 0;

            if (S(0, n) == 0) return 0;

            if (ver == 100) {
                m = row + 1;
                for (j = 0; j <= row; ++j)
                    if (S(j, n) == 0) { m = j; break; }
                for (k = 0; k <= n; ++k) {
                    found = 1;
                    for (l = 0; l < m; ++l)
                        if (S(l, n - k) >= S(l, n)) { found = 0; break; }
                    if (found) {
                        for (l = 0; l <= row; ++l)
                            Delta[l] = (l < m - 1) ? S(l, n) - S(l, n - k) : 0;
                        return static_cast<int>(k);
                    }
                }
                return 0;
            }
            if (ver == 210) {
                m = row + 1;
                for (j = 0; j <= row; ++j)
                    if (S(j, n) == 0) { m = j; break; }
                for (k = 0; k <= n; ++k) {
                    for (l = 1; n - (k + l) >= 0; ++l) {
                        if (S(0, n - (k + l)) < S(0, n - k)) { k = k + l; break; }
                    }
                    found = 1;
                    for (l = 0; l < m; ++l)
                        if (S(l, n - k) >= S(l, n)) { found = 0; break; }
                    if (found) {
                        for (l = 0; l <= row; ++l)
                            Delta[l] = (l < m - 1) ? S(l, n) - S(l, n - k) : 0;
                        bad = static_cast<int>(k);
                        k = n + 1;
                    }
                    else {
                        k = k - 1;
                    }
                }
                return bad;
            }
            if (ver == 220) {
                m = row + 1;
                for (j = 0; j <= row; ++j)
                    if (S(j, n) == 0) { m = j; break; }
                l = getConcestor(S, m, static_cast<int>(n), nr);
                bad = (l >= 0) ? static_cast<int>(n) - l : 0;
                for (j = 0; j <= row; ++j)
                    Delta[j] = (j < m - 1) ? S(j, n) - S(j, n - bad) : 0;
                return bad;
            }

            if (ver == 200 || ver == 230 || ver == 300 || ver == 310 || ver == 320 ||
                ver == 330 || ver == 400) {
                for (m = 0; m <= row; ++m) Delta[m] = 0;
                for (k = 0; k <= n; ++k) {
                    bool done = false;
                    for (l = 0; l <= row; ++l) {
                        if (S(l, n - k) < S(l, n) - Delta[l]) {
                            if (l == row || S(l + 1, n) == 0) {
                                bad = static_cast<int>(k);
                                done = true;
                                break;
                            }
                            else {
                                Delta[l] = S(l, n) - S(l, n - k);
                            }
                        }
                        else {
                            break;
                        }
                    }
                    if (done) break;
                }
            }

            if (ver == 200 || ver == 310) {
                for (k = 1; k <= bad; ++k) {
                    for (l = static_cast<int>(k); l >= 0; --l) {
                        if (S(0, n - bad + l) < S(0, n - bad + k)) {
                            for (m = 0; m <= row; ++m) {
                                C[m + static_cast<int>((k + 1) * nr)] =
                                    (S(m, n - bad) < S(m, n - bad + k) &&
                                        C[m + static_cast<int>((l + 1) * nr)] == 1) ? 1 : 0;
                            }
                            break;
                        }
                    }
                }
            }
            if (ver == 230 || ver == 320) {
                for (j = 0; j < nr; ++j) {
                    for (k = 0; k < bad; ++k) {
                        p = k;
                        while (true) {
                            if (p == 0) { C[j + static_cast<int>((k + 1) * nr)] = 1; break; }
                            if (p < 0) { C[j + static_cast<int>((k + 1) * nr)] = 0; break; }
                            p = getParentIB(S, j, static_cast<int>(p + n - bad), nr) -
                                (n - bad);
                        }
                    }
                }
            }
            if (ver == 310 || ver == 320) {
                for (m = nr - 1; m >= 0; --m)
                    if (S(m, n) != 0) break;
                for (k = 0; k < bad; ++k) {
                    for (l = 0; l <= m; ++l) {
                        if (C[l + static_cast<int>((k + 1) * nr)] == 0) {
                            for (l = 1; l < nr; ++l)
                                C[l + static_cast<int>((k + 1) * nr)] = 0;
                            break;
                        }
                    }
                }
            }
            if (ver == 330) {
                for (m = 0; m <= row; ++m)
                    for (k = 0; k < bad; ++k)
                        C[m + static_cast<int>((k + 1) * nr)] = 0;

                int lmost = 0;
                for (m = 0; m <= row; ++m)
                    if (S(m, n) != 0) lmost = m;

                C[lmost + 1 * nr] = 1;
                for (k = bad - 1; k >= 0; --k) {
                    p = getParentIB(S, lmost, static_cast<int>(n - k), nr);
                    if (p >= 0 && C[lmost + static_cast<int>((p - (n - bad) + 1) * nr)] == 1)
                        C[lmost + static_cast<int>((bad - k + 1) * nr)] = 1;
                }
                for (k = 0; k <= bad; ++k) {
                    if (C[lmost + static_cast<int>((bad - k + 1) * nr)] == 1)
                        for (m = 0; m <= lmost; ++m)
                            C[m + static_cast<int>((bad - k + 1) * nr)] = 1;
                }
                for (m = 0; m < lmost; ++m) {
                    for (k = bad; k > 0; --k) {
                        p = getParentIB(S, m, static_cast<int>(n - k), nr);
                        if (p > n - bad &&
                            C[m + static_cast<int>((p - (n - bad) + 1) * nr)] == 1)
                            C[m + static_cast<int>((bad - k + 1) * nr)] = 1;
                    }
                }
            }
            if (ver == 400) {
                std::vector<int> E(static_cast<std::size_t>(nr), 0);
                for (l = bad; l >= 2; --l) {
                    for (m = 0; m <= row; ++m) {
                        int q = 0;
                        std::fill(E.begin(), E.end(), 0);
                        int pp = 0;
                        for (int n2 = l; n2 <= bad; ++n2) {
                            for (int o = 0; o <= m; ++o) {
                                if (S(o, n - n2) < S(o, n - l + 1) - E[o]) {
                                    if (o == m || (o + 1 < nr && S(o + 1, n - l + 1) == 0)) {
                                        pp = n2;
                                        q = o;
                                        o = m;
                                        n2 = bad;
                                    }
                                    else {
                                        E[o] = S(o, n - l + 1) - S(o, n - n2);
                                    }
                                }
                                else {
                                    o = m;
                                }
                            }
                        }
                        if (C[(bad - pp + 1) * nr + m] == 1 && q == m)
                            C[(bad - l + 2) * nr + m] = 1;
                        else
                            C[(bad - l + 2) * nr + m] = 0;
                    }
                }
            }
            return bad;
        }

        inline void copyBadSequence(Matrix& S, const std::vector<int>& Delta,
            const std::vector<int>& C, int ver,
            long& n, long nn, int nr, int bad) {
            const int row = nr - 1;
            if (bad <= 0) {
                n = nn;
                return;
            }
            if (ver == 100 || ver == 210 || ver == 220) {
                while (n < nn) {
                    for (int l = 0; l <= row; ++l)
                        S(l, n) = S(l, n - bad) + Delta[l];
                    ++n;
                }
                return;
            }
            if (ver == 200 || ver == 230 || ver == 310 || ver == 320 || ver == 330 || ver == 400) {
                int m = 1;
                while (n < nn) {
                    for (int l = 0; l <= row; ++l)
                        S(l, n) = S(l, n - bad) + Delta[l] * C[l + m * nr];
                    ++m;
                    ++n;
                    if (m > bad) m = 1;
                }
                return;
            }
            if (ver == 300) {
                int K = 1;
                while (n < nn) {
                    for (int L = 0; L <= row; ++L) {
                        S(L, n) = S(L, n - bad);
                        int M = 0;
                        for (int m = 0; m <= n; ++m) {
                            if (S(0, n - m) < S(0, n)) { M = m; break; }
                        }
                        if (L == 0 || K == 1 ||
                            (S(L, n - bad - M) < S(L, n - M) &&
                                S(L, n - M) < S(L, n) + Delta[L])) {
                            S(L, n) += Delta[L];
                        }
                    }
                    ++K;
                    ++n;
                    if (K > bad) K = 1;
                }
            }
        }

        inline int chkStd(const Matrix& S, int ver) {
            const int nc = S.cols();
            const int nr = S.rows();
            int row = 0;
            for (int i = 0; i < nc; ++i) {
                if (row + 1 == nr) break;
                for (int j = row + 1; j < nr; ++j)
                    if (S(j, i) > 0) row = j;
            }

            int p = -1;
            for (int i = 0; i < nc && p < 0; ++i) {
                for (int j = 0; j <= row; ++j) {
                    if (S(j, i) > i) return 1;
                    if (S(j, i) < i) { p = i; break; }
                }
            }
            if (p < 0) return 0;

            const int R = row + 1;
            Matrix SA(R, p + 1);
            // 复制 S 的前 p+1 列作为初始 SA。
            // 注意：第 p 列必须用 S 的真实值，不能用理想值 i，
            // 否则等于把待检验的偏差列替换掉，标准式会被误判为非标准。
            for (int i = 0; i <= p; ++i)
                for (int j = 0; j < R; ++j)
                    SA(j, i) = S(j, i);

            std::vector<int> C(static_cast<std::size_t>(R) * (static_cast<std::size_t>(nc) * 2 + 4), 0);
            for (int i = 0; i < R; ++i) C[i + R] = 1;
            std::vector<int> Delta(static_cast<std::size_t>(R), 0);

            while (true) {
                int bad = getBadSequence(SA, Delta, C, ver, p, R);
                if (bad == 0) return 1;
                const int num = (nc + 1 - p) / bad + 1;
                const long nn = p + static_cast<long>(bad) * num;
                long p2 = p;
                SA.resize_cols(static_cast<int>(nn) + 1);
                copyBadSequence(SA, Delta, C, ver, p2, nn, R, bad);
                p2 = -1;
                int p3 = -1;
                for (int i = p; i < nc; ++i) {
                    int smaller = 0, larger = 0;
                    for (int j = 0; j < R; ++j) {
                        if (S(j, i) > SA(j, i)) larger = 1;
                        if (S(j, i) < SA(j, i)) smaller = 1;
                    }
                    if (larger && !smaller) return 1;
                    if (larger || smaller) { p3 = i; break; }
                }
                if (p3 == -1) return 0;
                p = p3;
            }
        }

        inline int cmpSeq(const Matrix& S, const Matrix& S2, int ver) {
            const int nr = S.rows();
            const long nc = S.cols();
            const long nc2 = S2.cols();
            if (S2.rows() != nr) return 0;

            long nc3 = nc < nc2 ? nc : nc2;
            bool found = false;
            long i = 0;
            for (; i < nc3; ++i) {
                for (int j = 0; j < nr; ++j)
                    if (S(j, i) != S2(j, i)) { found = true; break; }
                if (found) break;
            }
            if (!found) {
                if (nc == nc2) return 2;
                if (nc < nc2) return 3;
                return 1;
            }

            Matrix S3 = S;
            std::vector<int> Delta(static_cast<std::size_t>(nr), 0);
            std::vector<int> C(static_cast<std::size_t>(nr) *
                (static_cast<std::size_t>(nc + nc2) * 2 + 4), 0);
            for (int j = 0; j < nr; ++j) C[j + nr] = 1;

            while (true) {
                const int bad = getBadSequence(S3, Delta, C, ver, i, nr);
                if (bad == 0) return 3;
                long n = i;
                S3.resize_cols(static_cast<int>(i) + 2);
                copyBadSequence(S3, Delta, C, ver, n, i + 1, nr, bad);
                bool eq = true;
                for (int j = 0; j < nr; ++j)
                    if (S2(j, i) != S3(j, i)) { eq = false; break; }
                if (eq) return 1;
            }
        }

        inline std::string getOrd(const Matrix& S, int ver) {
            const long nc = S.cols();
            const int  nr = S.rows();
            if (chkStd(S, ver) != 0) return "Not standard";

            if (nc == 1) return "1";

            for (long i = nc - 1; i > 0; --i) {
                bool found = false;
                for (int j = 0; j < nr; ++j) if (S(j, i) > 0) { found = true; break; }
                if (!found) {
                    if (i == nc - 1) {
                        long k = nc - 1;
                        for (; k >= 0; --k) {
                            bool any = false;
                            for (int j = 0; j < nr; ++j) if (S(j, k) > 0) { any = true; break; }
                            if (any) break;
                        }
                        ++k;
                        if (k == 0) return std::to_string(nc);
                        Matrix L(nr, static_cast<int>(k));
                        for (int c = 0; c < static_cast<int>(k); ++c)
                            for (int r = 0; r < nr; ++r) L(r, c) = S(r, c);
                        return getOrd(L, ver) + "+" + std::to_string(nc - k);
                    }
                    Matrix L(nr, static_cast<int>(i));
                    for (int c = 0; c < static_cast<int>(i); ++c)
                        for (int r = 0; r < nr; ++r) L(r, c) = S(r, c);
                    Matrix R(nr, static_cast<int>(nc - i));
                    for (int c = 0; c < static_cast<int>(nc - i); ++c)
                        for (int r = 0; r < nr; ++r) R(r, c) = S(r, static_cast<int>(i) + c);
                    return getOrd(L, ver) + "+" + getOrd(R, ver);
                }
            }

            for (long i = 1; i < nc; ++i) {
                bool found = false;
                if (S(0, i) != 1) found = true;
                for (int j = 1; j < nr; ++j) if (S(j, i) > 0) found = true;
                if (found) continue;

                if (i == nc - 1) {
                    if (i == 1) return "w";
                    Matrix L(nr, static_cast<int>(i));
                    for (int c = 0; c < static_cast<int>(i); ++c)
                        for (int r = 0; r < nr; ++r) L(r, c) = S(r, c);
                    return "(" + getOrd(L, ver) + ")w";
                }
                return "(" + std::to_string(nc) + ")";
            }

            if (nr == 2 && S(0, 1) == 1 && S(1, 1) == 1) {
                if (nc == 2) return "e_0";
                if (S(0, 2) == 2 && S(1, 2) > 0) {
                    std::string out = "p0(";
                    for (long i = 1; i < nc; ++i) {
                        if (S(0, i) > S(0, i - 1)) {
                            out += "p" + std::to_string(S(1, i)) + "(";
                        }
                        else {
                            out += "0";
                            for (int j = S(0, i); j < S(0, i - 1) + 1; ++j) out += ")";
                            out += "+p" + std::to_string(S(1, i)) + "(";
                        }
                    }
                    out += "0";
                    for (int j = 0; j < S(0, nc - 1) + 1; ++j) out += ")";
                    return out;
                }
            }

            return common::matrix_to_string(S);
        }

    }  // namespace detail

    // =========================================================================
    // BMSFamilyNotation：detail 已在前方定义，可正常引用
    // =========================================================================
    struct BMSFamilyNotation {
        enum class Version : int {
            V1 = 100,
            V2 = 200,
            V21 = 210,
            V22 = 220,
            V23 = 230,
            V3 = 300,
            V31 = 310,
            V32 = 320,
            V33 = 330,
            V4 = 400,
        };

        static constexpr const char* kName = "BMS Family";
        static std::string suffix() { return {}; }

        static common::Matrix expand(const common::Matrix& m, int term,
            Version ver = Version::V4) {
            common::Matrix S = m;
            if (S.rows() == 0 || S.cols() == 0) return S;

            const int  nr = S.rows();
            const long n = static_cast<long>(S.cols()) - 1;
            long num = term < 1 ? 1 : term;

            std::vector<int> Delta(static_cast<std::size_t>(nr), 0);
            // C 参考实现里 C 的大小是 nr * num * (nc+1)：
            // getBadSequence 会写到 (k+1) 列，k 最大到 bad-1 ≤ n，
            // 后续 copyBadSequence 会写到 m 列，m ≤ bad ≤ n。
            // 因此按 num*(n+1) 开列数，覆盖所有写点。
            const long cap_cols = num * (n + 1) + 2;
            std::vector<int> C(static_cast<std::size_t>(nr) *
                static_cast<std::size_t>(cap_cols), 0);
            for (int r = 0; r < nr; ++r) C[r + nr] = 1;

            const int bad = detail::getBadSequence(S, Delta, C, static_cast<int>(ver), n, nr);
            if (bad <= 0) {
                if (S.cols() > 1) S.resize_cols(S.cols() - 1);
                else S.resize_cols(0);
                return S;
            }

            const long nn = n + static_cast<long>(bad) * num;
            S.resize_cols(static_cast<int>(nn) + 1);

            // copyBadSequence 的第 5 个参数是 long&（会被修改），
            // 而这里的 n 是 const long，不能直接传，必须用非 const 副本。
            long nCopy = n;
            detail::copyBadSequence(S, Delta, C, static_cast<int>(ver), nCopy, nn, nr, bad);

            S.resize_cols(static_cast<int>(nn) + 1);
            return S;
        }

        static bool is_standard(const common::Matrix& m, Version ver = Version::V4) {
            return detail::chkStd(m, static_cast<int>(ver)) == 0;
        }

        static int compare(const common::Matrix& a, const common::Matrix& b,
            Version ver = Version::V4) {
            const int r = detail::cmpSeq(a, b, static_cast<int>(ver));
            if (r == 1) return +1;
            if (r == 2) return  0;
            if (r == 3) return -1;
            return 0;
        }

        static std::string ordinal(const common::Matrix& m, Version ver = Version::V4) {
            return detail::getOrd(m, static_cast<int>(ver));
        }
    };

}  // namespace omegay::notation

#endif  // OMEGAY_NOTATION_BMS_FAMILY_HPP