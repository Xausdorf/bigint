#include "big_integer.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <functional>
#include <limits>
#include <ostream>
#include <stdexcept>

big_integer::big_integer() = default;

big_integer::big_integer(const big_integer& other) = default;

big_integer::big_integer(int a) : big_integer(static_cast<long long>(a)) {}

big_integer::big_integer(unsigned int a) : big_integer(static_cast<unsigned long long>(a)) {}

big_integer::big_integer(long a) : big_integer(static_cast<long long>(a)) {}

big_integer::big_integer(unsigned long a) : big_integer(static_cast<unsigned long long>(a)) {}

big_integer::big_integer(long long a)
        : big_integer(static_cast<unsigned long long>(a == INT64_MIN ? INT64_MAX + 1ull : std::abs(a))) {
    is_negative = a < 0;
}

big_integer::big_integer(unsigned long long a) : big_integer() {
    do {
        value.push_back(static_cast<uint32_t>(a & UINT32_MAX));
        a >>= std::numeric_limits<uint32_t>::digits;
    } while (a > 0);
    skip_leading_zeros();
}

big_integer::big_integer(const std::string& str) : big_integer() {
    for (size_t i = 1; i < str.size(); ++i) {
        if (str[i] < '0' || str[i] > '9') {
            throw std::invalid_argument("String is not a number");
        }
    }
    if (str.empty()) {
        throw std::invalid_argument("String is empty");
    }
    size_t i = 0;
    if (str[0] == '+' || str[0] == '-') {
        ++i;
        if (str.size() == 1) {
            throw std::invalid_argument(R"(String can't be only "-" or "+")");
        }
    }
    uint32_t cur_digit = 0;
    uint32_t cur_radix = 1;
    for (; i < str.size(); ++i) {
        cur_digit = cur_digit * big_integer::CHAR_RADIX + (str[i] - '0');
        cur_radix *= big_integer::CHAR_RADIX;
        if (cur_radix == big_integer::STRING_RADIX) {
            this->mul_to_short(big_integer::STRING_RADIX);
            this->add_to_short(cur_digit);
            cur_digit = 0;
            cur_radix = 1;
        }
    }
    if (cur_radix != 1) {
        this->mul_to_short(cur_radix);
        this->add_to_short(cur_digit);
    }
    if (*this != 0) {
        is_negative = str[0] == '-';
    }
}

big_integer::~big_integer() = default;

big_integer& big_integer::operator=(const big_integer& other) {
    if (*this != other) {
        big_integer(other).swap(*this);
    }
    return *this;
}

void big_integer::swap(big_integer& other) {
    std::swap(value, other.value);
    std::swap(is_negative, other.is_negative);
}

big_integer& big_integer::operator+=(const big_integer& rhs) {
    if (is_negative != rhs.is_negative) {
        is_negative = !is_negative;
        *this -= rhs;
        if (*this != 0) {
            is_negative = !is_negative;
        }
        return *this;
    }
    size_t left_size = value.size();
    size_t right_size = rhs.value.size();
    value.resize(std::max(left_size, right_size) + 1);
    bool carry = false;
    for (size_t i = 0; i < value.size() - 1; ++i) {
        uint64_t digit = (carry ? 1 : 0);
        if (i < left_size) {
            digit += value[i];
        }
        if (i < right_size) {
            digit += rhs.value[i];
        }
        carry = digit > UINT32_MAX;
        if (carry) {
            digit -= UINT32_MAX;
            --digit;
        }
        value[i] = static_cast<uint32_t>(digit);
    }
    if (carry) {
        value[value.size() - 1] = 1;
    } else {
        value.pop_back();
    }
    return *this;
}

big_integer& big_integer::add_to_short(uint32_t rhs) {
    if (is_negative) {
        is_negative = false;
        sub_to_short(rhs);
        if (*this != 0) {
            is_negative = true;
        }
        return *this;
    }
    if (value.empty()) {
        value.resize(1, rhs);
        return *this;
    }
    uint64_t first_digit = static_cast<uint64_t>(value[0]) + static_cast<uint64_t>(rhs);
    bool carry = first_digit > UINT32_MAX;
    if (carry) {
        first_digit -= UINT32_MAX;
        --first_digit;
        for (size_t i = 1; i < value.size(); ++i) {
            if (value[i] == UINT32_MAX) {
                value[i] = 0;
            } else {
                ++value[i];
                carry = false;
                break;
            }
        }
        if (carry) {
            value.push_back(1);
        }
    }
    value[0] = static_cast<uint32_t>(first_digit);
    return *this;
}

