#include "ThreeAxisGenerator.h"

#include <functional>

#include <QtMath>

#include "DataBuffer.h"

namespace {
    constexpr double STEP_US = 0.00001; // 10us
    constexpr double PI = 3.14159265358979323846;

    void writeLineSegment(double speed, bool laserOn, double x1, double y1, double z1, double x2, double y2, double z2,
        int laserOnDelay, const std::function<void(double&, double&, double&)>& correction,
        const std::function<quint16(double)>& clamp) {
        speed *= 0.001;
        const double length = 0.001 * qSqrt(qPow(x2 - x1, 2) + qPow(y2 - y1, 2) + qPow(z2 - z1, 2));
        int nMax = static_cast<int>(length / (speed * STEP_US));
        if (!qFuzzyIsNull(length)) {
            nMax = static_cast<int>((length / (speed * STEP_US)) + 1);
        }

        if (qFuzzyCompare(x1, x2) && qFuzzyCompare(y1, y2) && qFuzzyCompare(z1, z2)) {
            double cx = x1;
            double cy = y1;
            double cz = z1;
            correction(cx, cy, cz);
            DataBuffer::instance().addProcessJumpData(clamp(cx), clamp(cy), clamp(cz), 0, 0);
            return;
        }

        for (int i = 1; i <= nMax; ++i) {
            double x = x1 + i * (x2 - x1) * speed * STEP_US / length;
            double y = y1 + i * (y2 - y1) * speed * STEP_US / length;
            double z = z1 + i * (z2 - z1) * speed * STEP_US / length;
            correction(x, y, z);

            if (laserOn) {
                if (i * 10 < laserOnDelay) {
                    DataBuffer::instance().addProcessJumpData(clamp(x), clamp(y), clamp(z), 0, 0);
                }
                else {
                    DataBuffer::instance().addProcessData(clamp(x), clamp(y), clamp(z), 0, 0);
                }
            }
            else {
                DataBuffer::instance().addProcessJumpData(clamp(x), clamp(y), clamp(z), 0, 0);
            }
        }
    }
}

quint16 ThreeAxisGenerator::clampToUint16(double value) {
    if (value < 0.0) {
        return 0;
    }
    if (value > 65535.0) {
        return 65535;
    }
    return static_cast<quint16>(value);
}

void ThreeAxisGenerator::applyCorrection(double& x, double& y, double& z) {
    // 放大/系数校正
    constexpr double X_GAIN = 776.991;
    constexpr double Y_GAIN = 778.062;
    constexpr double Z_GAIN = 830.0;

    constexpr double X_Z_COEFF = 0.13095395;
    constexpr double Y_Z_COEFF = 0.1702982;
    constexpr double ROTATION_DEG = 44.8;
    constexpr double Z_OFFSET = 4.5;

    z -= Z_OFFSET;

    const double zReal = z;
    const double xZ = X_Z_COEFF * zReal;
    const double yZ = Y_Z_COEFF * zReal;

    x = (x - xZ) * X_GAIN;
    y = (y + yZ) * Y_GAIN;
    z = Z_GAIN * z;

    const double rad = qDegreesToRadians(ROTATION_DEG);
    const double tempX = x;
    const double tempY = y;
    x = tempX * qCos(rad) + tempY * qSin(rad);
    y = tempY * qCos(rad) - tempX * qSin(rad);

    x = x - z + 32768.0;
    y += 32768.0;
    z += 32768.0;
}

void ThreeAxisGenerator::waitDelay(double x, double y, double z, int delayOn, int delayOff) {
    const int t = 10;
    applyCorrection(x, y, z);
    int n = delayOn / t;
    int i = 0;
    while (i < n) {
        DataBuffer::instance().addProcessData(clampToUint16(x), clampToUint16(y), clampToUint16(z), 0, 0);
        ++i;
    }
    n = delayOff / t;
    i = 0;
    while (i < n) {
        DataBuffer::instance().addProcessJumpData(clampToUint16(x), clampToUint16(y), clampToUint16(z), 0, 0);
        ++i;
    }
}

void ThreeAxisGenerator::generateLine(double speed, bool laserOn, double x1, double y1, double z1, double x2,
    double y2, double z2) {
    DataBuffer::instance().addProcessBegin();
    writeLineSegment(speed, laserOn, x1, y1, z1, x2, y2, z2, LASER_ON_DELAY,
        [](double& x, double& y, double& z) { applyCorrection(x, y, z); },
        [](double v) { return clampToUint16(v); });
    DataBuffer::instance().addProcessEnd();
}

