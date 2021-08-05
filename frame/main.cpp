//
// Created by septemberhx on 2020/5/22.
//

#include <DApplication>
#include <DGuiApplicationHelper>
#include <unistd.h>
#include "window/MainWindow.h"

#include "../interfaces/constants.h"
#include "../globalmenu/menuproxy.h"

DWIDGET_USE_NAMESPACE
#ifdef DCORE_NAMESPACE
DCORE_USE_NAMESPACE
#else
DUTIL_USE_NAMESPACE
#endif

int main(int argc, char *argv[]) {
    qputenv("QT_LOGGING_TO_CONSOLE", QByteArray("0"));
    DApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    DGuiApplicationHelper::setUseInactiveColorGroup(false);
    DApplication app(argc, argv);

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

        app.setProperty(PROP_POSITION, Dock::Top);
        app.setProperty(PROP_DISPLAY_MODE, Dock::Fashion);
        app.setAttribute(Qt::AA_DontUseNativeMenuBar, true);

        TopPanelLauncher launcher;
        MenuProxy menuPrxy;

        return app.exec();
    }
    else
    {
        app.exit();
        return 0;
    }
}