big_integer& big_integer::sub_to_short(uint32_t rhs) {
    if (is_negative) {
        is_negative = false;
        *this += rhs;
        if (*this != 0) {
            is_negative = true;
        }
        return *this;
    }
    if (value.empty()) {
        value.resize(1, rhs);
        is_negative = true;
        return *this;
    }
    int64_t first_digit = static_cast<int64_t>(value[0]) - static_cast<int64_t>(rhs);
    if (first_digit < 0) {
        if (value.size() == 1) {
            value[0] = static_cast<uint32_t>(std::abs(first_digit));
            is_negative = true;
            return *this;
        } else {
            first_digit += UINT32_MAX;
            first_digit++;
            for (size_t i = 1; i < value.size(); ++i) {
                if (value[i] == 0) {
                    value[i] = UINT32_MAX;
                } else {
                    --value[i];
                    break;
                }
            }
            skip_leading_zeros();
        }
    }
    value[0] = static_cast<uint32_t>(first_digit);
    return *this;
}

void big_integer::skip_leading_zeros() {
    while (value.size() > 1 && value.back() == 0) {
        value.pop_back();
    }
}

big_integer& big_integer::operator-=(const big_integer& rhs) {
    if (*this == rhs) {
        return *this = 0;
    }
    if (is_negative != rhs.is_negative) {
        is_negative = !is_negative;
        *this += rhs;
        if (*this != 0) {
            is_negative = !is_negative;
        }
        return *this;
    }
    if (rhs.value.size() > value.size()) {
        value.resize(rhs.value.size(), 0);
    }
    bool is_abs_left_greater = (*this > rhs) != is_negative;
    bool borrow = false;
    for (size_t i = 0; i < value.size(); ++i) {
        int64_t digit = (is_abs_left_greater ? value[i] : rhs.value[i]);
        if (i < rhs.value.size()) {
            digit -= (is_abs_left_greater ? rhs.value[i] : value[i]);
        }
        if (borrow) {
            --digit;
        }
        borrow = digit < 0;
        value[i] = static_cast<uint32_t>(digit);
    }
    is_negative = is_negative == is_abs_left_greater;
    skip_leading_zeros();
    return *this;
}

big_integer& big_integer::operator*=(const big_integer& rhs) {
    if (*this == 0 || rhs == 0) {
        return *this = 0;
    }
    size_t left_size = value.size();
    size_t right_size = rhs.value.size();
    uint32_t carry = 0;
    value.resize(left_size + right_size + 1, 0);
    for (size_t i = left_size; i > 0; --i) {
        carry = 0;
        uint64_t cur_digit = value[i - 1];
        for (size_t j = 0; j < right_size || carry > 0; ++j) {
            uint64_t cur = carry;
            if (j < right_size) {
                cur += cur_digit * static_cast<uint64_t>(rhs.value[j]);
            }
            if (j > 0) {
                cur += value[i + j - 1];
            }
            value[i + j - 1] = static_cast<uint32_t>(cur);
            carry = static_cast<uint32_t>(cur >> std::numeric_limits<uint32_t>::digits);
        }
    }
    is_negative = is_negative != rhs.is_negative;
    skip_leading_zeros();
    return *this;
}

big_integer& big_integer::mul_to_short(uint32_t rhs) {
    if (*this == 0 || rhs == 0) {
        return *this = 0;
    }
    uint32_t carry = 0;
    for (size_t i = 0; i < value.size(); ++i) {
        uint64_t cur = static_cast<uint64_t>(value[i]) * static_cast<uint64_t>(rhs) + carry;
        value[i] = static_cast<uint32_t>(cur);
        carry = static_cast<uint32_t>(cur >> std::numeric_limits<uint32_t>::digits);
    }
    if (carry != 0) {
        value.push_back(carry);
    }
    return *this;
}

