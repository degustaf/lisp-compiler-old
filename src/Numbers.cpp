#include <algorithm>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cassert>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstring>

class Number {
  public:
    // virtual operator char() const = 0;
    // virtual operator int() const = 0;
    // virtual operator short() const = 0;
    virtual operator long() = 0;
    // virtual operator float() const = 0;
    virtual operator double() const = 0;
};

class BigDecimal;

#define BASE (1 << CHAR_BIT)
class BigInt : public Number {
  public:
    BigInt(long x);
    BigInt() = default;
    virtual operator long();
    virtual operator double() const;
    virtual operator std::string() const;

    bool operator==(const BigInt &y) const {return (sign == y.sign) && cmp(y);};
    BigInt operator+(const BigInt &y) const;
    BigInt operator-() const;
    BigInt operator-(const BigInt &y) const;
    BigInt operator*(const BigInt &y) const;
    BigInt operator*(const long &y) const;
    BigInt operator/(const BigInt &y) const;
    BigInt operator%(const BigInt &y) const;
    BigInt operator>>(unsigned long y) const;
    BigInt operator>>(int y) const;
    bool operator<(const BigInt &y) const;
    bool operator>(const BigInt &y) const;
    BigInt abs() const;

    long signum(void) const;
    BigInt gcd(const BigInt &d) const;
    bool isZero(void) const;
    bool isNull(void) const {return array.size() == 0;};
    bool isOdd(void) const;
    BigInt pow(BigInt y) const;
    BigInt divide(const BigInt &y, BigInt *q) const;
    long divide(long y, BigInt *q) const;
    BigDecimal toBigDecimal(int sign, int scale) const;

    // These should be private with Friend BigDecimal::stripZerosToMatchScale
    int cmp(const BigInt &y) const;
    unsigned char &operator[] (size_t n) {return array[n];};
    const unsigned char &operator[] (size_t n) const {return array[n];};

    static const BigInt ZERO;
    static const BigInt ONE;
    static const BigInt TEN;
  private:
    BigInt(size_t n, bool) : array(n), sign(1), ndigits(1) {};
    bool isOne(void) const;
    bool isPos(void) const;
    static size_t maxDigits(const BigInt &x, const BigInt &y);
    void add(const BigInt &x, const BigInt &y);
    int add_loop(size_t n, const BigInt &x, int carry);
    int add_loop(size_t n, const BigInt &x, const BigInt &y, int carry);
    void sub(const BigInt &x, const BigInt &y);
    int sub_loop(size_t n, const BigInt &x, int borrow);
    int sub_loop(size_t n, const BigInt &x, const BigInt &y, int borrow);
    void div(const BigInt &y, BigInt *q, BigInt *r) const;
    int quotient(const BigInt &x, int y);
    size_t length() const;
    void normalize(void);
    
    int sign;
    size_t ndigits;
    std::vector<unsigned char> array;
};

BigInt::BigInt(long x) : array(sizeof(long)), sign(x<0 ? -1 : 1) {
  unsigned long u = (x == LONG_MIN) ? (LONG_MAX + 1UL) : (x<0 ? -x : x);
  size_t i = 0;
  do {
    array[i++] = u % BASE;
  } while((u /= BASE) > 0 && i < array.size());
  ndigits = i == 0 ? 1 : i;
  for(; i<array.size(); i++)
    array[i] = 0;
};

BigInt::operator long() {
  if(array.size() == 0)
    return 0;
  long ret = sign;
  for(auto it = array.rbegin(); it!=array.rend(); it++) {
    ret *= BASE;
    ret += *it;
  }
  return ret;
}

BigInt::operator double() const {
  if(array.size() == 0)
    return 0.0;
  double ret = sign;
  for(auto it = array.rbegin(); it!=array.rend(); it++) {
    ret *= BASE;
    ret += *it;
  }
  return ret;
}

BigInt::operator std::string() const {
  std::stringstream ss;
  BigInt q = *this;
  size_t n = ndigits;
  do {
    int r = q.quotient(q, 10);
    ss << "0123456789"[r];
    while (n > 1 && q[n-1] == 0)
			n--;
  } while(n > 1 && q[0] != 0);
  if(sign == -1)
    ss << "-";
  std::string s = ss.str();
  reverse(s.begin(), s.end());
  return s;
}

BigInt BigInt::operator+(const BigInt &y) const {
  if(sign ^ y.sign == 0) {
    BigInt ret(maxDigits(*this, y)+1);
    ret.add(*this, y);
    ret.sign == ret.isZero() ? 1 : sign;
    return ret;
  }
  if(cmp(y) > 0) {
    BigInt ret(ndigits);
    ret.sub(*this, y);
    ret.sign == ret.isZero() ? 1 : sign;
    return ret;
  }
  BigInt ret(y.ndigits);
  ret.sub(y, *this);
  ret.sign == ret.isZero() ? 1 : -sign;
  return ret;
}

BigInt BigInt::operator-() const {
  BigInt ret = *this;
  ret.sign = isZero() ? 1 : -sign;
  return ret;
}

BigInt BigInt::operator-(const BigInt &y) const {
  return *this + -y;
}

