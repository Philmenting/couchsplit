// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "Language.h"
#include "../gba/FireRedRecovery.h"
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <functional>

class FireRedNotice final : public QFrame {
    QLabel *status,*explanation;
    QPushButton* proceed;
    uint32_t pause=0;
    bool requested=false;
public:
    std::function<bool(uint32_t)> continueIndividually;
    explicit FireRedNotice(QWidget* parent=nullptr):QFrame(parent) {
        setObjectName("fireRedNotice");
        setStyleSheet("QFrame#fireRedNotice { background:#183847; border:1px solid #547681; border-radius:8px; } QLabel { color:#e0ebed; background:transparent; border:0; }");
        auto* layout=new QVBoxLayout(this);layout->setContentsMargins(12,8,12,8);layout->setSpacing(6);
        status=new QLabel;status->setObjectName("fireRedPlayStatus");status->setWordWrap(true);layout->addWidget(status);
        explanation=new QLabel;explanation->setObjectName("fireRedRecoveryHelp");explanation->setWordWrap(true);layout->addWidget(explanation);
        proceed=new QPushButton;proceed->setObjectName("fireRedContinueIndividual");proceed->setMinimumHeight(40);layout->addWidget(proceed);
        connect(proceed,&QPushButton::clicked,this,[this] {
            if(!requested && continueIndividually && continueIndividually(pause)) {
                requested=true;proceed->setEnabled(false);
            }
        });
    }
    void showState(uint32_t state,const QString& text) {
        if(pause!=state) {pause=state;requested=false;}
        status->setText(text);
        const bool blocked=coop::firered::battlePaused(state);
        explanation->setVisible(blocked);proceed->setVisible(blocked);proceed->setEnabled(!requested);
        explanation->setText(coop::i18n::text("Beide Spiele sind pausiert. Euer bisheriger Fortschritt bleibt in dieser Sitzung erhalten. Ihr könnt die ausstehenden Kämpfe einzeln spielen; sobald beide wieder in der Spielwelt sind, geht Koop weiter.")+"\n"+
            coop::i18n::text(coop::firered::pairState(state)==coop::firered::PairState::Enigma?
                "Legt danach die Enigmabeere ab, um wieder gemeinsam zu kämpfen.":
                "Legt danach EP-Teiler ab, um wieder gemeinsam zu kämpfen."));
        proceed->setText(coop::i18n::text("Einzelkämpfe fortsetzen"));
        updateGeometry();
    }
    void focusAction() {proceed->setFocus(Qt::OtherFocusReason);}
    void activateAction() {if(proceed->isVisible() && proceed->isEnabled())proceed->click();}
};