big_integer big_integer::mul_to_short(const big_integer& a, uint32_t rhs) {
    return big_integer(a).mul_to_short(rhs);
}

std::pair<big_integer, big_integer> big_integer::division(const big_integer& a, const big_integer& b) {
    uint32_t f = static_cast<uint32_t>(big_integer::RADIX / (b.value.back() + 1));
    big_integer r = a * f;
    big_integer d = b * f;
    r.is_negative = false;
    d.is_negative = false;
    big_integer result;
    size_t result_size = r.value.size() - d.value.size();
    result.value = std::vector<uint32_t>(result_size + 1);
    result.value[result_size] = 0;
    if (r >= (d << (std::numeric_limits<uint32_t>::digits * result_size))) {
        result.value[result_size] = 1;
        r -= d << (std::numeric_limits<uint32_t>::digits * result_size);
    }
    for (size_t i = result_size; i > 0; --i) {
        uint64_t r3 = 0;
        if (i + b.value.size() - 1 < r.value.size()) {
            r3 += static_cast<uint64_t>(r.value[i + b.value.size() - 1]) << std::numeric_limits<uint32_t>::digits;
        }
        if (i + b.value.size() - 1 <= r.value.size()) {
            r3 += r.value[i + b.value.size() - 2];
        }
        uint64_t d2 = d.value[b.value.size() - 1];
        uint32_t trial = static_cast<uint32_t>(std::min(r3 / d2, static_cast<uint64_t>(UINT32_MAX)));
        big_integer dq = mul_to_short(d, trial) << (std::numeric_limits<uint32_t>::digits * (i - 1));
        if (r < dq) {
            --trial;
            dq = mul_to_short(d, trial) << (std::numeric_limits<uint32_t>::digits * (i - 1));
        }
        result.value[i - 1] = trial;
        r -= dq;
    }
    r.div_to_short(f);
    r.is_negative = a.is_negative;
    r.skip_leading_zeros();
    result.skip_leading_zeros();
    result.is_negative = a.is_negative != b.is_negative;
    return {result, r};
}

big_integer& big_integer::operator/=(const big_integer& rhs) {
    if (rhs.value.size() == 1) {
        this->div_to_short(rhs.value[0]);
        if (*this != 0 && rhs.is_negative) {
            is_negative = !is_negative;
        }
        return *this;
    }
    if (value.size() < rhs.value.size()) {
        return *this = 0;
    }
    if (*this == 0) {
        return *this = 0;
    }
    return *this = division(*this, rhs).first;
}

big_integer& big_integer::operator%=(const big_integer& rhs) {
    if (rhs.value.size() == 1) {
        return this->mod_to_short(rhs.value[0]);
    }
    if (value.size() < rhs.value.size()) {
        return *this;
    }
    if (*this == 0) {
        return *this = 0;
    }
    return *this = division(*this, rhs).second;
}

big_integer& big_integer::div_to_short(uint32_t rhs) {
    if (*this == 0) {
        return *this;
    }
    uint32_t carry = 0;
    for (size_t i = value.size(); i > 0; --i) {
        uint64_t temp =
                (static_cast<uint64_t>(carry) << std::numeric_limits<uint32_t>::digits) + static_cast<uint64_t>(value[i - 1]);
        value[i - 1] = static_cast<uint32_t>(temp / rhs);
        carry = static_cast<uint32_t>(temp % rhs);
    }
    skip_leading_zeros();
    if (value.size() == 1 && value[0] == 0) {
        is_negative = false;
    }
    return *this;
}

big_integer& big_integer::mod_to_short(uint32_t rhs) {
    if (*this == 0) {
        return *this;
    }
    uint32_t carry = 0;
    for (size_t i = value.size(); i > 0; --i) {
        uint64_t temp =
                (static_cast<uint64_t>(carry) << std::numeric_limits<uint32_t>::digits) + static_cast<uint64_t>(value[i - 1]);
        carry = static_cast<uint32_t>(temp % rhs);
    }
    std::vector<uint32_t>(1, carry).swap(value);
    if (carry == 0) {
        is_negative = false;
    }
    return *this;
}

