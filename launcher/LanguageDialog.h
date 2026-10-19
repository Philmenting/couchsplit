// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include "Language.h"

namespace coop {
// Bilingual until an explicit choice is committed by the caller. Closing or
// pressing Escape rejects the choice without writing preferences.
inline QString chooseLauncherLanguage(QWidget* parent) {
    QDialog dialog(parent);dialog.setObjectName("languageDialog");
    dialog.setWindowTitle("Language / Sprache");
    auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(24,24,24,24);layout->setSpacing(16);
    auto* title=new QLabel("Language / Sprache");title->setProperty("role","sectionTitle");layout->addWidget(title);
    auto* description=new QLabel("Choose your launcher language.\nWähle die Sprache für den Launcher.");description->setWordWrap(true);layout->addWidget(description);
    auto* choices=new QHBoxLayout;layout->addLayout(choices);
    QString selected;
    QPushButton* preferred=nullptr;
    for(const auto& option:{qMakePair(QString("de"),QString("Deutsch")),qMakePair(QString("en"),QString("English"))}) {
        auto* button=new QPushButton(option.second);button->setObjectName("language-"+option.first);button->setMinimumHeight(56);choices->addWidget(button);
        QObject::connect(button,&QPushButton::clicked,&dialog,[&,code=option.first]{selected=code;dialog.accept();});
        if(option.first==i18n::systemLanguage())preferred=button;
    }
    auto* hint=new QLabel("Change later in Settings.\nSpäter unter Einstellungen änderbar.");hint->setObjectName("subtitle");hint->setWordWrap(true);layout->addWidget(hint);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Cancel);buttons->button(QDialogButtonBox::Cancel)->setText("Cancel / Abbrechen");layout->addWidget(buttons);
    QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    dialog.setMinimumWidth(qMin(560,qMax(280,parent->width()-48)));
    if(preferred){preferred->setDefault(true);preferred->setFocus();}
    return dialog.exec()==QDialog::Accepted?selected:QString();
}
}
