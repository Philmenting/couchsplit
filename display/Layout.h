// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QRectF>
#include <array>
namespace coop {
inline QRectF fitAspect(QRectF area,double aspect) {
    const double w=qMin(area.width(),area.height()*aspect),h=w/aspect;
    return {area.center().x()-w/2,area.center().y()-h/2,w,h};
}
inline QRectF fit(QRectF area) {
    const double w = qMin(area.width(), area.height() * 4.0 / 3.0);
    const double h = w * 3.0 / 4.0;
    return {area.center().x() - w/2, area.center().y() - h/2, w, h};
}
// 0: top large + touch small; 1: side-by-side; 2: single-screen toggle.
inline std::array<QRectF, 2> screens(QRectF area, int layout, bool bottomOnly) {
    if (layout == 2) return bottomOnly ? std::array<QRectF,2>{{{}, fit(area)}} : std::array<QRectF,2>{{fit(area), {}}};
    if (layout == 1) {
        const double half = area.width()/2;
        return {{fit({area.x(),area.y(),half,area.height()}), fit({area.x()+half,area.y(),half,area.height()})}};
    }
    // Reserve a third of the height for the lower screen, then letterbox both.
    const double top = area.height()*0.70;
    return {{fit({area.x(),area.y(),area.width(),top}), fit({area.x(),area.y()+top,area.width(),area.height()-top})}};
}
inline QRectF playerArea(QRectF area, int player, bool horizontal) {
    if (horizontal) { const double h = area.height()/2; return {area.x(),area.y()+player*h,area.width(),h}; }
    const double w=area.width()/2; return {area.x()+player*w,area.y(),w,area.height()};
}
}