BigInt BigInt::operator*(const BigInt &y) const {
  BigInt ret(ndigits + y.ndigits);
  for(size_t i=0; i<ndigits; i++) {
    unsigned int carry = 0;
    for(size_t j=0; j<y.ndigits; j++) {
      carry += array[i] * y.array[j] + ret.array[i+j];
      ret.array[i+j] = carry % BASE;
      carry /= BASE;
    }
    for(size_t j=y.ndigits; j < ndigits + y.ndigits - i; j++) {
      carry += ret.array[i+j];
      ret.array[i+j] = carry % BASE;
      carry /= BASE;
    }
  }
  ret.normalize();
  ret.sign = (ret.isZero() || (sign ^ y.sign == 0)) ? 1 : -1;
  return ret;
}

BigInt BigInt::operator*(const long &y) const {
  return *this * BigInt(y);
}

BigInt BigInt::operator/(const BigInt &y) const {
  BigInt q, r;
  div(y, &q, &r);
  if((sign ^ y.sign != 0) && !r.isZero()) {
    int carry = q.add_loop(q.array.size(), q, 1);
    assert(carry == 0);
    q.normalize();
  }
  return q;
}

BigInt BigInt::operator%(const BigInt &y) const {
  BigInt q, r;
  div(y, &q, &r);
  if((sign ^ y.sign != 0) && !r.isZero()) {
    int borrow = r.sub_loop(r.array.size(), r, y, 0);
    assert(borrow == 0);
    r.normalize();
  }
  return r;
}

BigInt BigInt::operator>>(unsigned long y) const {
  if(y > ndigits)
    return isPos() ? ZERO : -ONE;
  BigInt ret((ndigits - y + CHAR_BIT - 1) / CHAR_BIT);
  size_t offset = y / CHAR_BIT;
  size_t shift = y % CHAR_BIT;
  for(size_t i = 0; i+offset < array.size(); i++)
    ret[i] = array[i + offset] >> shift;
  ret.sign = sign;
  ret.normalize();
  return ret;
}

BigInt BigInt::operator>>(int y) const {
  return *this >> ((unsigned long) y);
}

bool BigInt::operator<(const BigInt &y) const {
  return (y - *this).isPos();
}

bool BigInt::operator>(const BigInt &y) const {
  return (*this - y).isPos();
}

BigInt BigInt::abs() const {
  BigInt ret = *this;
  ret.sign = 1;
  return ret;
}

long BigInt::signum(void) const {
  if(isZero())
    return 0;
  return sign;
}

BigInt BigInt::gcd(const BigInt &d) const {
  BigInt a = *this;
  BigInt b = d;
  while(!b.isZero()) {
    BigInt temp = b;
    b = a % b;
    a = temp;
  }
  return a;
}

const BigInt BigInt::ZERO((long)0);
const BigInt BigInt::ONE((long)1);
const BigInt BigInt::TEN((long)10);

bool BigInt::isZero(void) const {
  return ndigits == 1 && array[0] == 0;
}

BigInt BigInt::pow(BigInt y) const {
  if(isZero())
    return ZERO;
  if(y.isZero())
    return ONE;
  if(isOne())
    return BigInt((long)((y[0] & 1) ? sign : 1));
  BigInt z = *this;
  BigInt u((long)1);
  while(ONE < y) {
    if(y.isOdd())
      u = u * z;
      z = z * z;
      y = y >> 1;
  }
  return z;
}

BigInt BigInt::divide(const BigInt &y, BigInt *q) const {
  BigInt ret;
  div(y, q, &ret);
  return ret;
}

long BigInt::divide(long y, BigInt *q) const {
  if(y == 0)
    throw std::domain_error("Divide by zero error.");
  if(y < 0) {
    q->sign = -1;
    y = -y;
  }
  q->sign *= sign;
  int i = array.size() - 1;
  long ret = 0;
  while(ret / y == 0 && i >= 0) {
    ret = ret * BASE + array[i];
    i--;
  }
  if(i < 0)
    return ret;
  for(q->array = std::vector<unsigned char>(i+1); i >= 0; i--) {
    q->array[i] = ret / y;
    ret = (ret % y) * BASE + array[i];
  }
  q->normalize();
  return ret;
}

bool BigInt::isOne(void) const {
  return ndigits == 1 && array[0] == 1;
}

bool BigInt::isOdd(void) const {
  return array[0] & 1;
}

bool BigInt::isPos(void) const {
  return sign > 0;
}

size_t BigInt::maxDigits(const BigInt &x, const BigInt &y) {
  return (x.ndigits > y.ndigits) ? x.ndigits : y.ndigits;
}

void BigInt::add(const BigInt &x, const BigInt &y) {
  size_t n = y.ndigits;
  if(x.ndigits < n)
    return add(y, x);
  if(x.ndigits > n) {
    int carry = add_loop(n, x, y, 0);
    array[array.size()-1] = add_loop(n, x, carry);
  } else {
    array[n] = add_loop(n, x, y, 0);
  }
  normalize();
}

int BigInt::add_loop(size_t n, const BigInt &x, int carry) {
  for(; n<x.ndigits; n++) {
    carry += x.array[n];
    array[n] = carry % BASE;
    carry /= BASE;
  }
  return carry;
}

