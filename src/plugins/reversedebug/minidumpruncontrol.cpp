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
#include "minidumpruncontrol.h"
#include "reversedebuggerconstants.h"

#include <QDebug>
#include <QFile>
#include <QMessageBox>

#include <string>
#include <unistd.h>

using namespace std;

// global variable used extern.
bool g_emd_running = false;
bool g_emd_buffer_syscall = false;
QString g_emd_params;

namespace ReverseDebugger {
namespace Internal {

string found_crash(const char *subdir, int *ppid)
{
    // find crash.txt
    string parent_dir = getenv("HOME");
    parent_dir += subdir;   // "/.local/share/rdb/";
    string linkname = parent_dir;
    linkname += "latest-trace";
    string filename = parent_dir;
    int pos = filename.size();
    filename.resize(512, 0);
    int len = readlink(linkname.data(),
                       (char *)filename.data() + pos, 512 - pos);
    if (len < 0) {
        return string();
    }
    filename.resize(len + pos);
    parent_dir = filename;
    filename += "/crash.txt";

    // parse crash.txt
    QFile file(QString::fromStdString(filename));
    if (file.size() > 0 && file.open(QFile::ReadOnly)) {
        char buf[256];
        int size = file.readLine(buf, sizeof(buf));
        buf[size] = 0;
        char *stop = nullptr;
        int pid = strtol(buf, &stop, 10);
        int sig = strtol(stop + 1, NULL, 10);
        if (sig > 0 && pid > 0) {
            qDebug() << __FUNCTION__ << "found crash sig: " << sig;
            *ppid = pid;
            return parent_dir;
        }
    }

    return string();
}

MinidumpRunControl::MinidumpRunControl(QObject *parent)
    : QObject(parent),
      process(new QProcess(this))
{
    if (g_emd_running) {
        qDebug() << "emd is running now!";
        return;
    }

    //    process->setWorkingDirectory()
    //    process->setProcessEnvironment();

    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(onStraceExit(int, QProcess::ExitStatus)));

    execFile = ReverseDebugger::Constants::ST_PATH;
    // TODO(mozart):replay params with dialog config.
    g_emd_params = "--sys=desc, --x11=2, --stack-size=32 --heap-size=0 --param-size=256 -1";
    if (g_emd_params.size() > 0) {
        execFile += ' ' + g_emd_params + ' ';
    }
    execFile += "";   // target debuggee
}

MinidumpRunControl::~MinidumpRunControl()
{
}

void MinidumpRunControl::start()
{
    qDebug() << __FUNCTION__ << ", object:" << this;

    appendMessage(tr("[Start] %1").arg(execFile) + QLatin1Char('\n'));

    process->start(execFile);
    if (!process->waitForStarted(1000)) {
        process = nullptr;
        qDebug() << "Failed to run emd";
        return;
    }

    g_emd_running = true;
}

StopResult MinidumpRunControl::stop()
{
    qDebug() << __FUNCTION__ << ", object:" << this;

    if (process) {
        QByteArray data = process->readAll();
        QString outstr = QString::fromLocal8Bit(data.data());
        appendMessage(outstr + QLatin1Char('\n'));
        process = nullptr;
    }

    g_emd_running = false;

    appendMessage(tr("[Stop] %1").arg(execFile) + QLatin1Char('\n'));

    return StoppedSynchronously;
}

bool MinidumpRunControl::isRunning() const
{
    return g_emd_running;
}

QString MinidumpRunControl::displayName() const
{
    return "event debug recored";
}

void MinidumpRunControl::appendMessage(const QString &msg)
{
    Q_UNUSED(msg)
}

void MinidumpRunControl::onStraceExit(int, QProcess::ExitStatus)
{
    stop();

    int pid = 0;
    string parent_dir = found_crash(("/.local/share/emd/"), &pid);
    if (!parent_dir.empty()) {
        //        emd_replay(QString::fromStdString(parent_dir), pid);
        return;
    }

    QMessageBox::information(nullptr, tr("reverse debug"), tr("Recored done, minidump load ready."));
}

}   // namespace Internal
}   // namespace ReverseDebugger