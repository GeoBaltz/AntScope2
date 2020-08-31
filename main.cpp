#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QAbstractNativeEventFilter>
#include "analyzer/customanalyzer.h"

bool g_developerMode = false;
bool g_raspbian = false;

#ifdef Q_OS_WIN
#include <windows.h>
#include <dbt.h>

#define LOG_TO_FILE

#ifdef LOG_TO_FILE
QString logFilePath = "antscope2";
bool firstLog = true;
void customMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (type != QtInfoMsg)
        return;
    QHash<QtMsgType, QString> msgLevelHash({{QtDebugMsg, "Debug"}, {QtInfoMsg, "Info"}, {QtWarningMsg, "Warning"}, {QtCriticalMsg, "Critical"}, {QtFatalMsg, "Fatal"}});
    QTime time = QTime::currentTime();
    QString formattedTime = time.toString("hh:mm:ss.zzz");
    QString sufix = QDateTime::currentDateTime().toString("-yyyyMMdd_hhmmss.log");
    QString logLevelName = "";//msgLevelHash[type];

    QString txt = QString("%1 %2: %3 (%4:%5, %6)")
            .arg(formattedTime, logLevelName, msg,  context.file)
            .arg(context.line)
            .arg(context.function);
    if (firstLog) {
        firstLog = false;
        QDir dir = QDir::tempPath();
        logFilePath = dir.absoluteFilePath(logFilePath + sufix);
    }
    QFile outFile(logFilePath);
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << endl;
    ts.flush();
}
#endif

class MyNativeEventFilter : public QAbstractNativeEventFilter {
public :
    virtual bool nativeEventFilter( const QByteArray &eventType, void *message, long * /*result*/ )
    Q_DECL_OVERRIDE
    {
        if (eventType == "windows_generic_MSG")
        {
          MSG *msg = static_cast<MSG *>(message);
          static int i = 0;

              msg = (MSG*)message;
                  //qDebug() << "message: " << msg->message << " wParam: " << msg->wParam
                    //  << " lParam: " << msg->lParam;
              if (msg->message == WM_DEVICECHANGE)
              {
                  qDebug() << "WM_DEVICECHANGE: " <<
                              (msg->wParam==DBT_DEVICEARRIVAL?"DBT_DEVICEARRIVAL":
                              (msg->wParam==DBT_DEVICEREMOVECOMPLETE?"DBT_DEVICEREMOVECOMPLETE":QString::number(msg->wParam)));
              }
            }
        return false;
    }
};
#endif


void setAbsoluteFqMaximum()
{
    int fqMax = 0;

    if (CustomAnalyzer::customized() && CustomAnalyzer::getCurrent() != nullptr) {
            fqMax = CustomAnalyzer::getCurrent()->maxFq().toInt();
    } else {
        for (int idx=1; idx<QUANTITY; idx++) {
            QString str = maxFq[idx];
            int fq = str.toInt();
            fqMax = qMax(fqMax, fq);
        }
    }
    ABSOLUTE_MAX_FQ = fqMax;
}

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);
    QStringList args = a.arguments();

#ifdef LOG_TO_FILE
    qInstallMessageHandler(customMessageOutput);
    qInfo() << "                                                         ";
    qInfo() << "*********************************************************";
    qInfo() << "  AntScope2 " << QString(ANTSCOPE2VER) << " STARTED " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    qInfo() << "                                                         ";
#endif

#ifdef Q_OS_WIN
    // TODO DEBUG: catch attach/detach device event
    //MyNativeEventFilter myEventfilter;
    //a.eventDispatcher()->installNativeEventFilter(&myEventfilter);
#endif

    if (args.contains("-developer")) {
        g_developerMode = true;
        MAX_DOTS = 1000000;
    }

    g_raspbian = QSysInfo::productType().contains("raspbian", Qt::CaseInsensitive);

    MainWindow w;

    foreach (QString path, args) {
        if (path.contains(".asd")) {
            w.openFile(path);
            break;
        }
    }
    w.show();

    return a.exec();
}