int BigInt::add_loop(size_t n, const BigInt &x, const BigInt &y, int carry) {
  for(size_t i=0; i<n; i++) {
    carry += x.array[i] + y.array[i];
    array[i] = carry % BASE;
    carry /= BASE;
  }
  return carry;
}

void BigInt::sub(const BigInt &x, const BigInt &y) {
  size_t n = y.ndigits;
  int borrow = sub_loop(n, x, y, 0);
  if(x.ndigits > n)
    borrow = sub_loop(n, x, borrow);
  assert(borrow == 0);
  normalize();
}

int BigInt::sub_loop(size_t n, const BigInt &x, int borrow) {
  for(; n<x.ndigits; n++) {
    int d = x.array[n] + BASE - borrow;
    array[n] = d % BASE;
    borrow = 1 - d / BASE;
  }
  return borrow;
}

int BigInt::sub_loop(size_t n, const BigInt &x, const BigInt &y, int borrow) {
  for(size_t i=0; i<n; i++) {
    int d = x.array[i] + BASE - borrow - y.array[i];
    array[i] = d % BASE;
    borrow = 1 - d / BASE;
  }
  return borrow;
}

static int product(size_t n, unsigned char *z, const std::vector<unsigned char> &x, int y) {
  unsigned int carry = 0;
  for(size_t i=0; i<n; i++) {
    carry += x[i] * y;
    z[i] = carry % BASE;
    carry /= BASE;
  }
  return carry;
}

void BigInt::div(const BigInt &y, BigInt *q, BigInt *r) const {
  *q = BigInt(ndigits, true);
  *r = BigInt(y.ndigits, true);
  unsigned char tmp[ndigits + y.ndigits + 2];
  size_t n = length();
  size_t m = y.length();
  if(m == 1) {
    if(y[0] == 0)
      throw std::domain_error("Divide by zero error.");
    r->array[0] = q->quotient(*this, y.array[0]);
    for(size_t i=1; i<r->array.size(); i++)
      r->array[i] = 0;
    
  } else if(m > n) {
    q->array.assign(q->array.size(), 0);
    r->array = array;
  } else {
    unsigned char *rem = tmp;
    unsigned char *dq = tmp + n + 1;
    assert(2 <= m);
    assert(m <= n);
    for(size_t i=0; i<n; i++)
      rem[i] = array[i];
    rem[n] = 0;
    for(int k=n-m; k>=0; k--) {
			assert(2 <= m);
			assert(m <= k+m);
			assert(k+m <= n);
			int km = k + m;
			unsigned long y2 = y[m-1] * BASE + y[m-2];
			unsigned long r3 = (rem[km] * BASE + rem[km-1]) * BASE + rem[km-2];
			int q_k = r3/y2;
			if (q_k >= BASE)
				q_k = BASE - 1;
			dq[m] = product(m, dq, y.array, q_k);
			int i;
			for (i=m; i>0; i--)
				if (rem[i+k] != dq[i])
					break;
			if (rem[i+k] < dq[i])
				dq[m] = product(m, dq, y.array, --q_k);
			q->array[k] = q_k;
			assert(0 <= k);
			assert (k <= k+m);
			// int borrow = XP_sub(m + 1, &rem[k], &rem[k], dq, 0);
			// int XP_sub(int n, T z, T x, T y, int borrow) 
			int borrow = 0;
			for (i = 0; i < n; i++) {
    		int d = rem[k+i] + BASE - borrow - dq[i];
    		rem[k+i] = d%BASE;
    		borrow = 1 - d/BASE;
    	}
			assert(borrow == 0);
    }
    for(size_t i=0; i<m; i++)
      r->array[i] = rem[i];
    for (size_t i=n-m+1; i<q->array.size(); i++)
			q->array[i] = 0;
		for (size_t i=m; i<r->array.size(); i++)
			r->array[i] = 0;
  }

  q->normalize();
  r->normalize();
  q->sign = (q->isZero() || (sign ^ y.sign == 0)) ? 1 : -1;
}

int BigInt::quotient(const BigInt &x, int y) {
  unsigned int carry = 0;
  for(int i=x.array.size()-1; i>=0; i--) {
    carry = carry * BASE + x[i];
    array[i] = carry / y;
    carry %= y;
  }
  return carry;
}

size_t BigInt::length() const {
  size_t n = array.size();
  while(n>1 && array[n-1] == 0)
    n--;
  return n;  
}

void BigInt::normalize(void) {
  ndigits = length();
}

int BigInt::cmp(const BigInt &y) const {
  if(ndigits == y.ndigits) {
    size_t i = ndigits - 1;
    while(i>0 && array[i]==y.array[i])
      i--;
    return array[i]==y.array[i];
  }
  return ndigits - y.ndigits;
}


typedef enum {
  ROUND_UP,
  ROUND_DOWN,
  ROUND_CEILING,
  ROUND_FLOOR,
  ROUND_HALF_UP,
  ROUND_HALF_DOWN,
  ROUND_HALF_EVEN,
  ROUND_UNNECESSARY,
} roundingMode;

