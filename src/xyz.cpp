#include <cmath>

#include "xyz.hpp"

XYZ::XYZ(double x, double y, double z)
{
    setX(x);
    setY(y);
    setZ(z);
}

double XYZ::getL() const
{
    return sqrt(pow(this->getX(), 2) + pow(this->getY(), 2) + pow(this->getZ(), 2));
}

double XYZ::getX() const
{
    return x;
}

double XYZ::getY() const
{
    return y;
}

double XYZ::getZ() const
{
    return z;
}

void XYZ::setX(double x)
{
    this->x = x;
}

void XYZ::setY(double y)
{
    this->y = y;
}

void XYZ::setZ(double z)
{
    this->z = z;
}
