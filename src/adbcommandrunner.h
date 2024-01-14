#pragma once

#include <qobject.h>
#include <qprocess.h>

/**
 * @brief Adb命令执行封装类
 */
class AdbCommandRunner {
public:
    explicit AdbCommandRunner(const QString& deviceName = QString());

    ~AdbCommandRunner();

    /**
     * @brief 执行Adb命令
     * @param cmds 参数列表
     * @param waitForFinished 是否等待执行完成
     */
    void runAdb(const QStringList& cmds, bool waitForFinished = true);

    /**
     * @brief 执行Aapt命令
     * @param cmds
     */
    void runAapt(const QStringList& cmds);

    /**
     * @brief 获取执行结果的错误
     * @return
     */
    QString getLastErr();

    QString lastFeedback; //执行结果返回的字符串

private:
    QProcess process;
    QString deviceName;
};