// BigDecimal
static const long LONG_TEN_POWERS_TABLE[] = {
  1,                     // 0 / 10^0
  10,                    // 1 / 10^1
  100,                   // 2 / 10^2
  1000,                  // 3 / 10^3
  10000,                 // 4 / 10^4
  100000,                // 5 / 10^5
  1000000,               // 6 / 10^6
  10000000,              // 7 / 10^7
  100000000,             // 8 / 10^8
  1000000000,            // 9 / 10^9
  10000000000L,          // 10 / 10^10
  100000000000L,         // 11 / 10^11
  1000000000000L,        // 12 / 10^12
  10000000000000L,       // 13 / 10^13
  100000000000000L,      // 14 / 10^14
  1000000000000000L,     // 15 / 10^15
  10000000000000000L,    // 16 / 10^16
  100000000000000000L,   // 17 / 10^17
  1000000000000000000L   // 18 / 10^18
};
static const long THRESHOLDS_TABLE[] = {
  LONG_MAX,                     // 0
  LONG_MAX/10L,                 // 1
  LONG_MAX/100L,                // 2
  LONG_MAX/1000L,               // 3
  LONG_MAX/10000L,              // 4
  LONG_MAX/100000L,             // 5
  LONG_MAX/1000000L,            // 6
  LONG_MAX/10000000L,           // 7
  LONG_MAX/100000000L,          // 8
  LONG_MAX/1000000000L,         // 9
  LONG_MAX/10000000000L,        // 10
  LONG_MAX/100000000000L,       // 11
  LONG_MAX/1000000000000L,      // 12
  LONG_MAX/10000000000000L,     // 13
  LONG_MAX/100000000000000L,    // 14
  LONG_MAX/1000000000000000L,   // 15
  LONG_MAX/10000000000000000L,  // 16
  LONG_MAX/100000000000000000L, // 17
  LONG_MAX/1000000000000000000L // 18
};
static const size_t THRESHOLDS_TABLE_COUNT = sizeof(THRESHOLDS_TABLE)/sizeof(THRESHOLDS_TABLE[0]);

class BigDecimal : public Number {
  public:
    BigDecimal(double x);

    static BigDecimal valueOf(long val);
    static BigDecimal valueOf(long unscaledVal, int scale);

    virtual operator long();
    virtual operator double() const;
    virtual operator std::string() const;
    BigInt toBigInt();

    int signum() const;

    static const BigDecimal ONE;
  private:
    BigDecimal(BigInt intVal, long val, int scale, int prec) :
      intVal(intVal), intCompact(val), scale(scale), precision(prec) { };

    BigDecimal() = default;

    BigDecimal setScale(int newscale, roundingMode rm);
    BigInt inflate();
    int checkScale(long val) const;
    BigInt bigMultiplyPowerTen(int n);
    BigDecimal stripZerosToMatchScale(long preferredScale);

    BigInt intVal;
    long intCompact;
    int scale;
    int precision;

    static long longMultiplyPowerTen(long val, int n);
    static BigInt bigTenToThe(int n);
    static BigDecimal divideAndRound(long ldividend, const BigInt bdividend, long ldivisor, const BigInt bdivisor,
            int scale, roundingMode rm, int preferredScale);
    static int longCompareMagnitude(long x, long y);
    static long compactValFor(BigInt b);
    static const long INFLATED = LONG_MIN;
    
    friend BigDecimal BigInt::toBigDecimal(int sign, int scale) const;
};

BigDecimal BigInt::toBigDecimal(int sign, int scale) const {
  if(isZero())
    return BigDecimal::valueOf(0, scale);
  if(ndigits > sizeof(long) || (ndigits == sizeof(long) && (char) array[sizeof(long) - 1] < 0))
    return BigDecimal(*this, BigDecimal::INFLATED, scale, 0);
  return BigDecimal(BigInt(), (long)*this, scale, 0);
}

#define SIGN_BIT (((uint64_t) 1) << (CHAR_BIT * sizeof(double) - 1))
#define EXPONENT_SHIFT (DBL_MANT_DIG - 1)
#define EXPONENT_MASK ((1 << (CHAR_BIT * sizeof(double) - DBL_MANT_DIG)) - 1)
#define EXPONENT(x) ((x) >> EXPONENT_SHIFT) & EXPONENT_MASK
#define EXPONENT_BIAS 1023    // This should be deducible from cfloat, but I'm not sure how.
#define FRACTION_MASK ((1L << EXPONENT_SHIFT) - 1)
#define IMPLICIT_ONE (1L << EXPONENT_SHIFT)
BigDecimal::BigDecimal(double val) {
  if(!std::isfinite(val))
    throw std::runtime_error("Infinite or Not a number");
  static_assert(sizeof(double) == 8);
  static_assert(sizeof(long) == 8);
  uint64_t valBits;
  memcpy(&valBits, &val, sizeof(val));
  int sign = valBits & SIGN_BIT ? -1 : 1;
  int exponent = (int) EXPONENT(valBits);
  long significand = exponent==0 ? (valBits & FRACTION_MASK) << 1
                                 : (valBits & FRACTION_MASK) | IMPLICIT_ONE;
  exponent -= EXPONENT_BIAS + EXPONENT_SHIFT;
  // At this point, val == sign * significand * 2**exponent.

  if(significand == 0) {
    intVal = BigInt::ZERO;
    intCompact = 0;
    precision = 1;
    return;
  }
  
  // Normalize
  while((significand & 1) == 0) {
    significand >>= 1;
    exponent++;
  }
  
  long s = sign * significand;
  BigInt b;
  if (exponent < 0) {
    b = BigInt(2).pow(-exponent) * s;
    scale = -exponent;
  } else if (exponent > 0) {
    b = BigInt(2).pow(exponent) * s;
  } else {
    b = BigInt(s);
  }
  intCompact = compactValFor(b);
  intVal = (intCompact != INFLATED) ? BigInt() : b;
}

