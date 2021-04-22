//
// Created by septemberhx on 2020/5/22.
//

#include <DApplication>
#include <DGuiApplicationHelper>
#include <unistd.h>
#include "window/MainWindow.h"

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
        app.setAttribute(Qt::AA_UseHighDpiPixmaps, false);

        TopPanelLauncher launcher;
        MenuProxy proxy;

        return app.exec();
    }
    else
    {
        app.exit();
        return 0;
    }
}
