﻿#include "cmakegenerator.h"
#include <QtXml>
#include <QFileIconProvider>

namespace  {
enum_def(ProjectKit, QString)
{
    enum_exp CDT = "Eclipse CDT4 - Unix Makefiles";
    enum_exp CDT_PROJECT = ".project";
    enum_exp CDT_CPROJECT = ".cproject";
};

enum_def(CDT_XML_KEY, QString)
{
    enum_exp projectDescription = "projectDescription";
    enum_exp name = "name";
    enum_exp comment = "comment";
    enum_exp project = "project";
    enum_exp buildSpec = "buildSpec";
    enum_exp buildCommand = "buildCommand";
    enum_exp triggers = "triggers";
    enum_exp arguments = "arguments";
    enum_exp dictionary = "dictionary";
    enum_exp link = "link";
    enum_exp type = "type";
    enum_exp location = "location";
    enum_exp locationURI = "locationURI";
    enum_exp key = "key";
    enum_exp value = "value";
    enum_exp natures = "natures";
    enum_exp linkedResources = "linkedResources";
};

enum_def(CDT_TARGETS_TYPE, QString)
{
    enum_exp Subprojects = "[Subprojects]";
    enum_exp Targets = "[Targets]";
    enum_exp Lib = "[lib]";
    enum_exp Exe = "[exe]";
};

enum_def(CDT_FILES_TYPE, QString)
{
    enum_exp Unknown = "//";
    enum_exp ObjectLibraries = "/Object Libraries/";
    enum_exp ObjectFiles = "/Object Files/";
    enum_exp SourceFiles = "/Source Files/";
    enum_exp HeaderFiles = "/Header Files/";
    enum_exp CMakeRules = "/CMake Rules/";
    enum_exp Resources = "/Resources/";
};

static int currentCount = 0;
static int maxCount = 100;
static QFileIconProvider iconProvider;
}

CMakeGenerator::CMakeGenerator()
{

}

QStandardItem *CMakeGenerator::createRootItem(const QString &projectPath)
{
    Generator::started();
    currentCount = 0;
    maxCount = 100;

    process.setProgram("cmake");

    QStringList arguments;
    arguments << "-S";
    arguments << QFileInfo(projectPath).path();
    arguments << "-B";
    arguments << cmakeBuildPath(projectPath);
    arguments << "-G";
    arguments << ProjectKit::get()->CDT;
    arguments << "-DCMAKE_EXPORT_COMPILE_COMMANDS=1";
    arguments << "-DCMAKE_BUILD_TYPE=Debug";
    process.setArguments(arguments);

    // 消息和進度轉發
    QObject::connect(&process, &QProcess::readyRead,
                     this, &CMakeGenerator::processReadAll, Qt::UniqueConnection);

    QObject::connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     this , &CMakeGenerator::processFinished, Qt::UniqueConnection);
    process.start();

    // step.1 cmake generator CDT4 project and compile json file
    if (process.exitCode() != 0 || QProcess::ExitStatus::NormalExit) {
        Generator::setErrorString(process.errorString());
        Generator::finished(false);
        return nullptr; // nullptr root item
    }

    // step.2 read project from CDT4 xml file;
    QStandardItem * rootItem = nullptr;
    auto projectXmlDoc = loadXmlDoc(projectPath);
    QDomElement docElem = projectXmlDoc.documentElement();
    QDomNode n = docElem.firstChild();
    while(!n.isNull()) {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if(!e.isNull()) {
            if (!rootItem && e.tagName() == CDT_XML_KEY::get()->name) {
                rootItem = new QStandardItem();
                rootItem->setText(e.text());
                rootItem->setToolTip(projectPath);
            }
            if (rootItem && e.tagName() == CDT_XML_KEY::get()->linkedResources) {
                QDomNode linkedResChildNode = e.firstChild();
                while (!linkedResChildNode.isNull()) {
                    QDomElement linkElem = linkedResChildNode.toElement();
                    if (linkElem.tagName() == CDT_XML_KEY::get()->link) {
                        QDomNode linkChildNode = linkElem.firstChild();
                        QStandardItem *childItem = nullptr;
                        while (!linkChildNode.isNull()) {
                            QDomElement linkChildElem = linkChildNode.toElement();
                            if (!childItem && linkChildElem.tagName() == CDT_XML_KEY::get()->name) {
                                QString name = linkChildElem.text();
                                auto parentItem = cmakeCDT4FindParentItem(rootItem, name); // 查找上级节点
                                if (!name.isEmpty()) { // 节点为有效数据 避免 dir/ 和dir解析歧义生成空节点
                                    childItem = new QStandardItem(); // 创建子节点
                                    childItem->setText(name); // 设置子节点名称
                                    if (parentItem) {
                                        parentItem->appendRow(childItem);
                                    }
                                }
                            }
                            if (childItem && linkChildElem.tagName() == CDT_XML_KEY::get()->location) {
                                childItem->setToolTip(linkChildElem.text());
                                childItem->setIcon(iconProvider.icon(QFileIconProvider::File));
                            }
                            if (childItem && linkChildElem.tagName() == CDT_XML_KEY::get()->locationURI) {
                                childItem->setToolTip(linkChildElem.text());
                            }
                            // 节点缓存来自xml的所有数据
                            ProjectGenerator::setToolKitProperty(childItem, linkChildElem.tagName(), linkChildElem.text());
                            linkChildNode = linkChildNode.nextSibling();
                        }
                    }
                    linkedResChildNode = linkedResChildNode.nextSibling();
                }
            }
        }
        n = n.nextSibling();
    }
    return cmakeCDT4DisplayOptimize(rootItem);
}