BigDecimal BigDecimal::valueOf(long val) {
  if(val != INFLATED)
    return BigDecimal(BigInt(), val, 0, 0);
  return BigDecimal(BigInt(val), val, 0, 0);
}

BigDecimal BigDecimal::valueOf(long unscaledVal, int scale) {
  if(scale == 0)
    return valueOf(unscaledVal);
  if(unscaledVal == 0)
    return BigDecimal(BigInt::ZERO, 0, scale, 1);
  return BigDecimal(unscaledVal == INFLATED ? BigInt(unscaledVal) : BigInt(), unscaledVal, scale, 0);
}

BigDecimal::operator long() {
  return (intCompact != INFLATED && scale == 0) ?
    intCompact : (long)toBigInt();
}

BigInt BigDecimal::toBigInt() {
  return setScale(0, ROUND_UNNECESSARY).inflate();
}

BigDecimal::operator double() const {
  if (scale == 0 && intCompact != INFLATED)
    return (double)intCompact;
  return std::stod((std::string)*this);
}

BigDecimal::operator std::string() const {
  if (scale == 0)                      // zero scale is trivial
    return (intCompact != INFLATED) ? std::to_string(intCompact) : (std::string)intVal;
  const char* coeff;
  int offset = 0;
  // Get the significand as an absolute value
  if (intCompact != INFLATED) {
    std::stringstream ss;
    ss << intCompact;
    coeff  = ss.str().c_str();
  } else {
    offset = 0;
    coeff  = ((std::string)intVal.abs()).c_str();
  }
  // TODO
}

int BigDecimal::signum() const {
  if(intCompact != INFLATED) {
    return intCompact == 0 ? 0 : intCompact > 0 ? 1 : -1;
  }
  return intVal.signum();
}

const BigDecimal BigDecimal::ONE = BigDecimal(BigInt::ONE, 1, 0, 1);

BigDecimal BigDecimal::setScale(int newscale, roundingMode rm) {
  int oldscale = scale;
  if(newscale == oldscale)
    return *this;
  if(signum() == 0)
    return valueOf(0, newscale);
  long rs = intCompact;
  if(newscale > oldscale) {
    int raise = checkScale((long)newscale - oldscale);
    BigInt rb;
    if(rs == INFLATED || (rs = longMultiplyPowerTen(rs, raise)) == INFLATED)
      rb = bigMultiplyPowerTen(raise);
    return BigDecimal(rb, rs, newscale, (precision > 0) ? precision + raise : 0);
  }
  int drop = checkScale((long)oldscale - newscale);
  if(drop < THRESHOLDS_TABLE_COUNT)
    return divideAndRound(rs, intVal, LONG_TEN_POWERS_TABLE[drop], BigInt(), newscale, rm, newscale);
  return divideAndRound(rs, intVal, INFLATED, bigTenToThe(drop), newscale, rm, newscale);
}

BigInt BigDecimal::inflate() {
  if(intVal.isNull())
    intVal = BigInt(intCompact);
  return intVal;
}

int BigDecimal::checkScale(long val) const {
  int asInt = (int)val;
    if(asInt != val) {
      asInt = val>INT_MAX ? INT_MAX : INT_MIN;
      BigInt b;
      if(intCompact && ((b = intVal).isNull() || b.signum()))
        if(asInt>0)
          throw std::underflow_error("");
        throw std::overflow_error("");
    }
    return asInt;
}

BigInt BigDecimal::bigMultiplyPowerTen(int n) {
  if(n <= 0)
    return inflate();
  if(intCompact != INFLATED)
    return bigTenToThe(n) * intCompact;
  return intVal * bigTenToThe(n);
}

BigDecimal BigDecimal::stripZerosToMatchScale(long preferredScale) {
  inflate();
  BigInt q;
  long r;
  while(intVal.cmp(BigInt::TEN) >= 0 && scale > preferredScale) {
    if(intVal[0] & 1)
      break;
    r = intVal.divide(10, &q);
    if(r != 0)
      break;
    intVal = q;
    scale = checkScale((long)scale - 1);
    if(precision > 0)
      precision--;
  }
  if(!intVal.isNull())
    intCompact = compactValFor(intVal);
  return *this;
}

long BigDecimal::longMultiplyPowerTen(long val, int n) {
  if (val == 0 || n <= 0)
    return val;
  if (n < THRESHOLDS_TABLE_COUNT) {
    long tenpower = LONG_TEN_POWERS_TABLE[n];
    if (val == 1)
      return tenpower;
    if (abs(val) <= THRESHOLDS_TABLE[n])
      return val * tenpower;
  }
  return INFLATED;
}

BigInt BigDecimal::bigTenToThe(int n) {
  return BigInt(10).pow(BigInt(n));
}

