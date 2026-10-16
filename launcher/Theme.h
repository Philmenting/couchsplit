// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QApplication>
#include <QFontDatabase>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QWidget>

namespace coop::theme {
inline QString pixelFamily() {
    static const QString family=[] {
        const int id=QFontDatabase::addApplicationFont(":/launcher/fonts/Silkscreen-Regular.ttf");
        const auto names=QFontDatabase::applicationFontFamilies(id);
        return names.isEmpty()?QApplication::font().family():names.first();
    }();
    return family;
}
inline void labelButtons(QDialogButtonBox& box) {
    if(auto* button=box.button(QDialogButtonBox::Cancel))button->setText("Abbrechen");
    if(auto* button=box.button(QDialogButtonBox::Close))button->setText("Schließen");
}
enum class Glyph { Play, Controller, Screens, Cartridge, Save, Buttons, Settings, Logs, Exit };
inline QIcon icon(Glyph glyph, QColor color=QColor("#f3eee0")) {
    QIcon result;
    for(int size:{24,32,48,64,96}) {
        QPixmap pixmap(size,size);pixmap.fill(Qt::transparent);QPainter p(&pixmap);
        p.setRenderHint(QPainter::Antialiasing);p.scale(size/24.0,size/24.0);
        p.setPen(QPen(color,1.7,Qt::SolidLine,Qt::SquareCap,Qt::MiterJoin));p.setBrush(Qt::NoBrush);
        switch(glyph) {
        case Glyph::Play: {QPainterPath path;path.moveTo(7,4);path.lineTo(20,12);path.lineTo(7,20);path.closeSubpath();p.fillPath(path,color);break;}
        case Glyph::Controller:
            p.drawRoundedRect(QRectF(2,6,20,13),3,3);p.drawLine(6,12,12,12);p.drawLine(9,9,9,15);p.drawEllipse(QPointF(17,10),.8,.8);p.drawEllipse(QPointF(19,14),.8,.8);break;
        case Glyph::Screens:p.drawRect(QRectF(3,2,18,8));p.drawRect(QRectF(3,14,18,8));p.drawLine(9,12,15,12);break;
        case Glyph::Cartridge:p.drawRect(QRectF(5,3,14,18));p.drawRect(QRectF(8,6,8,7));for(int x:{8,12,16})p.drawLine(x,17,x,21);break;
        case Glyph::Save:p.drawRect(QRectF(3,3,18,18));p.drawRect(QRectF(7,3,10,6));p.drawRect(QRectF(7,14,10,7));break;
        case Glyph::Buttons:p.drawEllipse(QPointF(8,8),3,3);p.drawEllipse(QPointF(17,16),3,3);p.drawLine(15,5,21,5);p.drawLine(3,19,9,19);break;
        case Glyph::Settings:p.drawLine(3,6,21,6);p.drawLine(3,12,21,12);p.drawLine(3,18,21,18);p.setBrush(QColor("#223f4e"));p.drawRect(QRectF(7,3,4,6));p.drawRect(QRectF(15,9,4,6));p.drawRect(QRectF(5,15,4,6));break;
        case Glyph::Logs:p.drawRect(QRectF(5,2,14,20));p.drawLine(8,7,16,7);p.drawLine(8,12,16,12);p.drawLine(8,17,13,17);break;
        case Glyph::Exit:p.drawLine(10,3,3,3);p.drawLine(3,3,3,21);p.drawLine(3,21,10,21);p.drawLine(9,12,21,12);p.drawLine(17,8,21,12);p.drawLine(17,16,21,12);break;
        }
        p.end();result.addPixmap(pixmap);
    }
    return result;
}
// The open-license display font is embedded; body and controls keep the native
// system font for readable German labels and platform-appropriate text input.
inline void apply(QWidget* root, bool large, bool compact) {
    const int scale=large?2:1;
    QString css=QString(R"(
        QWidget { color:#f3eee0; font-size:%1px; }
        QWidget#launcherWindow, QWidget#menuCanvas { background:#142c3b; }
        QLabel { background:transparent; }
        QLabel#title { font-family:'%2'; font-size:%3px; color:#f5cf67; }
        QLabel#subtitle { color:#c0d2d7; font-size:%4px; }
        QLabel[role='sectionTitle'] { font-family:'%2'; font-size:%5px; color:#f5cf67; }
        QLabel#alphaBadge { color:#c0d2d7; border:1px solid #72909c; padding:%6px; }
        QFrame#gamePanel { background:#f3eee0; border:2px solid #8fa5aa; border-radius:12px; }
        QFrame#gamePanel QLabel { color:#213846; }
        QFrame#gamePanel QLabel[role='sectionTitle'] { color:#8b392e; }
        QLabel#romLabel { font-size:%7px; font-weight:600; }
        QLabel#padLabel { font-size:%4px; }
        QLabel#sessionStatus { color:#c0d2d7; font-size:%4px; }
        QLabel#sessionStatus[tone='error'] { color:#ffb0a3; }
        QLabel#navigationHelp { color:#c0d2d7; font-size:%4px; }
        QPushButton { background:#223f4e; border:2px solid #5b7b89; border-radius:8px;
            padding:%8px; text-align:left; min-height:%9px; }
        QPushButton:hover { background:#2d5261; border-color:#b6c9cd; }
        QPushButton:focus { background:#325666; border:2px solid #f5cf67; color:#fff9e9; }
        QPushButton:pressed { background:#102633; border-color:#f5cf67; }
        QPushButton:disabled { color:#8ba1aa; background:#1a3443; border-color:#3e5966; }
        QPushButton#startGame { background:#b83f35; border-color:#e78d7a; color:#fff9e9; font-size:%7px; font-weight:600; }
        QPushButton#startGame:hover { background:#cc4b3e; }
        QPushButton#startGame:focus { background:#b83f35; border:2px solid #f5cf67; }
        QPushButton#startGame:pressed { background:#92352e; }
        QPushButton#exitLauncher { color:#e5b4a8; }
        QDialog, QMessageBox { background:#183442; }
        QLineEdit, QComboBox, QTextEdit { background:#0e2532; color:#f3eee0;
            border:1px solid #8fa5aa; border-radius:4px; padding:%6px;
            selection-background-color:#f5cf67; selection-color:#213846; }
        QLineEdit:focus, QComboBox:focus, QTextEdit:focus { border:2px solid #f5cf67; }
        QComboBox QAbstractItemView { background:#183442; color:#f3eee0; selection-background-color:#325666; }
        QScrollArea { border:0; background:transparent; }
        QScrollBar:vertical { background:#142c3b; width:%10px; margin:0; }
        QScrollBar::handle:vertical { background:#72909c; min-height:32px; border-radius:4px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background:transparent; }
        QToolTip { color:#213846; background:#f3eee0; border:1px solid #8fa5aa; padding:6px; }
    )").arg(18*scale).arg(pixelFamily()).arg((compact?23:30)*scale)
      .arg(15*scale).arg(14*scale).arg(6*scale).arg(21*scale)
      .arg(12*scale).arg(28*scale).arg(10*scale);
    root->setStyleSheet(css);
}
}
