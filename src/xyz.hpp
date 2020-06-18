#ifndef XYZ_H
#define XYZ_H

class XYZ
{
private:
    double x = 0;
    double y = 0;
    double z = 0;

public:
    XYZ(double x, double y, double z);

    double getL() const; // get magnitude of the vector

    double getX() const;
    double getY() const;
    double getZ() const;

    void setX(double x);
    void setY(double y);
    void setZ(double z);
};

#endif
