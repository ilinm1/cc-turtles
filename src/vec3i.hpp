#pragma once

struct Vec3i
{
	int X;
	int Y;
	int Z;

    Vec3i() {};

    Vec3i(int xyz) 
    {
        X = Y = Z = xyz;
    };

    Vec3i(int x, int y, int z)
    {
        X = x; Y = y; Z = z;
    }

    //returns vector with maximal component values of both input vectors
    static Vec3i Max(Vec3i a, Vec3i b)
    {
        return Vec3i(std::max(a.X, b.X), std::max(a.Y, b.Y), std::max(a.Z, b.Z));
    }

    //returns vector with minimal component values of both input vectors
    static Vec3i Min(Vec3i a, Vec3i b)
    {
        return Vec3i(std::min(a.X, b.X), std::min(a.Y, b.Y), std::min(a.Z, b.Z));
    }

    Vec3i Abs()
    {
        return Vec3i(abs(X), abs(Y), abs(Z));
    }

    unsigned int LengthLinear()
    {
        return abs(X) + abs(Y) + abs(Z);
    }

    Vec3i& operator+=(const Vec3i& r)
    {
        X += r.X;
        Y += r.Y;
        Z += r.Z;
        return *this;
    }

    friend Vec3i operator+(Vec3i l, const Vec3i& r)
    {
        l += r;
        return l;
    }

    Vec3i& operator-=(const Vec3i& r)
    {
        X -= r.X;
        Y -= r.Y;
        Z -= r.Z;
        return *this;
    }

    friend Vec3i operator-(Vec3i l, const Vec3i& r)
    {
        l -= r;
        return l;
    }

    Vec3i& operator*=(const float& r)
    {
        X *= r;
        Y *= r;
        Z *= r;
        return *this;
    }

    friend Vec3i operator*(Vec3i l, const float& r)
    {
        l *= r;
        return l;
    }

    Vec3i& operator/=(const float& r)
    {
        X /= r;
        Y /= r;
        Z /= r;
        return *this;
    }

    friend Vec3i operator/(Vec3i l, const float& r)
    {
        l /= r;
        return l;
    }

    friend bool operator==(const Vec3i& l, const Vec3i& r)
    {
        return l.X == r.X && l.Y == r.Y && l.Z == r.Z;
    }

    friend bool operator!=(const Vec3i& l, const Vec3i& r)
    {
        return !(l == r);
    }

    operator std::string()
    {
        return std::to_string(X) + " " + std::to_string(Y) + " " + std::to_string(Z);
    }

    static Vec3i FromString(std::string str)
    {
        Vec3i result;
        sscanf(str.data(), "%i %i %i", &result.X, &result.Y, &result.Z);
        return result;
    }
};
