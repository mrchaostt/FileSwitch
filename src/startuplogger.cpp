#include "startuplogger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>
#include <QStringList>
#include <QSysInfo>
#include <QThread>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <cstdlib>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {

QMutex s_logMutex;
QFile *s_logFile = nullptr;
QString s_logFilePath;
QtMessageHandler s_previousMessageHandler = nullptr;

QString timestamp()
{
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
}

QString threadId()
{
    return QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()), 16);
}

void writeLine(const QString &level, const QString &message)
{
    QMutexLocker locker(&s_logMutex);
    if (!s_logFile || !s_logFile->isOpen()) {
        return;
    }

    const QString line = QStringLiteral("%1 [%2] [tid:%3] %4\n")
                             .arg(timestamp(), level, threadId(), message);
    s_logFile->write(line.toUtf8());
    s_logFile->flush();
}

bool openLogFile(const QString &path)
{
    if (path.isEmpty()) {
        return false;
    }

    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile *file = new QFile(path);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        delete file;
        return false;
    }

    s_logFile = file;
    s_logFilePath = QFileInfo(path).absoluteFilePath();
    return true;
}

#ifdef Q_OS_WIN
QString windowsExecutablePath()
{
    DWORD bufferSize = MAX_PATH;
    QString result;

    while (bufferSize <= 32768) {
        QString buffer(bufferSize, QChar(0));
        const DWORD copied = GetModuleFileNameW(nullptr, reinterpret_cast<wchar_t *>(buffer.data()), bufferSize);
        if (copied == 0) {
            return QString();
        }

        if (copied < bufferSize - 1) {
            buffer.truncate(static_cast<int>(copied));
            result = buffer;
            break;
        }

        bufferSize *= 2;
    }

    return result;
}

QString exceptionCodeToString(DWORD code)
{
    return QStringLiteral("0x%1").arg(static_cast<qulonglong>(code), 8, 16, QChar('0'));
}

LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS *exceptionInfo)
{
    DWORD code = 0;
    quintptr address = 0;

    if (exceptionInfo && exceptionInfo->ExceptionRecord) {
        code = exceptionInfo->ExceptionRecord->ExceptionCode;
        address = reinterpret_cast<quintptr>(exceptionInfo->ExceptionRecord->ExceptionAddress);
    }

    writeLine(QStringLiteral("FATAL"),
              QStringLiteral("Unhandled Windows exception: code=%1 address=0x%2")
                  .arg(exceptionCodeToString(code))
                  .arg(static_cast<qulonglong>(address), 0, 16));

    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

QStringList logPathCandidates(int argc, char *argv[])
{
    QStringList candidates;

#ifdef Q_OS_WIN
    const QString exePath = windowsExecutablePath();
    if (!exePath.isEmpty()) {
        candidates << QFileInfo(exePath).dir().filePath(QStringLiteral("FileSwitch-startup.log"));
    }
#endif

    if (argc > 0 && argv && argv[0]) {
        const QFileInfo argv0(QString::fromLocal8Bit(argv[0]));
        candidates << argv0.absoluteDir().filePath(QStringLiteral("FileSwitch-startup.log"));
    }

    candidates << QDir::current().filePath(QStringLiteral("FileSwitch-startup.log"));
    candidates << QDir(QDir::tempPath()).filePath(QStringLiteral("FileSwitch-startup.log"));

    QStringList unique;
    QSet<QString> seen;
    for (const QString &candidate : candidates) {
        const QString cleaned = QDir::cleanPath(candidate);
        if (!cleaned.isEmpty() && !seen.contains(cleaned)) {
            seen.insert(cleaned);
            unique << cleaned;
        }
    }

    return unique;
}

QString messageTypeName(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return QStringLiteral("DEBUG");
    case QtInfoMsg:
        return QStringLiteral("INFO");
    case QtWarningMsg:
        return QStringLiteral("WARN");
    case QtCriticalMsg:
        return QStringLiteral("ERROR");
    case QtFatalMsg:
        return QStringLiteral("FATAL");
    }

    return QStringLiteral("LOG");
}

void qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QString contextText;
    if (context.file) {
        contextText = QStringLiteral(" (%1:%2")
                          .arg(QFileInfo(QString::fromUtf8(context.file)).fileName())
                          .arg(context.line);
        if (context.function) {
            contextText += QStringLiteral(", ");
            contextText += QString::fromUtf8(context.function);
        }
        contextText += QLatin1Char(')');
    }

    writeLine(messageTypeName(type), message + contextText);

    if (s_previousMessageHandler) {
        s_previousMessageHandler(type, context, message);
    }
}

} // namespace

namespace StartupLogger {

void initialize(int argc, char *argv[])
{
    if (s_logFile && s_logFile->isOpen()) {
        return;
    }

    for (const QString &candidate : logPathCandidates(argc, argv)) {
        if (openLogFile(candidate)) {
            break;
        }
    }

    info(QStringLiteral("========== FileSwitch startup =========="));
    info(QStringLiteral("Log file: %1").arg(s_logFilePath.isEmpty() ? QStringLiteral("<unavailable>") : s_logFilePath));
    info(QStringLiteral("Working directory: %1").arg(QDir::currentPath()));

    QStringList arguments;
    for (int i = 0; i < argc; ++i) {
        arguments << QString::fromLocal8Bit(argv[i]);
    }
    info(QStringLiteral("Arguments: %1").arg(arguments.join(QStringLiteral(" | "))));

    info(QStringLiteral("Qt runtime: %1; Qt build headers: %2").arg(QString::fromLatin1(qVersion()), QString::fromLatin1(QT_VERSION_STR)));
    info(QStringLiteral("Build CPU: %1; current CPU: %2").arg(QSysInfo::buildCpuArchitecture(), QSysInfo::currentCpuArchitecture()));
    info(QStringLiteral("Kernel: %1 %2").arg(QSysInfo::kernelType(), QSysInfo::kernelVersion()));

#ifdef QT_NO_DEBUG
    info(QStringLiteral("Build type: Release"));
#else
    info(QStringLiteral("Build type: Debug"));
#endif

#ifdef Q_OS_WIN
    info(QStringLiteral("Executable path: %1").arg(windowsExecutablePath()));
    const wchar_t *path = _wgetenv(L"PATH");
    info(QStringLiteral("PATH: %1").arg(path ? QString::fromWCharArray(path) : QStringLiteral("<empty>")));
#endif
}

void installQtMessageHandler()
{
    s_previousMessageHandler = qInstallMessageHandler(qtMessageHandler);
    info(QStringLiteral("Qt message handler installed"));
}

void installCrashHandler()
{
#ifdef Q_OS_WIN
    SetUnhandledExceptionFilter(unhandledExceptionFilter);
    info(QStringLiteral("Windows unhandled exception filter installed"));
#else
    info(QStringLiteral("Native crash handler is only enabled on Windows"));
#endif
}

void info(const QString &message)
{
    writeLine(QStringLiteral("INFO"), message);
}

void warning(const QString &message)
{
    writeLine(QStringLiteral("WARN"), message);
}

void critical(const QString &message)
{
    writeLine(QStringLiteral("ERROR"), message);
}

QString logFilePath()
{
    return s_logFilePath;
}

} // namespace StartupLogger
