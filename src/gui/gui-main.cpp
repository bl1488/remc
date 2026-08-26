#include "gui/gui-main.h"
#include "gui/details.h"
#include "include/remc-spdlog.h"

#include <cassert>

#include <QPushButton>
#include <QWidget>
#include <QTextEdit>
#include <QPalette>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QFile>
#include <QGraphicsColorizeEffect>
#include <QPropertyAnimation>
#include <qcolor.h>

namespace remc::gui {

//
// MainWindow
//
MainWindow::MainWindow(QApplication* qapp) : 
   QMainWindow(nullptr), 
   page_list_(new QStackedWidget(this))
{
   assert(qapp);

   this->setWindowTitle("REMC");
   this->setCentralWidget(page_list_);

   InitStyles(qapp);
   InitMenuBar();

   // init home page
   PageInitHome();
   // Home by default
   page_list_->setCurrentWidget(page_home_);
}

void MainWindow::InitStyles(QApplication* qapp) {
   assert(qapp);
   
   QPalette palette = qapp->palette();
   palette.setColor(QPalette::Highlight, QColorConstants::DarkYellow);
   qapp->setPalette(palette);

   // qss menu-bar
   QFile file(details::GetFileAbsolutePath("/styles/menu-bar.qss"));
   if (file.open(QFile::ReadOnly | QFile::Text)) {
      qapp->setStyleSheet(QLatin1String(file.readAll()));
      GlobalLogDebug("qss: menu-bar loaded");
   }
   else GlobalLogWarning("menu-bar qss loading failed: ({})", file.errorString().toStdString());
}

void MainWindow::InitMenuBar() {
   // Home
   auto menu_remc = menuBar()->addAction("Home");
   connect(menu_remc, &QAction::triggered, this, [this] {
      if (page_list_->currentWidget() != page_home_) {
         PageInitHome();
         page_list_->setCurrentWidget(page_home_);
      }
   });
   menu_remc->setIcon(QIcon(details::GetFileAbsolutePath("/rsrc/home.png")));

   // Sessions
   auto menu_sessions = menuBar()->addAction("Sessions");
   connect(menu_sessions, &QAction::triggered, this, [this] {
      if (page_list_->currentWidget() != page_sessions_) {
         PageInitSessions();
         page_list_->setCurrentWidget(page_sessions_);
      }
   });

   // Help
   auto menu_help = menuBar()->addAction("Help");
   connect(menu_help, &QAction::triggered, this, [this] {
      if (page_list_->currentWidget() != page_help_) {
         PageInitHelp();
         page_list_->setCurrentWidget(page_help_);
      }
   });
}

} // namespace remc::gui