BigDecimal BigDecimal::divideAndRound(long ldividend, const BigInt bdividend, long ldivisor, const BigInt bdivisor,
            int scale, roundingMode rm, int preferredScale) {
  bool isRemainderZero;   // record remainder is zero or not
  int qsign;              // quotient sign
  long q, r;              // store quotient & remainder in long
  BigInt mq;              // store quotient
  BigInt mr;              // store remainder
  BigInt mdivisor;
  BigInt mdividend;
  bool isLongDivision = (ldividend != INFLATED && ldivisor != INFLATED);
  if(isLongDivision) {
    q = ldividend / ldivisor;
    if(rm == ROUND_DOWN && scale == preferredScale)
      return BigDecimal(BigInt(), q, scale, 0);
    r = ldividend % ldivisor;
    isRemainderZero = (r == 0);
    qsign = ((ldividend < 0) == (ldivisor < 0)) ? 1 : -1;
  } else {
    mdividend = bdividend.isNull() ? BigInt(ldividend) : bdividend;
    if (ldivisor != INFLATED) {
      r = mdividend.divide(ldivisor, &mq);
      isRemainderZero = (r == 0);
      qsign = (ldivisor < 0) ? -bdividend.signum() : bdividend.signum();
    } else {
      mdivisor = bdivisor;
      mr = mdividend.divide(mdivisor, &mq);
      isRemainderZero = mr.isZero();
      qsign = (bdividend.signum() != bdivisor.signum()) ? -1 : 1;
    }
 }
  bool increment = false;
  if(!isRemainderZero) {
    switch(rm) {
      case ROUND_UNNECESSARY:
        throw std::runtime_error("Rounding necessary");
      case ROUND_UP:
        increment = true;
      case ROUND_DOWN:
        increment = false;
      case ROUND_CEILING:
        increment = (qsign > 0);
      case ROUND_FLOOR:
        increment = (qsign < 0);
      default: {
        int cmpFracHalf;
        if(isLongDivision || ldivisor != INFLATED) {
          if(r <= LONG_MIN >> 1 || r >= LONG_MAX >> 1)
            cmpFracHalf = 1;
          else
            cmpFracHalf = longCompareMagnitude(2 * r, ldivisor);
        } else {
          cmpFracHalf = (mr - mdivisor >> 1).signum();
        }
        if (cmpFracHalf < 0)
          increment = false;     // We're closer to higher digit
        else if (cmpFracHalf > 0)  // We're closer to lower digit
          increment = true;
        else if (rm == ROUND_HALF_UP)
          increment = true;
        else if (rm == ROUND_HALF_DOWN)
          increment = false;
        else  // rm == ROUND_HALF_EVEN, true iff quotient is odd
          increment = isLongDivision ? (q & 1L) != 0L : mq.isOdd();
      }
    }
  }
  BigDecimal res;
  if(isLongDivision)
    res = BigDecimal(BigInt(), (increment ? q + qsign : q), scale, 0);
  else {
    if(increment)
      mq = mq + BigInt::ONE;
    res = mq.toBigDecimal(qsign, scale);
  }
  if (isRemainderZero && preferredScale != scale)
    res.stripZerosToMatchScale(preferredScale);
  return res;
}

int BigDecimal::longCompareMagnitude(long x, long y) {
  if(x < 0)
    x = -x;
  if(y < 0)
    y = -y;
  return (x < y) ? -1 : (x == y) ? 0 : 1;
}

long BigDecimal::compactValFor(BigInt b) {
  long lb = (long) b;
  if(b == BigInt(lb))
    return lb;
  return INFLATED;
}



class LongOps;
class DoubleOps;
class BigIntOps;
class Ops {
  public:
    virtual const Ops &combine(const Ops&) const = 0;
    
    virtual const Ops &opsWith(const LongOps&) const = 0;
  	virtual const Ops &opsWith(const DoubleOps&) const = 0;
  	// Ops opsWith(RatioOps x);
  	virtual const Ops &opsWith(const BigIntOps&) const = 0;
  	// Ops opsWith(BigDecimalOps x);

    virtual bool isZero(std::shared_ptr<const Number> x) const = 0;
  	virtual bool isPos(std::shared_ptr<const Number> x) const = 0;
  	virtual bool isNeg(std::shared_ptr<const Number> x) const = 0;