void big_integer::commutative_bitwise_operation(const big_integer& rhs,
                                                const std::function<uint32_t(uint32_t a, uint32_t b)> binary_function) {
assert(rhs != 0);
assert(*this != 0);

if (is_negative) {
++*this;
for (uint32_t& digit : value) {
digit = ~digit;
}
}
if (rhs.value.size() > value.size()) {
value.resize(rhs.value.size(), (is_negative ? UINT32_MAX : 0));
}

if (rhs.is_negative) {
size_t left_border = 0;
while (left_border < rhs.value.size() && rhs.value[left_border] == 0) {
++left_border;
}
for (size_t i = 0; i < left_border; ++i) {
value[i] = binary_function(value[i], 0);
}
if (left_border < rhs.value.size()) {
value[left_border] = binary_function(value[left_border], ~(rhs.value[left_border] - 1));
}
for (size_t i = left_border + 1; i < value.size(); ++i) {
if (i < rhs.value.size()) {
value[i] = binary_function(value[i], ~rhs.value[i]);
} else {
value[i] = binary_function(value[i], UINT32_MAX);
}
}
} else {
for (size_t i = 0; i < value.size(); ++i) {
if (i < rhs.value.size()) {
value[i] = binary_function(value[i], rhs.value[i]);
} else {
value[i] = binary_function(value[i], 0);
}
}
}

is_negative = binary_function(is_negative, rhs.is_negative);
if (is_negative) {
for (uint32_t& digit : value) {
digit = ~digit;
}
--*this;
}
skip_leading_zeros();
}

big_integer& big_integer::operator&=(const big_integer& rhs) {
    if (rhs == 0 || *this == 0) {
        return *this = 0;
    }
    commutative_bitwise_operation(rhs, std::bit_and<>());
    return *this;
}

big_integer& big_integer::operator|=(const big_integer& rhs) {
    if (rhs == 0) {
        return *this;
    }
    if (*this == 0) {
        return *this = rhs;
    }
    commutative_bitwise_operation(rhs, std::bit_or<>());
    return *this;
}

big_integer& big_integer::operator^=(const big_integer& rhs) {
    if (rhs == 0) {
        return *this;
    }
    if (*this == 0) {
        return *this = rhs;
    }
    commutative_bitwise_operation(rhs, std::bit_xor<>());
    return *this;
}

big_integer& big_integer::operator<<=(int rhs) {
    if (rhs == 0 || *this == 0) {
        return *this;
    }
    uint32_t digits_shift = rhs / std::numeric_limits<uint32_t>::digits;
    uint32_t shift = rhs % std::numeric_limits<uint32_t>::digits;
    if (digits_shift > 0) {
        value.insert(value.begin(), digits_shift, 0);
    }
    if (shift > 0) {
        uint64_t carry = 0;
        for (size_t i = digits_shift; i < value.size(); ++i) {
            uint64_t temp = (static_cast<uint64_t>(value[i]) << shift) + carry;
            value[i] = temp & UINT32_MAX;
            carry = temp >> std::numeric_limits<uint32_t>::digits;
        }
        if (carry > 0) {
            value.push_back(carry);
        }
    }
    return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
    if (rhs == 0 || *this == 0) {
        return *this;
    }
    uint32_t digits_shift = rhs / std::numeric_limits<uint32_t>::digits;
    uint32_t shift = rhs % std::numeric_limits<uint32_t>::digits;
    if (digits_shift > 0) {
        if (digits_shift >= value.size()) {
            return *this = 0;
        }
        value.erase(value.begin(), value.begin() + digits_shift);
    }
    if (shift > 0) {
        uint32_t carry = 0;
        for (size_t i = value.size(); i > 0; --i) {
            uint32_t new_carry = value[i - 1] << (std::numeric_limits<uint32_t>::digits - shift);
            value[i - 1] = (value[i - 1] >> shift) + carry;
            carry = new_carry;
        }
    }
    skip_leading_zeros();
    if (is_negative) {
        --*this;
    }
    return *this;
}

big_integer big_integer::operator+() const {
    return *this;
}

