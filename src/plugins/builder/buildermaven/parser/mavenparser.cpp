/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhouyi<zhouyi1@uniontech.com>
 *
 * Maintainer: zhouyi<zhouyi1@uniontech.com>
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
#include "mavenparser.h"

#include "services/builder/task.h"
#include "services/builder/fileutils.h"

const char TASK_CATEGORY_BUILDSYSTEM[] = "Task.Category.Buildsystem";

MavenParser::MavenParser()
{
    setObjectName(QLatin1String("MavenParser"));
}

void MavenParser::stdOutput(const QString &line, OutputFormat format)
{
    QString newContent = line;
    QRegExp exp("\\033\\[(\\d*;*\\d*)m");
    newContent.replace(exp, "");

    if (newContent.indexOf("[ERROR]") != -1) {
        format = OutputFormat::ErrorMessage;
        stdError(newContent);
    }

    emit outputAdded(newContent, format);

    IOutputParser::stdOutput(newContent, format);
}

void MavenParser::stdError(const QString &line)
{
    taskAdded(Task(Task::Error,
                   line,
                   Utils::FileName::fromUserInput(""),
                   0,
                   TASK_CATEGORY_BUILDSYSTEM), 1, 0);
}

void MavenParser::taskAdded(const Task &task, int linkedLines, int skippedLines)
{
    emit addTask(task, linkedLines, skippedLines);
}