#pragma once

#include <QString>

namespace StartupLogger {

void initialize(int argc, char *argv[]);
void installQtMessageHandler();
void installCrashHandler();

void info(const QString &message);
void warning(const QString &message);
void critical(const QString &message);

QString logFilePath();

} // namespace StartupLogger
