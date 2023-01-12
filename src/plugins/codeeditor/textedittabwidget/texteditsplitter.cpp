﻿/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     hongjinchuan<hongjinchuan@uniontech.com>
 *
 * Maintainer: hongjinchuan<hongjinchuan@uniontech.com>
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "texteditsplitter.h"
#include "transceiver/codeeditorreceiver.h"

#include <QBoxLayout>
#include <QDebug>

namespace Private {
static TextEditSplitter *splitter = nullptr;
}

TextEditSplitter::TextEditSplitter(QWidget *parent)
    : QWidget (parent)
    , vLayout(new QVBoxLayout)
    , mainSplitter(new QSplitter)
{
    tabWidget = new TextEditTabWidget(mainSplitter);
    mainSplitter->addWidget(tabWidget);
    mainSplitter->setHandleWidth(0);
    tabWidgets.insert(tabWidget, true);
    splitters.append(mainSplitter);
    tabWidget->setCloseButtonVisible(false);
    tabWidget->setSplitButtonVisible(false);

    QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toOpenFileWithKey,
                     tabWidget, &TextEditTabWidget::openFileWithKey);
    QObject::connect(tabWidget, &TextEditTabWidget::splitClicked,
                     this, &TextEditSplitter::doSplit);
    QObject::connect(tabWidget, &TextEditTabWidget::closed,
                     this, &TextEditSplitter::doClose);
    QObject::connect(tabWidget, &TextEditTabWidget::selected,
                     this, &TextEditSplitter::doSelected);
    QObject::connect(tabWidget, &TextEditTabWidget::closeWidget,
                     this, &TextEditSplitter::doClose);
    QObject::connect(tabWidget, &TextEditTabWidget::sigOpenFile,
                     this, &TextEditSplitter::doShowSplit);
    QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toAddDebugPoint,
                     tabWidget, &TextEditTabWidget::addDebugPoint);
    QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toRemoveDebugPoint,
                     tabWidget, &TextEditTabWidget::removeDebugPoint);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addStretch(20);

    vLayout->addLayout(hLayout);
    vLayout->addWidget(mainSplitter);
    this->setLayout(vLayout);

}

TextEditSplitter::~TextEditSplitter()
{

}

void TextEditSplitter::doSplit(Qt::Orientation orientation, const newlsp::ProjectKey &key, const QString &file)
{
    auto oldEditWidget = qobject_cast<TextEditTabWidget*>(sender());
    if (!oldEditWidget)
        return;
    auto splitter = qobject_cast<QSplitter *>(oldEditWidget->parent());
    QSplitter *newSplitter = new QSplitter(splitter);
    splitters.append(newSplitter);
    newSplitter->setOrientation(orientation);
    newSplitter->setHandleWidth(0);
    newSplitter->setOpaqueResize(true);
    int index = splitter->indexOf(oldEditWidget);
    oldEditWidget->setParent(newSplitter);
    oldEditWidget->setCloseButtonVisible(true);
    newSplitter->setChildrenCollapsible(false);

    TextEditTabWidget *newEditWidget = new TextEditTabWidget(newSplitter);
    tabWidgets.insert(newEditWidget, true);
    tabWidgets[oldEditWidget] = false;
    if (tabWidgets.size() >= 4) {
        for (auto it = tabWidgets.begin(); it != tabWidgets.end(); ++it) {
            it.key()->setSplitButtonVisible(false);
        }
    }
    if (key.isValid()) {
        newEditWidget->openFileWithKey(key, file);
    }

    splitter->insertWidget(index, newSplitter);

    // cancel all open file sig-slot from texteditwidget
    QObject::disconnect(EditorCallProxy::instance(), &EditorCallProxy::toOpenFileWithKey,
                        oldEditWidget, &TextEditTabWidget::openFileWithKey);

    QObject::disconnect(EditorCallProxy::instance(), &EditorCallProxy::toJumpFileLineWithKey,
                        oldEditWidget, &TextEditTabWidget::jumpToLineWithKey);

    QObject::disconnect(EditorCallProxy::instance(), &EditorCallProxy::toRunFileLineWithKey,
                        oldEditWidget, &TextEditTabWidget::runningToLineWithKey);

    // connect new texteditwidget openfile slot
    QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toOpenFileWithKey,
                     newEditWidget, &TextEditTabWidget::openFileWithKey);

    // connect selected texteditwidget sig-slot bind, from texteditwidget focus
    QObject::connect(newEditWidget, &TextEditTabWidget::splitClicked,
                     this, &TextEditSplitter::doSplit);

    QObject::connect(newEditWidget, &TextEditTabWidget::selected,
                     this, &TextEditSplitter::doSelected);

    QObject::connect(newEditWidget, &TextEditTabWidget::closed,
                     this, &TextEditSplitter::doClose);
    QObject::connect(newEditWidget, &TextEditTabWidget::closeWidget,
                     this, &TextEditSplitter::doClose);
    QObject::connect(newEditWidget, &TextEditTabWidget::sigOpenFile,
                     this, &TextEditSplitter::doShowSplit);
    QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toAddDebugPoint,
                     newEditWidget, &TextEditTabWidget::addDebugPoint);
    QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toRemoveDebugPoint,
                     newEditWidget, &TextEditTabWidget::removeDebugPoint);
}

