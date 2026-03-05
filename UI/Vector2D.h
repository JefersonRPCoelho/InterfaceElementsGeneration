/**
 * MIT License
 *
 * Copyright (c) 2023 Jéferson Coêlho
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once
#include <cmath>
#include <iostream>
#include <vector>
/**
 * @file
 *
 * @brief Declares alias to allow using a Vector2D as a Point2D. In some cases, it
 * is more convenient to use a Point definition, despite both having the same
 * representation.
 */

/**
 *  @brief Forward declaration to a two-dimensional vector class.
 */
template<class T>
class Vector2D;

/**
 * @brief An alias that allows to use a vector representation as a point.
 */
template<class T>
using Point2D = Vector2D<T>;

/**
 * @brief An alias to use a very specific point representation with a single floating point. This type is commonly used
 * in render representations.
 */
using Point2Df = Point2D<float>;

/**
 * @brief An alias to use a very specific point representation with a double floating point. This type is commonly used
 * in render representations.
 */
using Point2Dd = Point2D<double>;


/**
 * @brief Alias to facilitate the usage of a vector of point as a polygon.
 */
template<class T>
using Polygon2D = std::vector<Point2D<T>>;

/**
 * @brief Alias to facilitate the usage of a vector of point as a Polyline2D.
 */
template<class T>
using Polyline2D = std::vector<Point2D<T>>;

/**
 * @brief The Vector2D class represent a vector or a point in two dimensions.
 *
 * It implements all basic arithmetic and algebraic operations.
 */
template<class T>
class Vector2D
{
public:
    /**
     * @brief Default constructor.
     */
    Vector2D() = default;

    /**
     * @brief Constructor that receives vector coordinates as parameters.
     * @param x x coordinate.
     * @param y y coordinate.
     */
    Vector2D( T x, T y ) :
        _xp(x),
        _yp(y)
    {
    }

    /**
     * @brief Compute the angle between the current vector and q vector.
     * @param q The Vector with which the current one shall be compared
     * @return Angle between vectors in rad.
     */
    T angle( const Vector2D& q ) const
    {
        const Vector2D& u = *this;
        return std::atan2( u ^ q, u * q );
    }

    /**
     * @brief Compute the angle between vectors u = p - r and w = r - q.
     * @param p first point.
     * @param q base point of vectors.
     * @param r second point.
     * @return Angle between vectors in rad.
     */
    static T angle( const Point2D<T>& p, const Point2D<T> &q, const Point2D<T>& r )
    {
        Point2D<T> u = p - q, w = r - q;
        return atan2( u ^ w, u * w );
    }

    /**
     * @brief Check if a direction is counter-clock wise.
     * @param p first Point2D.
     * @param q second Point2D.
     * @param r third Point2D.
     * @return Returns 1 if it is counterclockwise, -1 if it is clockwise and 0 if it is a line.
     */
    static int ccw( const Point2D<T>& p, const Point2D<T>& q, const Point2D<T>& r )
    {
        return cmp((p - r) ^ (q - r));
    }

    /**
     * @brief Compare two vectors.
     * @param q vector to be compared with the current vector.
     * @param eps tolerance to be used.
     * @return -1 if current < y, 0 if current == y and 1 if current > y considering the tolerance.
     * eps.
     */
    int cmp( const Vector2D& q, T eps = 0 ) const
    {
        if (int t = cmp( _xp, q._xp, eps))
            return t;
        return cmp( _yp, q._yp, eps );
    }

    /**
     * @brief Verify if x <= y with a tolerance eps.
     * @param x x value to be compared with y.
     * @param y y value to be compared with x.
     * @param eps tolerance to be used.
     * @return -1 if x < y, 0 if x == y and 1 if x > y considering the tolerance eps.
     */
    static int cmp( T x, T y = static_cast<T>(0), T eps = 0 )
    {
        return (x <= y + eps) ? (x + eps < y) ? -1 : 0 : 1;
    }

    /**
     * @brief Compute the norm of the current Vector.
     * @return the vector norm value.
     */
    T norm( ) const
    {
        return std::sqrt(_xp * _xp + _yp * _yp);
    }

    /**
     * @brief Normalize the current vector.
     * @return the previous norm of the Vector.
     */
    T normalize( )
    {
        T nor = norm();
        _xp = _xp / nor;
        _yp = _yp / nor;
		return nor;
    }

    /**
     * @brief Sum two vectors and return the resulting vector: a + p.
     * @param p Vector that will be added
     * @return New vector which is the sum between the caller of the function and p
     */
    Vector2D operator+(const Vector2D& p) const
    {
         return Vector2D<T> (_xp + p._xp, _yp + p._yp);
    }

    /**
     * @brief Sum a vector q with the current vector and return the current vector: a += q.
     * @param q Vector that will be added.
     * @return Modifies the caller of the function and returns it ;
     */
    Vector2D& operator+=(const Vector2D& q)
    {
        _xp += q._xp;
        _yp += q._yp;
        return *this;
	}

    /**
     * @brief Invert coordinate signals: -(a).
     * @return Returns a new vector which is the negative of the caller of the function
     */
    Vector2D operator-() const
    {
        return Vector2D<T> (-_xp, -_yp);
    }

