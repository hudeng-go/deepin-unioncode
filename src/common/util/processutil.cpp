/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     huangyu<huangyub@uniontech.com>
 *
 * Maintainer: huangyu<huangyub@uniontech.com>
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
#include "processutil.h"

#include <QProcess>
#include <QFileInfo>
#include <QDirIterator>
#include <QDebug>
#include <QUrl>

bool ProcessUtil::execute(const QString &program,
                          const QStringList &arguments,
                          ProcessUtil::ReadCallBack func,
                          int cmpExitCode)
{
    bool ret = false;
    QProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    process.connect(&process, QOverload<int, QProcess::ExitStatus >::of(&QProcess::finished),
                    [&ret, &cmpExitCode](int exitCode, QProcess::ExitStatus exitStatus){
        if (exitCode == cmpExitCode && exitStatus == QProcess::NormalExit)
            ret = true;
    });

    process.start();
    process.waitForFinished();

    if (func) {
        func(process.readAll());
    }

    if (!ret) {
        qCritical() << process.errorString();
    }
    return ret;
}

bool ProcessUtil::exists(const QString &name)
{
    bool ret = false;
#ifdef linux
    auto outCallback = [&ret, &name](const QByteArray &array) {
        QList<QByteArray> rmSearch = array.split(' ');
        foreach (QByteArray rmProcess, rmSearch) {
            QFileInfo info(rmProcess);
            if (info.fileName() == name && info.isExecutable()) {
                if (!ret)
                    ret = true;
            }
        }
    };
    ret = ProcessUtil::execute("whereis", {name}, outCallback);
#else
#endif
    return ret;
}

QString ProcessUtil::version(const QString &name)
{
    QString retOut;
#ifdef linux
    auto outCallback = [&retOut](const QByteArray &array) {
        retOut = QString ::fromLatin1(array);
    };
    ProcessUtil::execute(name, {"-version"}, outCallback);
#else
#endif
    return retOut;
}

bool ProcessUtil::hasGio()
{
    return exists("gio");
}

bool ProcessUtil::moveToTrash(const QString &filePath)
{
#ifdef linux
    if (!hasGio())
        return false;
    return ProcessUtil::execute("gio", {"trash", filePath});
#else

#endif
}

bool ProcessUtil::recoverFromTrash(const QString &filePath)
{
#ifdef linux
    if (!hasGio() || filePath.isEmpty())
        return false;

    QDirIterator itera(QDir::homePath() + QDir::separator() + ".local/share/Trash/files");
    while (itera.hasNext()){
        itera.next();
        QFileInfo info(filePath);
        if(info.suffix() == itera.fileInfo().suffix()
                && info.baseName() == itera.fileInfo().baseName()) {
            QByteArray readArray;
            auto readCB = [&](const QByteArray array){readArray = array;};
            QString trashFileUri = QString("trash:///%0").arg(info.fileName());
            QStringList queryArgs;
            queryArgs << "info";
            queryArgs << trashFileUri;
            queryArgs << "| grep trash::orig-path";
            bool execResult = ProcessUtil::execute("gio", queryArgs, readCB);
            if (!execResult && readArray.isEmpty()) {
                qCritical() << "Unknown Error";
                abort();
            }
            readArray = readArray.replace(" ", "");
            auto list = readArray.split('\n');
            auto itera = list.rbegin();
            while (itera != list.rend()) {
                if (itera->startsWith("trash::orig-path:")) {
                    readArray = *itera;
                    break;
                }
                itera ++;
            }
            if (!readArray.startsWith("trash::orig-path:")) {
                qCritical() << "Error from: " <<QString::fromUtf8(readArray);
                abort();
            }
            readArray = readArray.replace("trash::orig-path:", "");
            if (readArray == filePath) {
                QStringList args;
                args << "move";
                args << trashFileUri;
                args << QUrl::fromLocalFile(filePath).toString();
                return ProcessUtil::execute("gio", args);
            }
        }
    }

#else

#endif
    return false;
}