QMenu *CMakeGenerator::createIndexMenu(const QModelIndex &index)
{
    Q_UNUSED(index);
    return nullptr;
}

void CMakeGenerator::processReadAll()
{
    QString mess = process.readAll();
    currentCount += 10;
    qInfo() << mess;
    message({mess, currentCount, maxCount});
}

void CMakeGenerator::processFinished(int code, QProcess::ExitStatus status)
{
    QString mess = process.readAll();
    QMetaEnum mateEnum = QMetaEnum::fromType<QProcess::ExitStatus>();
    mess += "\n";
    mess += QString("return code: %0, return Status: %1").arg(code).arg(mateEnum.key(status));
    qInfo() << mess;
    message({mess, maxCount, maxCount});
}

QStandardItem *CMakeGenerator::cmakeCDT4FindParentItem(QStandardItem *rootItem, QString &name)
{
    if (!rootItem) {
        return nullptr;
    }
    for (int row = 0; row < rootItem->rowCount(); row ++) {
        QStandardItem *childItem = rootItem->child(row);
        QString childDisplayText = childItem->data(Qt::DisplayRole).toString();
        if (name.startsWith(childDisplayText + "/")) {
            name = name.replace(childDisplayText + "/", "");
            return cmakeCDT4FindParentItem(childItem, name);
        }
    }
    return rootItem;
}

QStandardItem *CMakeGenerator::cmakeCDT4FindItem(QStandardItem *rootItem, QString &name)
{
    if (!rootItem) {
        return nullptr;
    }

    QStandardItem *parentItem = cmakeCDT4FindParentItem(rootItem, name);
    if (parentItem) {
        for (int row = 0; row < parentItem->rowCount(); row ++) {
            QStandardItem * childItem = parentItem->child(row);
            qInfo() << parentItem->data(Qt::DisplayRole) << childItem->data(Qt::DisplayRole);
            if (childItem->data(Qt::DisplayRole) == name) {
                name = name.replace(childItem->data(Qt::DisplayRole).toString(), "");
                return childItem;
            }
        }
    }
    return parentItem;
}

QHash<QString, QString> CMakeGenerator::cmakeCDT4Subporjects(QStandardItem *rootItem)
{
    QString subprojectsKey = CDT_TARGETS_TYPE::get()->Subprojects;
    QStandardItem * subprojectsItem = cmakeCDT4FindItem(rootItem, subprojectsKey);
    QHash<QString, QString> subprojectHash;
    for (int row = 0; row < subprojectsItem->rowCount(); row ++) {
        auto name = ProjectGenerator::toolKitProperty(subprojectsItem->child(row), CDT_XML_KEY::get()->name).toString();
        auto location = ProjectGenerator::toolKitProperty(subprojectsItem->child(row), CDT_XML_KEY::get()->location).toString();
        subprojectHash[name] = location;
    }
    return subprojectHash;
}

