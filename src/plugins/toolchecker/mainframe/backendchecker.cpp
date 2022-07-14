#include "backendchecker.h"
#include "wgetdialog.h"

#include "common/common.h"

#include <QDirIterator>
#include <QMimeDatabase>

#define JDTLS_PACKAGE_URL "https://download.eclipse.org/jdtls/snapshots/jdt-language-server-1.10.0-202203040350.tar.gz"
#define JDTLS_PACKAGE_NAME "jdt-language-server.tar.gz"
#define JDTLS_CHECKFILE_URL "https://download.eclipse.org/jdtls/snapshots/jdt-language-server-1.10.0-202203040350.tar.gz.sha256"
#define JDTLS_CHECKFILE_NAME "jdt-language-server.tar.gz.sha256"
#define JDTLS_CHECKPROGRAM_NAME "shasum"
#define JDTLS_CHECKPROGRAM_MODE "256"

#define JDTLS_PROGRAM_NAME "jdtls"
#define JDTLS_PROGRAM_MIME "text/x-python3"

BackendChecker::BackendChecker(QWidget *parent)
    : QWidget(parent)
{
    RequestInfo info;
    info.setPackageUrl(QUrl(JDTLS_PACKAGE_URL));
    info.setPackageSaveName(JDTLS_PACKAGE_NAME);
    info.setCheckFileUrl(QUrl(JDTLS_CHECKFILE_URL));
    info.setCheckFileSaveName(JDTLS_CHECKFILE_NAME);
    info.setCheckNumProgram(JDTLS_CHECKPROGRAM_NAME);
    info.setCheckNumMode(JDTLS_CHECKPROGRAM_MODE);

    requestInfos["Java"] = info;

    QDir dir = QDir::home();
    if (!dir.cd(".config")) { dir.mkdir(".config"); }
    if (!dir.cd("languageadapter")) { dir.mkdir("languageadapter"); }
    adapterPath = dir.path();

    auto itera = requestInfos.begin();
    while (itera != requestInfos.end()) {
        if (!dir.cd(itera.key())) {
            dir.mkdir(itera.key());
        }
        dir.cdUp();
        itera ++;
    }

    for (auto languageID : requestInfos.keys()) {
        auto info = requestInfos.value(languageID);
        if (!exists(languageID)) {
            if (!checkCachePackage(languageID)) { // Sha256 校验
                QStringList args = { info.getPackageUrl().toEncoded(),
                                     "-O",
                                     info.getPackageSaveName() };
                auto dialog =  new WGetDialog;
                dialog->setWorkDirectory(adapterPath);
                dialog->setWgetArguments(args);
                dialog->exec();
            }

            auto processDialog = new ProcessDialog();
            processDialog->setWorkDirectory(adapterPath);
            processDialog->setProgram("tar");
            processDialog->setArguments({"zxvf", info.getPackageSaveName(), "-C", languageID});
            processDialog->exec();
        } else {

        }
    };
}

bool BackendChecker::exists(const QString &languageID)
{
    if (languageID == "Java") {
        QDir dir(adapterPath + QDir::separator() + languageID);
        qInfo() << dir;
        dir.setFilter(QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Files | QDir::Dirs);
        QDirIterator itera(dir, QDirIterator::Subdirectories);
        QMimeDatabase mimeDB;
        while (itera.hasNext()) {
            itera.next();
            if (JDTLS_PROGRAM_MIME == mimeDB.mimeTypeForFile(itera.fileInfo()).name()) {
                return true;
            }
        }
    }
    return false;
}

bool BackendChecker::checkCachePackage(const QString &languageID)
{

    auto info = requestInfos.value(languageID);

    QDir adapterDir(adapterPath);
    if (!adapterDir.exists(info.getPackageSaveName()))
        return false;

    if (adapterDir.exists(info.getCheckFileSaveName())) {
        adapterDir.remove(info.getCheckFileSaveName());
    }

    // current cache package path
    QString localPackageName = info.getPackageSaveName();

    // download sha256
    QStringList args = { info.getCheckFileUrl().toEncoded(),
                         "-O",
                         info.getCheckFileSaveName() };

    QProcess wgetCheckFileProcess;
    wgetCheckFileProcess.setWorkingDirectory(adapterPath);
    wgetCheckFileProcess.setProgram("wget");
    wgetCheckFileProcess.setArguments({ info.getCheckFileUrl().toEncoded(),
                                        "-O",
                                        info.getCheckFileSaveName()});
    wgetCheckFileProcess.start();
    wgetCheckFileProcess.waitForFinished();

    QProcess checkProcess;
    checkProcess.setWorkingDirectory(adapterPath);
    checkProcess.setProgram(info.getCheckNumProgram());
    checkProcess.setArguments({"-a", info.getCheckNumMode(), info.getPackageSaveName()});
    checkProcess.start();
    checkProcess.waitForFinished();

    QString output = checkProcess.readAll();
    QStringList result = output.split(" ");
    if (result.size() >= 2) {
        output = result.first();
    }

    QFile checkFile(adapterDir.filePath(info.getCheckFileSaveName()));
    if (checkFile.open(QFile::OpenModeFlag::ReadOnly)) {
        QString readOut = checkFile.readAll();
        if (readOut == output)
            return true;
    }

    return false;
}

QString RequestInfo::getPackageSaveName() const
{
    return packageSaveName;
}

QUrl RequestInfo::getCheckFileUrl() const
{
    return checkFileUrl;
}

QString RequestInfo::getCheckFileSaveName() const
{
    return checkFileSaveName;
}

QString RequestInfo::getCheckNumProgram() const
{
    return checkNumProgram;
}

QString RequestInfo::getCheckNumMode() const
{
    return checkNumMode;
}

void RequestInfo::setPackageUrl(const QUrl &value)
{
    packageUrl = value;
}

void RequestInfo::setPackageSaveName(const QString &value)
{
    packageSaveName = value;
}

void RequestInfo::setCheckFileUrl(const QUrl &value)
{
    checkFileUrl = value;
}

void RequestInfo::setCheckFileSaveName(const QString &value)
{
    checkFileSaveName = value;
}

void RequestInfo::setCheckNumProgram(const QString &value)
{
    checkNumProgram = value;
}

void RequestInfo::setCheckNumMode(const QString &value)
{
    checkNumMode = value;
}

QUrl RequestInfo::getPackageUrl() const
{
    return packageUrl;
}