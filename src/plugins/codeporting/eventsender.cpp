/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     luzhen<luzhen@uniontech.com>
 *
 * Maintainer: zhengyouge<zhengyouge@uniontech.com>
 *             luzhen<luzhen@uniontech.com>
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
#include "eventsender.h"
#include "framework.h"
#include "common/common.h"

void EventSender::jumpTo(const QString &filePath, int lineNum)
{
    if (filePath.isEmpty() || lineNum < 0)
        return;

    dpf::Event event;
    event.setTopic(T_CODEEDITOR);
    event.setData(D_JUMP_TO_LINE);
    event.setProperty(P_FILEPATH, filePath);
    event.setProperty(P_FILELINE, lineNum);
    dpf::EventCallProxy::instance().pubEvent(event);
}

EventSender::EventSender(QObject *parent) : QObject(parent)
{

}