QStandardItem *CMakeGenerator::cmakeCDT4DisplayOptimize(QStandardItem *rootItem)
{
    if (!rootItem) {
        return nullptr;
    }
    QString targetsName = CDT_TARGETS_TYPE::get()->Targets;
    QStandardItem *targetsItem = cmakeCDT4FindItem(rootItem, targetsName); //targets 顶层item
    QHash<QString, QString> subprojectsMap = cmakeCDT4Subporjects(rootItem);
    cmakeCDT4TargetsDisplayOptimize(targetsItem, subprojectsMap);
    return rootItem;
}

void CMakeGenerator::cmakeCDT4TargetsDisplayOptimize(QStandardItem *item, const QHash<QString, QString> &subprojectsMap)
{
    QStandardItem * addRows = new QStandardItem("Temp");
    for (int row = 0; row < item->rowCount(); row ++) {
        QStandardItem *childItem = item->child(row);
        if (childItem->hasChildren())
            cmakeCDT4TargetsDisplayOptimize(item->child(row), subprojectsMap);
        else {
            QVariantMap map = ProjectGenerator::toolKitPropertyMap(childItem); //当前节点特性
            if (map.keys().contains(CDT_XML_KEY::get()->location)) { // 本地文件
                for (auto val : subprojectsMap.values()) {
                    QString childLocation = map.value(CDT_XML_KEY::get()->location).toString();
                    qInfo() << "childLocation:" << childLocation;
                    QString childFileName = childItem->data(Qt::DisplayRole).toString();
                    // 获取中间需要展示的文件夹
                    if (!val.isEmpty() && childLocation.startsWith(val)) {
                        QString suffixPath = childLocation.replace(val + "/","");
                        if (suffixPath.endsWith("/" + childFileName)) {
                            suffixPath = suffixPath.remove(suffixPath.size() - childFileName.size() - 1,
                                                           childFileName.size() + 1);
                            // 获取当前是否已经新建文件夹
                            QStandardItem *findNewItem = cmakeCDT4FindItem(addRows, suffixPath);
                            if (!suffixPath.isEmpty()) { // 新建子文件夹
                                QIcon icon = iconProvider.icon(QFileIconProvider::Folder);
                                auto newChild = new QStandardItem(icon, suffixPath);
                                findNewItem->insertRow(0, newChild); //置顶文件夹
                                findNewItem = newChild;
                            }
                            // 当前子节点移动到找到的节点下
                            qInfo() << item->rowCount();
                            findNewItem->appendRow(item->takeRow(row));
                            row --; //takeRow自动删除一行，此处屏蔽差异
                            qInfo() << item->rowCount();
                            qInfo() << findNewItem->data(Qt::DisplayRole).toString();
                            qInfo() << findNewItem->parent()->data(Qt::DisplayRole).toString();
                        }
                    }
                }
            }
        }
    }
    // 新增项
    while (addRows->hasChildren()) {
        QStandardItem *addRowItem = addRows->takeRow(0).first();
        qInfo() << addRowItem->data(Qt::DisplayRole).toString();
        item->appendRow(addRowItem);
    }
    delete addRows;
}

QDomDocument CMakeGenerator::loadXmlDoc(const QString &cmakePath)
{
    QDomDocument xmlDoc;
    QString cdtProjectFile = cmakeCDT4FilePath(cmakePath) + QDir::separator()
            + ProjectKit::get()->CDT_PROJECT;
    QFile docFile(cdtProjectFile);

    if (!docFile.exists()) {
        Generator::setErrorString("Failed, cdtProjectFile not exists!: " + cdtProjectFile);
        Generator::finished(false);
        return xmlDoc;
    }

    if (!docFile.open(QFile::OpenModeFlag::ReadOnly)) {
        Generator::setErrorString(docFile.errorString());
        Generator::finished(false);
        return xmlDoc;
    }

    if (!xmlDoc.setContent(&docFile)) {
        docFile.close();
        return xmlDoc;
    }
    docFile.close();
    return xmlDoc;
}

QString CMakeGenerator::cmakeCDT4FilePath(const QString &cmakePath)
{
    return cmakeBuildPath(cmakePath);
}

QString CMakeGenerator::cmakeBuildPath(const QString &cmakePath)
{
    return QFileInfo(cmakePath).path() + QDir::separator() + "build";
}