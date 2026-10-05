#ifndef OMEGAY_COMMON_MATRIX_HPP
#define OMEGAY_COMMON_MATRIX_HPP

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace omegay::common {

    // 列主序整数矩阵：元素 (r, c) 位于 raw()[c * rows() + r]。
    // 行数在构造时固定，列数可动态增长（与 BMS 的序列增长语义一致）。
    class Matrix {
    public:
        Matrix() = default;

        Matrix(int rows, int cols)
            : rows_(rows), cols_(cols),
            data_(static_cast<std::size_t>(rows)* static_cast<std::size_t>(cols), 0) {
            if (rows < 0 || cols < 0)
                throw std::invalid_argument("Matrix: negative dimension");
        }

        int  rows() const noexcept { return rows_; }
        int  cols() const noexcept { return cols_; }
        bool empty() const noexcept { return rows_ == 0 || cols_ == 0; }

        int& operator()(int r, int c)       noexcept { return data_[index(r, c)]; }
        int        operator()(int r, int c) const noexcept { return data_[index(r, c)]; }

        int* col(int c)       noexcept { return data_.data() + static_cast<std::size_t>(c) * rows_; }
        const int* col(int c) const noexcept { return data_.data() + static_cast<std::size_t>(c) * rows_; }

        const std::vector<int>& raw() const noexcept { return data_; }
        std::vector<int>& raw()       noexcept { return data_; }

        // 修改列数（保留已有列，新列以 0 填充）。
        void resize_cols(int new_cols) {
            if (new_cols < 0)
                throw std::invalid_argument("Matrix: negative cols");
            if (new_cols == cols_) return;
            std::vector<int> nd(static_cast<std::size_t>(rows_) * new_cols, 0);
            const int copy = cols_ < new_cols ? cols_ : new_cols;
            for (int c = 0; c < copy; ++c) {
                const int* src = col(c);
                int* dst = nd.data() + static_cast<std::size_t>(c) * rows_;
                std::copy(src, src + rows_, dst);
            }
            data_.swap(nd);
            cols_ = new_cols;
        }

        void push_col(const int* values) {
            const std::size_t old = data_.size();
            data_.resize(old + static_cast<std::size_t>(rows_));
            std::copy(values, values + rows_, data_.begin() + static_cast<std::ptrdiff_t>(old));
            ++cols_;
        }

        bool operator==(const Matrix& o) const noexcept {
            return rows_ == o.rows_ && cols_ == o.cols_ && data_ == o.data_;
        }
        bool operator!=(const Matrix& o) const noexcept { return !(*this == o); }

    private:
        std::size_t index(int r, int c) const noexcept {
            return static_cast<std::size_t>(c) * static_cast<std::size_t>(rows_) +
                static_cast<std::size_t>(r);
        }

        int rows_ = 0;
        int cols_ = 0;
        std::vector<int> data_;
    };

    // 解析形如 "(0,0)(1,1)[3]" 的 BMS 表达式。
    // 返回值为矩阵；若出现 "[n]"，则将 n 写入 term；否则 term 保持原值。
    //
    // 与 C 参考实现 getMatrix() 对齐的关键点：
    //   - 每遇到 '('、','、')' 都要把"当前格子已开始读数字"标记复位，
    //     否则上一列的残留值会被当作本列前缀参与累加。
    //   - cur[] 显式清零，杜绝栈上脏值。
    inline Matrix parse_matrix(std::string_view s, long& term) {
        // 第一次扫描：确定行数 nr。
        int nr = 0;
        {
            bool in_paren = false;
            int  row_cnt = 0;
            for (char ch : s) {
                if (ch == '[') break;
                if (ch == '(') { in_paren = true; row_cnt = 1; }
                else if (ch == ',') { if (in_paren) ++row_cnt; }
                else if (ch == ')') {
                    if (in_paren && row_cnt > nr) nr = row_cnt;
                    in_paren = false;
                }
            }
        }
        if (nr <= 0) return Matrix{};

        // 第二次扫描：逐列填充。
        std::vector<int> flat;
        int  cur[64] = { 0 };
        int  row = 0;
        bool has_digit = false;

        auto flush_col = [&]() {
            for (int r = 0; r < nr; ++r) {
                const int v = (r < row) ? cur[r] : 0;
                flat.push_back(v);
            }
            row = 0;
            has_digit = false;
            };

        for (std::size_t i = 0; i < s.size(); ++i) {
            const char ch = s[i];
            if (ch == '(') {
                row = 0;
                has_digit = false;
            }
            else if (ch == ',') {
                ++row;
                has_digit = false;
            }
            else if (ch == ')') {
                ++row;
                flush_col();
            }
            else if (ch == '[') {
                // 解析 [n]
                long n = 0;
                ++i;
                while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
                    n = n * 10 + (s[i] - '0');
                    ++i;
                }
                term = n;
                break;
            }
            else if (std::isdigit(static_cast<unsigned char>(ch))) {
                if (row >= 64) continue;
                if (!has_digit) {
                    cur[row] = 0;
                    has_digit = true;
                }
                cur[row] = cur[row] * 10 + (ch - '0');
            }
            else if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
                // skip
            }
            else {
                has_digit = false;
            }
        }

        const int cols = static_cast<int>(flat.size() / static_cast<std::size_t>(nr));
        Matrix m(nr, cols);
        for (int c = 0; c < cols; ++c)
            for (int r = 0; r < nr; ++r)
                m(r, c) = flat[static_cast<std::size_t>(c) * nr + r];
        return m;
    }

    // 序列化为 "(a,b)(c,d)..."，若 term > 0 追加 "[term]"。
    inline std::string matrix_to_string(const Matrix& m, long term = 0) {
        std::string out;
        for (int c = 0; c < m.cols(); ++c) {
            out += '(';
            for (int r = 0; r < m.rows(); ++r) {
                if (r) out += ',';
                out += std::to_string(m(r, c));
            }
            out += ')';
        }
        if (term > 0) {
            out += '[';
            out += std::to_string(term);
            out += ']';
        }
        return out;
    }

}  // namespace omegay::common

#endif  // OMEGAY_COMMON_MATRIX_HPP