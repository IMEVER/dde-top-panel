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

    DGuiApplicationHelper::setUseInactiveColorGroup(false);
    // DApplication::loadDXcbPlugin();
    DApplication app(argc, argv);

    if (app.setSingleInstance("imever"))
    {
        app.setOrganizationName("IMEVER");
        app.setApplicationName("dde-top-panel");
        app.setApplicationDisplayName("DDE Top Panel");
        app.setApplicationVersion("1.0.0");
        app.loadTranslator();
        app.setAttribute(Qt::AA_EnableHighDpiScaling, true);
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