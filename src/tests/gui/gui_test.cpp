#include "gui/gui_main.h"
#include "include/remc_spdlog.h"

#include <QApplication>

int main(int argc, char** argv) {
   // init logger
   remc::InitFileConsoleLogger(
      remc::DEFAULT_GLOBAL_LOGGER_NAME, 
      remc::DEFAULT_LOGFILE_PATH
   );

   QApplication app(argc, argv);
   QApplication::setStyle("Fusion");
   
   remc::gui::MainWindow main_wnd(&app);
   main_wnd.resize(600, 500);
   
   main_wnd.show();

   return app.exec();
}