void TextEditSplitter::doSelected(bool state)
{
    auto textEditTabWidget = qobject_cast<TextEditTabWidget*>(sender());
    if (!textEditTabWidget)
        return;

    auto findIsConnEdit = [=]() -> TextEditTabWidget*
    {
        for(auto it = tabWidgets.begin(); it != tabWidgets.end(); ++it) {
            if (it.value() == true)
                return it.key();
        }
        return nullptr;
    };

    if (tabWidgets.values().contains(true)) {
        if (state) {
            while (tabWidgets.values().contains(true)) {
                auto edit = findIsConnEdit();
                if (edit) {
                    QObject::disconnect(EditorCallProxy::instance(), &EditorCallProxy::toOpenFileWithKey,
                                        edit, &TextEditTabWidget::openFileWithKey);
                    QObject::disconnect(EditorCallProxy::instance(), &EditorCallProxy::toJumpFileLineWithKey,
                                        edit, &TextEditTabWidget::jumpToLineWithKey);
                    QObject::disconnect(EditorCallProxy::instance(), &EditorCallProxy::toRunFileLineWithKey,
                                        edit, &TextEditTabWidget::runningToLineWithKey);
                    QObject::disconnect(EditorCallProxy::instance(), &EditorCallProxy::toSetModifiedAutoReload,
                                        edit, &TextEditTabWidget::setModifiedAutoReload);
                    tabWidgets[edit] = false;
                }
            }
            QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toAddDebugPoint,
                             textEditTabWidget, &TextEditTabWidget::addDebugPoint);
            QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toRemoveDebugPoint,
                             textEditTabWidget, &TextEditTabWidget::removeDebugPoint);
            QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toOpenFileWithKey,
                             textEditTabWidget, &TextEditTabWidget::openFileWithKey);
            QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toJumpFileLineWithKey,
                             textEditTabWidget, &TextEditTabWidget::jumpToLineWithKey);
            QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toRunFileLineWithKey,
                             textEditTabWidget, &TextEditTabWidget::runningToLineWithKey);
            QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toSetModifiedAutoReload,
                             textEditTabWidget, &TextEditTabWidget::setModifiedAutoReload);
            tabWidgets[textEditTabWidget] = state;
        }
    }
}

void TextEditSplitter::doShowSplit()
{
    if (tabWidgets.size() > 1)
        return;
    auto textEditTabWidget = qobject_cast<TextEditTabWidget*>(sender());
    textEditTabWidget->setSplitButtonVisible(true);
}

TextEditSplitter *TextEditSplitter::instance()
{
    if (!Private::splitter)
        Private::splitter = new TextEditSplitter;
    return Private::splitter;
}

void TextEditSplitter::doClose()
{
    auto textEditTabWidget = qobject_cast<TextEditTabWidget*>(sender());
    int count = tabWidgets.size();
    if (count == 1) {
        textEditTabWidget->setCloseButtonVisible(false);
        textEditTabWidget->setSplitButtonVisible(false);
        return;
    }
    if (tabWidgets[textEditTabWidget]) {
        auto it = tabWidgets.begin();
        if (it.key() == textEditTabWidget)
            ++it;
        it.value() = true;
        QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toAddDebugPoint,
                         textEditTabWidget, &TextEditTabWidget::addDebugPoint);
        QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toRemoveDebugPoint,
                         textEditTabWidget, &TextEditTabWidget::removeDebugPoint);
        QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toOpenFileWithKey,
                         it.key(), &TextEditTabWidget::openFileWithKey);
        QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toJumpFileLineWithKey,
                         it.key(), &TextEditTabWidget::jumpToLineWithKey);
        QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toRunFileLineWithKey,
                         it.key(), &TextEditTabWidget::runningToLineWithKey);
        QObject::connect(EditorCallProxy::instance(), &EditorCallProxy::toSetModifiedAutoReload,
                         it.key(), &TextEditTabWidget::setModifiedAutoReload);
    }

    tabWidgets.remove(textEditTabWidget);
    count -= 1;
    if (count < 4) {
        for (auto it = tabWidgets.begin(); it != tabWidgets.end(); ++it) {
            it.key()->setSplitButtonVisible(true);
        }
    }

    auto splitter = qobject_cast<QSplitter *>(textEditTabWidget->parent());
    delete textEditTabWidget;
    if (splitter->count() == 0)
        delete splitter;

    if (tabWidgets.size() == 1) {
        auto it = tabWidgets.begin();
        it.key()->setCloseButtonVisible(false);
    }
}

