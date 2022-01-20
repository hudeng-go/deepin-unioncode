/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     luzhen<luzhen@uniontech.com>
 *
 * Maintainer: luzhen<luzhen@uniontech.com>
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
#ifndef EVENTRECEIVER_H
#define EVENTRECEIVER_H

#include <framework/framework.h>
#include <QObject>

class EventReceiver : public dpf::EventHandler, dpf::AutoEventHandlerRegister<EventReceiver>
{
    friend class dpf::AutoEventHandlerRegister<EventReceiver>;

public:
    explicit EventReceiver(QObject *parent = nullptr);
    static Type type();
    static QStringList &topics();

signals:

public slots:

private:
    virtual void eventProcess(const dpf::Event &event) override;
};

#endif   // EVENTRECEIVER_H