#pragma once

#include <QtGlobal>

class ThreeAxisGenerator {
public:
    // 生成三轴直线：只使用 X/Y/Z，不使用 A/B。
    static void generateLine(double speed, bool laserOn, double x1, double y1, double z1, double x2, double y2, double z2);

    // 生成三轴圆：以 (x0, y0) 为圆心，(x1, y1) 为圆上一点，Z 固定。
    static void generateCircle(double x0, double y0, double x1, double y1, double z, double speed);

    // 生成简单的三轴矩形（回形填充），上下扫描，Z 线性插值。
    static void generateRectangle(double x0, double y0, double z0, double x1, double y1, double z1, double speed, double yInterval);

private:
    static constexpr int LASER_ON_DELAY = 100;
    static constexpr int JUMP_SPEED = 500;
    static constexpr int JUMP_DELAY = 150;
    static constexpr int POLYGON_DELAY = 450;

    static quint16 clampToUint16(double value);
    static void applyCorrection(double& x, double& y, double& z);
    static void waitDelay(double x, double y, double z, int delayOn, int delayOff);
};