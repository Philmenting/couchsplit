// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>
#include <QVariant>
#include <atomic>
#include "Translations.h"

namespace coop::i18n {
inline std::atomic_bool english{false};
inline bool supported(const QString& language) {return language=="de" || language=="en";}
inline QString language() {return english.load()?QStringLiteral("en"):QStringLiteral("de");}
inline QString systemLanguage() {return QLocale::system().language()==QLocale::German?"de":"en";}

class Translator final : public QTranslator {
public:
    bool isEmpty() const override {return false;}
    QString translate(const char* context,const char* source,const char*,int) const override {
        const auto key=QString::fromUtf8(source);
        if(qstrcmp(context,"PlatinumCoop")==0)
            return english.load()?englishCatalog().value(key):QString();
        // Qt-owned standard buttons should follow the chosen app language.
        if(qstrcmp(context,"QPlatformTheme")==0 || qstrcmp(context,"QDialogButtonBox")==0 || qstrcmp(context,"QMessageBox")==0) {
            static const QHash<QString,QString> german{{"Cancel","Abbrechen"},{"Close","Schließen"},{"Open","Öffnen"},{"Save","Speichern"},{"Yes","Ja"},{"No","Nein"},{"&Yes","&Ja"},{"&No","&Nein"},{"OK","OK"}};
            if(german.contains(key))return english.load()?key:german.value(key);
        }
        return {};
    }
};
inline bool setLanguage(const QString& value) {
    if(!supported(value))return false;
    static Translator translator;
    static bool installed=false;
    if(installed && language()==value)return true;
    if(installed)QCoreApplication::removeTranslator(&translator);
    english.store(value=="en");
    installed=QCoreApplication::installTranslator(&translator);
    return installed;
}
inline QString text(const QString& source) {
    const auto utf8=source.toUtf8();
    return QCoreApplication::translate("PlatinumCoop",utf8.constData());
}
template<class Widget> Widget* bindText(Widget* widget,const QString& source) {
    widget->setProperty("translationKey",source);
    widget->setText(text(source));
    return widget;
}
}