  	virtual std::shared_ptr<const Number> add(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const = 0;
  	virtual std::shared_ptr<const Number> addP(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const = 0;

  	virtual std::shared_ptr<const Number> multiply(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const = 0;
  	virtual std::shared_ptr<const Number> multiplyP(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const = 0;

  	virtual std::shared_ptr<const Number> divide(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const = 0;
  	virtual std::shared_ptr<const Number> quotient(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const = 0;
  	virtual std::shared_ptr<const Number> remainder(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const = 0;

  	virtual bool equiv(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const = 0;

    /*
  	virtual bool lt(Number x, Number y);
  	virtual bool lte(Number x, Number y);
  	virtual bool gte(Number x, Number y);
  	virtual Number negate(Number x);
  	virtual Number negateP(Number x);
  	virtual Number inc(Number x);
  	virtual Number incP(Number x);
  	virtual Number dec(Number x);
  	virtual Number decP(Number x);
  	*/
};

class OpsP : public Ops {
  	virtual std::shared_ptr<const Number> addP(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  	  return add(x, y);
  	};
  	virtual std::shared_ptr<const Number> multiplyP(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  	  return multiply(x, y);
  	};
};

class LongOps : public Ops {
  private:
    virtual const Ops &combine(const Ops&) const;
    virtual const Ops &opsWith(const LongOps&) const;
  	virtual const Ops &opsWith(const DoubleOps&) const;
  	virtual const Ops &opsWith(const BigIntOps&) const;

    virtual bool isZero(std::shared_ptr<const Number> x) const;
  	virtual bool isPos(std::shared_ptr<const Number> x) const;
  	virtual bool isNeg(std::shared_ptr<const Number> x) const;

  	virtual std::shared_ptr<const Number> add(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;
  	virtual std::shared_ptr<const Number> addP(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;

  	virtual std::shared_ptr<const Number> multiply(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;
  	virtual std::shared_ptr<const Number> multiplyP(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;

  	virtual std::shared_ptr<const Number> divide(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;
  	virtual std::shared_ptr<const Number> quotient(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;
  	virtual std::shared_ptr<const Number> remainder(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;
  	
  	virtual bool equiv(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;
};

class DoubleOps : public OpsP {
  private:
    virtual const Ops &combine(const Ops&) const;
    virtual const Ops &opsWith(const LongOps&) const;
  	virtual const Ops &opsWith(const DoubleOps&) const;
  	virtual const Ops &opsWith(const BigIntOps&) const;

    virtual bool isZero(std::shared_ptr<const Number> x) const;
  	virtual bool isPos(std::shared_ptr<const Number> x) const;
  	virtual bool isNeg(std::shared_ptr<const Number> x) const;

  	virtual std::shared_ptr<const Number> add(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;

  	virtual std::shared_ptr<const Number> multiply(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;

  	virtual std::shared_ptr<const Number> divide(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;
  	virtual std::shared_ptr<const Number> quotient(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;
  	virtual std::shared_ptr<const Number> remainder(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;

  	virtual bool equiv(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;
};

class BigIntOps : public OpsP {
  public:
    virtual const Ops &combine(const Ops&) const;
    virtual const Ops &opsWith(const LongOps&) const;
  	virtual const Ops &opsWith(const DoubleOps&) const;
  	virtual const Ops &opsWith(const BigIntOps&) const;

    virtual bool isZero(std::shared_ptr<const Number> x) const;
  	virtual bool isPos(std::shared_ptr<const Number> x) const;
  	virtual bool isNeg(std::shared_ptr<const Number> x) const;

  	virtual std::shared_ptr<const Number> add(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;

  	virtual std::shared_ptr<const Number> multiply(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;

  	virtual std::shared_ptr<const Number> divide(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;
  	virtual std::shared_ptr<const Number> quotient(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;
  	virtual std::shared_ptr<const Number> remainder(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;

  	virtual bool equiv(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const;
};

static const LongOps LONG_OPS = LongOps();
static const DoubleOps DOUBLE_OPS = DoubleOps();
static const BigIntOps BIGINT_OPS = BigIntOps();


// Helper Functions
std::shared_ptr<const Number> num(long) {
  // TODO   requires Integer
}

std::shared_ptr<const Number> num(double) {
  // TODO   requires float
}

long add(long x, long y) {
  long ret = x + y;
  if((ret ^ x) < 0 && (ret ^ y) < 0)
    throw std::overflow_error("Integer Overflow");
  return ret;
}
long multiply(long x, long y) {
  if(x == LONG_MIN && y < 0)
    throw std::overflow_error("Integer Overflow");
  long ret = x * y;
  if(y != 0 && ret/y != x)
    throw std::overflow_error("Integer Overflow");
  return ret;
}

std::shared_ptr<const Number> divide(BigInt n, BigInt d) {
  if(d.isZero())
    throw  std::domain_error("Divide by zero error.");
  BigInt gcd = n.gcd(d);
  if(gcd.isZero())
    return std::make_shared<const BigInt>(BigInt::ZERO);
  n = n / gcd;
  d = d / gcd;
  if(d == BigInt::ONE)
    return std::make_shared<const BigInt>(n);
  if(d == -BigInt::ONE)
    return std::make_shared<const BigInt>(-n);
  // TODO requires Ratio
}

double quotient(double n, double d) {
  if(d == 0)
    throw std::domain_error("Divide by zero error.");

	double q = n / d;
	if(q <= LONG_MAX && q >= LONG_MIN)
		return (double)(long) q;
	return (double) BigDecimal(q).toBigInt();
}
double remainder(double n, double d) {
  if(d == 0)
    throw std::domain_error("Divide by zero error.");

	double q = n / d;
	if(q <= LONG_MAX && q >= LONG_MIN)
	  return n - ((long)q) * d;
	return n - ((double)BigDecimal(q).toBigInt()) * d;
}

BigInt toBigInt(std::shared_ptr<const Number> x) {
  std::shared_ptr<const BigInt> bx = std::dynamic_pointer_cast<const BigInt>(x);
  if(bx)
    return *bx;
  return BigInt(*x);
}


// LongOps
const Ops &LongOps::combine(const Ops &y) const {
  return y.opsWith(*this);
}
const Ops &LongOps::opsWith(const LongOps &x) const {
  return *this;
}
const Ops &LongOps::opsWith(const DoubleOps &x) const {
  return DOUBLE_OPS;
}
const Ops &LongOps::opsWith(const BigIntOps&) const {
  return BIGINT_OPS;
}

bool LongOps::isZero(std::shared_ptr<const Number> x) const {
  return ((long) *x) == 0;
}
bool LongOps::isPos(std::shared_ptr<const Number> x) const {
  return ((long) *x) > 0;
}
bool LongOps::isNeg(std::shared_ptr<const Number> x) const {
  return ((long) *x) < 0;
}

std::shared_ptr<const Number> LongOps::add(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  return num(::add((long)*x, (long)*y));
}
std::shared_ptr<const Number> LongOps::addP(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  long lx = (long) *x;
  long ly = (long) *y;
  long ret = lx + ly;
  if((lx ^ ret) < 0 && (ly ^ ret) < 0)
    return BIGINT_OPS.add(x, y);
  return num(ret);
}

std::shared_ptr<const Number> LongOps::multiply(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  return num(::multiply((long) *x, (long) *y));
}
std::shared_ptr<const Number> LongOps::multiplyP(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  long lx = (long) *x;
  long ly = (long) *y;
  if(lx == LONG_MIN && ly < 0)
    return BIGINT_OPS.multiply(x, y);
  long ret = lx * ly;
  if((ly != 0) && (ret/ly != lx))
    return BIGINT_OPS.multiply(x, y);
  return num(ret);
}

std::shared_ptr<const Number> LongOps::divide(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  // TODO   requires Ratio
}
std::shared_ptr<const Number> LongOps::quotient(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  return num((long)*x / (long)*y);
}
std::shared_ptr<const Number> LongOps::remainder(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  return num((long)*x % (long)*y);
}

bool LongOps::equiv(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  return (long) *x == (long) *y;
}



// DoubleOps
const Ops &DoubleOps::combine(const Ops &y) const {
  return y.opsWith(*this);
}
const Ops &DoubleOps::opsWith(const LongOps &x) const {
  return *this;
}
const Ops &DoubleOps::opsWith(const DoubleOps &x) const {
  return *this;
}
const Ops &DoubleOps::opsWith(const BigIntOps&) const {
  return *this;
}

bool DoubleOps::isZero(std::shared_ptr<const Number> x) const {
  return ((double) *x) == 0;
}
bool DoubleOps::isPos(std::shared_ptr<const Number> x) const {
  return ((double) *x) > 0;
}
bool DoubleOps::isNeg(std::shared_ptr<const Number> x) const {
return ((double) *x) < 0;
}

std::shared_ptr<const Number> DoubleOps::add(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  return num((double)*x + (double)*y);
}

std::shared_ptr<const Number> DoubleOps::multiply(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  return num((double)*x * (double)*y);
}

std::shared_ptr<const Number> DoubleOps::divide(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  return num((double)*x / (double)*y);
}
std::shared_ptr<const Number> DoubleOps::quotient(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  return num(::quotient((double)*x, (double)*y));
}
std::shared_ptr<const Number> DoubleOps::remainder(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  return num(::remainder((double)*x, (double)*y));
}

bool DoubleOps::equiv(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  return (double) *x == (double) *y;
}


// BigIntOps
const Ops &BigIntOps::combine(const Ops& y) const {
  return y.opsWith(*this);
}
const Ops &BigIntOps::opsWith(const LongOps&) const {
  return *this;
}
const Ops &BigIntOps::opsWith(const DoubleOps&) const {
  return DOUBLE_OPS;
}
const Ops &BigIntOps::opsWith(const BigIntOps&) const {
  return *this;
}

bool BigIntOps::isZero(std::shared_ptr<const Number> x) const {
  BigInt bx = toBigInt(x);
  return bx == BigInt::ZERO;
}
bool BigIntOps::isPos(std::shared_ptr<const Number> x) const {
  BigInt bx = toBigInt(x);
  return bx.signum() > 0;
}
bool BigIntOps::isNeg(std::shared_ptr<const Number> x) const {
  BigInt bx = toBigInt(x);
  return bx.signum() < 0;
}

std::shared_ptr<const Number> BigIntOps::add(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  BigInt ret = toBigInt(x) + toBigInt(y);
  return std::make_shared<const BigInt>(ret);
}

std::shared_ptr<const Number> BigIntOps::multiply(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  BigInt ret = toBigInt(x) * toBigInt(y);
  return std::make_shared<const BigInt>(ret);
}

std::shared_ptr<const Number> BigIntOps::divide(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  return ::divide(toBigInt(x), toBigInt(x));
}
std::shared_ptr<const Number> BigIntOps::quotient(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  BigInt ret = toBigInt(x) / toBigInt(y);
  return std::make_shared<const BigInt>(ret);
}
std::shared_ptr<const Number> BigIntOps::remainder(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  BigInt ret = toBigInt(x) % toBigInt(y);
  return std::make_shared<const BigInt>(ret);
}

bool BigIntOps::equiv(std::shared_ptr<const Number> x, std::shared_ptr<const Number> y) const {
  return toBigInt(x) == toBigInt(y);
}

int main() {
  std::cout << "Hello World!\n";
}












