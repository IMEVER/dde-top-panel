//
// Created by septemberhx on 2020/5/22.
//

#include <DApplication>
#include <DGuiApplicationHelper>
#include <unistd.h>
#include "window/MainWindow.h"

#include "../interfaces/constants.h"
#include "../globalmenu/menuproxy.h"
#include "../appmenu/menuimporter.h"

DWIDGET_USE_NAMESPACE
#ifdef DCORE_NAMESPACE
DCORE_USE_NAMESPACE
#else
DUTIL_USE_NAMESPACE
#endif


void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    switch (type) {
    case QtDebugMsg:
        fprintf(stdout, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        break;
    }
}


int main(int argc, char *argv[]) {
    // qputenv("QT_LOGGING_TO_CONSOLE", QByteArray("0"));
    QGuiApplication::setAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);
    DApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    DGuiApplicationHelper::setUseInactiveColorGroup(false);
    DApplication app(argc, argv);
    qInstallMessageHandler(myMessageOutput);

    if (app.setSingleInstance("imever"))
    {
        app.setOrganizationName("IMEVER");
        app.setOrganizationDomain("imever.me");
        app.setApplicationName("dde-top-panel");
        // app.setApplicationDisplayName("全局顶栏");
        app.setApplicationVersion("1.0.1");
        // app.loadTranslator();

        QLocale locale = QLocale::system();
        QString translatePath = QString("/usr/share/dde-dock/translations/dde-dock_%1.qm").arg(locale.name());
        if (QFile::exists(translatePath)) {
            qDebug() << "load translate" << translatePath;
            auto translator = new QTranslator();
            translator->load(translatePath);
            app.installTranslator(translator);
        }

        app.setAttribute(Qt::AA_UseHighDpiPixmaps, false);

        app.setProperty(PROP_POSITION, QVariant::fromValue(Dock::Top));
        app.setProperty(PROP_DISPLAY_MODE, QVariant::fromValue(Dock::Fashion));
        app.setAttribute(Qt::AA_DontUseNativeMenuBar, true);

        TopPanelLauncher launcher;
        MenuProxy menuPrxy;

        auto *menuImporter = new MenuImporter();
        menuImporter->connectToBus();

        return app.exec();
    }
    else
    {
        app.exit();
        return 0;
    }
}