void ThreeAxisGenerator::generateCircle(double x0, double y0, double x1, double y1, double z, double speed) {
    speed *= 0.001;

    double radius = qSqrt(qPow(x1 - x0, 2) + qPow(y1 - y0, 2));
    radius *= 0.001;

    const double circumferenceTime = 2 * PI * radius / speed;
    const int nMax = static_cast<int>((circumferenceTime / STEP_US) + 1);

    DataBuffer::instance().addProcessBegin();

    for (int n = 0; n < nMax; ++n) {
        const double angleRad = (n * STEP_US * 360 / circumferenceTime) * PI / 180.0;
        double x = x0 + radius * qCos(angleRad) * 1000.0;
        double y = y0 + radius * qSin(angleRad) * 1000.0;
        double zVal = z;
        applyCorrection(x, y, zVal);

        if (n * 10 < LASER_ON_DELAY) {
            DataBuffer::instance().addProcessJumpData(clampToUint16(x), clampToUint16(y), clampToUint16(zVal), 0, 0);
        }
        else {
            DataBuffer::instance().addProcessData(clampToUint16(x), clampToUint16(y), clampToUint16(zVal), 0, 0);
        }
    }

    DataBuffer::instance().addProcessEnd();
}

void ThreeAxisGenerator::generateRectangle(double x0, double y0, double z0, double x1, double y1, double z1,
    double speed, double yInterval) {
    const double yLength = qAbs(2 * (y1 - y0));
    const double xLength = qAbs(2 * (x1 - x0));
    const double zLength = qAbs(2 * (z1 - z0));

    const double yStart = y0 - yLength / 2.0;
    const double xStart = x0 - xLength / 2.0;
    const double zStart = z0 - zLength / 2.0;

    const double xInterval = xLength * yInterval / yLength;
    const double zInterval = zLength * yInterval / yLength;
    const double num = yLength / yInterval;

    DataBuffer::instance().addProcessBegin();

    // Jump 到起点
    writeLineSegment(JUMP_SPEED, false, 0, 0, 0, xStart, yStart, zStart, LASER_ON_DELAY,
        [](double& x, double& y, double& z) { applyCorrection(x, y, z); },
        [](double v) { return clampToUint16(v); });
    waitDelay(xStart, yStart, zStart, JUMP_DELAY, 0);

    for (int i = 0; i < num / 4; ++i) {
        writeLineSegment(speed, true, xStart + i * xInterval, yStart + i * yInterval, zStart + i * zInterval,
            xStart + i * xInterval, y1 - i * yInterval, zStart + i * zInterval, LASER_ON_DELAY,
            [](double& x, double& y, double& z) { applyCorrection(x, y, z); },
            [](double v) { return clampToUint16(v); });
        waitDelay(xStart + i * xInterval, y1 - i * yInterval, zStart + i * zInterval, POLYGON_DELAY, POLYGON_DELAY);

        writeLineSegment(speed, true, xStart + i * xInterval, y1 - i * yInterval, zStart + i * zInterval,
            x1 - i * xInterval, y1 - i * yInterval, z1 - i * zInterval, LASER_ON_DELAY,
            [](double& x, double& y, double& z) { applyCorrection(x, y, z); },
            [](double v) { return clampToUint16(v); });
        waitDelay(x1 - i * xInterval, y1 - i * yInterval, z1 - i * zInterval, POLYGON_DELAY, POLYGON_DELAY);

        writeLineSegment(speed, true, x1 - i * xInterval, y1 - i * yInterval, z1 - i * zInterval,
            x1 - i * xInterval, yStart + i * yInterval, z1 - i * zInterval, LASER_ON_DELAY,
            [](double& x, double& y, double& z) { applyCorrection(x, y, z); },
            [](double v) { return clampToUint16(v); });
        waitDelay(x1 - i * xInterval, yStart + i * yInterval, z1 - i * zInterval, POLYGON_DELAY, POLYGON_DELAY);

        writeLineSegment(speed, true, x1 - i * xInterval, yStart + i * yInterval, z1 - i * zInterval,
            xStart + i * xInterval, yStart + i * yInterval, zStart + i * zInterval, LASER_ON_DELAY,
            [](double& x, double& y, double& z) { applyCorrection(x, y, z); },
            [](double v) { return clampToUint16(v); });
        waitDelay(xStart + i * xInterval, yStart + i * yInterval, zStart + i * zInterval, POLYGON_DELAY, POLYGON_DELAY);

        writeLineSegment(JUMP_SPEED, false, xStart + i * xInterval, yStart + i * yInterval, zStart + i * zInterval,
            xStart + (i + 1) * xInterval, yStart + (i + 1) * yInterval, zStart + (i + 1) * zInterval,
            LASER_ON_DELAY, [](double& x, double& y, double& z) { applyCorrection(x, y, z); },
            [](double v) { return clampToUint16(v); });
        waitDelay(xStart + (i + 1) * xInterval, yStart + (i + 1) * yInterval, zStart + (i + 1) * zInterval,
            POLYGON_DELAY, POLYGON_DELAY);
    }

    DataBuffer::instance().addProcessEnd();
}