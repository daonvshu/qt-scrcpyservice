#include "adbcommandrunner.h"

#include <qdebug.h>

AdbCommandRunner::AdbCommandRunner(const QString &deviceName)
    : deviceName(deviceName)
{}

AdbCommandRunner::~AdbCommandRunner()  {
    if (process.isOpen()) {
        process.kill();
        process.waitForFinished();
    }
}

void AdbCommandRunner::runAdb(const QStringList &cmds, bool waitForFinished) {
    if (deviceName.isEmpty()) {
        process.start("adb/adb", cmds);
    } else {
        process.start("adb/adb", QStringList({"-s", deviceName}) + cmds);
    }
    qDebug() << "do adb execute command:" << "adb " + cmds.join(' ');
    if (waitForFinished) {
        process.waitForFinished();
    }
    lastFeedback = process.readAllStandardOutput();
}

void AdbCommandRunner::runAapt(const QStringList &cmds) {
    process.start("adb/aapt", cmds);
    process.waitForFinished();
    lastFeedback = process.readAllStandardOutput();
}

QString AdbCommandRunner::getLastErr() {
    QString failReason = process.readAllStandardError();
    if (failReason.isEmpty()) {
        failReason = lastFeedback;
    }
    return failReason;
}