big_integer big_integer::operator-() const {
    big_integer result = *this;
    if (*this != 0) {
        result.is_negative = !result.is_negative;
    }
    return result;
}

big_integer big_integer::operator~() const {
    big_integer result = *this;
    result.add_to_short(1);
    if (!(result == 0)) {
        result.is_negative = !result.is_negative;
    }
    return result;
}

big_integer& big_integer::operator++() {
    return add_to_short(1);
}

big_integer big_integer::operator++(int) {
    big_integer result = *this;
    ++*this;
    return result;
}

big_integer& big_integer::operator--() {
    return sub_to_short(1);
}

big_integer big_integer::operator--(int) {
    big_integer result = *this;
    --*this;
    return result;
}

big_integer operator+(const big_integer& a, const big_integer& b) {
    return big_integer(a) += b;
}

big_integer operator-(const big_integer& a, const big_integer& b) {
    return big_integer(a) -= b;
}

big_integer operator*(const big_integer& a, const big_integer& b) {
    return big_integer(a) *= b;
}

big_integer operator/(const big_integer& a, const big_integer& b) {
    return big_integer(a) /= b;
}

big_integer operator%(const big_integer& a, const big_integer& b) {
    return big_integer(a) %= b;
}

big_integer operator&(const big_integer& a, const big_integer& b) {
    return big_integer(a) &= b;
}

big_integer operator|(const big_integer& a, const big_integer& b) {
    return big_integer(a) |= b;
}

big_integer operator^(const big_integer& a, const big_integer& b) {
    return big_integer(a) ^= b;
}

big_integer operator<<(const big_integer& a, int b) {
    return big_integer(a) <<= b;
}

big_integer operator>>(const big_integer& a, int b) {
    return big_integer(a) >>= b;
}

bool operator==(const big_integer& a, const big_integer& b) {
    if (a == 0) {
        return b == 0;
    }
    if (a.is_negative != b.is_negative || a.value.size() != b.value.size()) {
        return false;
    }
    for (size_t i = 0; i < a.value.size(); ++i) {
        if (a.value[i] != b.value[i]) {
            return false;
        }
    }
    return true;
}

bool operator==(const big_integer& a, const int& b) {
    if (a.value.empty()) {
        return b == 0;
    }
    return a.value.size() == 1 && (b < 0) == a.is_negative &&
           static_cast<uint32_t>(std::abs(static_cast<int64_t>(b))) == a.value[0];
}

bool operator!=(const big_integer& a, const big_integer& b) {
    return !(a == b);
}

bool operator<(const big_integer& a, const big_integer& b) {
    if (a.is_negative != b.is_negative) {
        return a.is_negative;
    }
    if (a.value.size() != b.value.size()) {
        return (a.value.size() < b.value.size()) != a.is_negative;
    }
    for (size_t i = a.value.size(); i > 0; --i) {
        if (a.value[i - 1] != b.value[i - 1]) {
            return (a.value[i - 1] < b.value[i - 1]) != a.is_negative;
        }
    }
    return false;
}

bool operator>(const big_integer& a, const big_integer& b) {
    return b < a;
}

bool operator<=(const big_integer& a, const big_integer& b) {
    return !(a > b);
}

bool operator>=(const big_integer& a, const big_integer& b) {
    return !(a < b);
}

std::string to_string(const big_integer& a) {
    if (a == 0) {
        return "0";
    }
    big_integer cur = a;
    cur.is_negative = false;

    std::string result;
    while (cur > 0) {
        std::string temp = std::to_string((big_integer(cur).mod_to_short(big_integer::STRING_RADIX)).value[0]);
        std::reverse(temp.begin(), temp.end());
        temp.resize(std::numeric_limits<uint32_t>::digits10, '0');
        result += temp;
        cur.div_to_short(big_integer::STRING_RADIX);
    }

    while (result.size() > 1 && result.back() == '0') {
        result.pop_back();
    }
    if (a.is_negative) {
        result += '-';
    }
    std::reverse(result.begin(), result.end());
    return result;
}

std::ostream& operator<<(std::ostream& out, const big_integer& a) {
    return out << to_string(a);
}