    /**
     * @brief Subtract two vectors: a - p.
     * @param p Vector that will be subtracted.
     * @return New vector resulting from a - p.
     */
    Vector2D operator-(const Vector2D& p) const
    {
        return Vector2D<T> (_xp - p._xp, _yp - p._yp);
    }

    /**
     * @brief Subtract a vector q from the current vector: a -= q.
     * @param q Vector that will be subtracted.
     * @return  Returns the caller modified and returned.
     */
    Vector2D& operator-=(const Vector2D& q)
    {
        _xp -= q._xp;
        _yp -= q._yp;
		return *this;
    }

    /**
     * @brief Multiply a vector by a scalar t: a * t.
     * @param t scalar multiplier.
     * @return New vector which is a * t.
     */
    Vector2D operator*(T t) const
    {
        return Vector2D<T>( t * _xp , t * _yp );
    }

    /**
     * @brief Overloads to access the coordinates.
     * @param i dimension index: 0 to x and 1 to y.
     * @return the coordinate.
     */
    T &operator[](int i)
    {
       return *(&_xp + i);
    }

    /**
     * @brief Overloads to access the coordinates.
     * @param i dimension index: 0 to x and 1 to y.
     * @return the coordinate.
     */
    T operator[](int i) const
    {
        return *(&_xp + i);
    }

    /**
     * @brief Gets the x coordinate.
     * @return x coordinate.
     */
    T x() const
    {
        return _xp;
    }

    /**
     * @brief Gets the y coordinate.
     * @return y coordinate.
     */
    T y() const
    {
        return _yp;
    }

    /**
     * @brief Multiply a vector by a scalar t: t * a.
     * @param t Scalar multiplier.
     * @param p Vector that is being multiplied.
     * @return Returns a new vector with value t * a.
     */
    friend Vector2D<T> operator*(T t, const Vector2D<T>& p)
    {
        return Vector2D<T>(t * p._xp, t * p._yp);
    }

    /**
     * @brief Dot product: a * p.
     * @param p Vector that appears after the operator.
     * @return Template T with the result of the operation.
     */
    T operator*(const Vector2D& p) const
    {
        return _xp * p._xp + _yp * p._yp;
    }

    /**
     * @brief Multiply the current vector to a scalar t: a *= t.
     * @param t double scalar.
     * @return current vector with modified values.
     */
    Vector2D& operator*=(T t)
    {
        _xp *= t;
        _yp *= t;
		return *this;
    }

    /**
     * @brief Divide a vector to a scalar t: a / t.
     * @param t -double scalar.
     * @return New Vector with expected values.
     */
    Vector2D operator/(T t) const
    {
        return Vector2D<T>(_xp / t, _yp / t);
    }

    /**
     * @brief Divide the current vector to a scalar t a /= t.
     * @param t scalar denominator.
     * @return  modifies and return current vector.
     */
    Vector2D& operator/=(T t)
    {
        _xp /= t;
        _yp /= t;
        return *this;
    }

    /**
     * @brief Cross product: a x q
     * @param q vector that appears after the operator.
     * @return the resulting cross-product value.
     */
    T operator^(const Vector2D& q) const
    {
        return _xp * q._yp - q._xp * _yp;
    }

    /**
     * @brief Compare if two vectors are equals.
     * @param q vector that is being verified along with the caller.
     * @return true if equal, false if different.
     */
    bool operator==(const Vector2D& q) const
    {
        return cmp( q ) == 0;
    }

    /**
     * @brief Verify if two vectors are different.
     * @param q vector that is being verified along with the caller.
     * @return true if different, false if equal.
     */
    bool operator!=(const Vector2D& q) const
    {
        return cmp( q ) != 0;
    }

    /**
     * @brief Compare two points in order to determine the order of them.
     * @param q vector that is being compared.
     * @return true if <, false if >=.
     */
    bool operator<(const Vector2D& q) const
    {
        return cmp( q ) < 0;
    }

    /**
     * @brief Print the point coordinates.
     * @param o stream that will be used to print.
     * @param p Vector or Point you desire to print.
     * @return modified stream with the expected format.
     */
    friend std::ostream& operator<<( std::ostream& o, const Vector2D& p)
    {
        o << "(" << p._xp << "," << p._yp << ")";
		return o;
    }

    /**
     * @brief compute the squared norm.
     * @return the squared norm.
     */
    T sqrNorm( ) const
    {
        return _xp * _xp + _yp * _yp;
    }

    /**
     * @brief sets the point with new coordinates
     * @param p new to coordinates to be set
     */
    void setPoint( const Vector2D<T>& p )
    {
        _xp = p._xp;
        _yp = p._yp;
    }

    /**
     * @brief set a new x coordinate.
     * @param x new x coordinate.
     */
    void setX(T x)
    {
        _xp = x;
    }

    /**
     * @brief set a new y coordinate.
     * @param y new y coordinate.
     */
    void setY(T y)
    {
        _yp = y;
    }
private:
    /**
     * @brief x vector/point coordinate.
     **/
    T _xp = 0;

    /**
     * @brief y vector/point coordinate.
     */
    T _yp = 0;
};
