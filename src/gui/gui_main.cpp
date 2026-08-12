#include "gui_main.h"
#include "include/remc_spdlog.h"

#include <iostream>
#include <cassert>

#include <QMainWindow>
#include <QPushButton>
#include <QApplication>
#include <QWidget>
#include <QTextEdit>
#include <QPainter>
#include <QPalette>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QFile>
#include <QGraphicsColorizeEffect>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <qapplication.h>
#include <qboxlayout.h>
#include <qcolor.h>
#include <qobject.h>
#include <qpalette.h>
#include <qpushbutton.h>
#include <qwidget.h>

namespace remc::gui {

//
// MainWindow
//
MainWindow::MainWindow(QApplication* qapp) : 
   QMainWindow(nullptr), 
   main_widget_(new QWidget(this)),
   main_layout_(new QVBoxLayout(main_widget_)) 
{
   assert(qapp);

   this->setWindowTitle("REMC");
   this->setCentralWidget(main_widget_);

   InitStyles(qapp);

   DrawMenuBar();
}

void MainWindow::InitStyles(QApplication* qapp) {
   assert(qapp);
   
   QPalette palette = qapp->palette();
   palette.setColor(QPalette::Highlight, QColorConstants::DarkCyan);
   // set global palette
   QApplication::setPalette(palette);

   QString qss_folder_path = QFileInfo(__FILE__).absolutePath() + "/qss/";
   GlobalLogDebug("qss folder at ({})", qss_folder_path.toStdString());

   // qss menu-bar
   QFile file(qss_folder_path + "menu_bar.qss");
   if (file.open(QFile::ReadOnly | QFile::Text)) {
      QString style = QLatin1String(file.readAll()); 
      qapp->setStyleSheet(style);
      GlobalLogDebug("qss for menu-bar successfully loaded");
   }
   else GlobalLogWarning("qss for menu-bar is not loaded: ({})", file.errorString().toStdString());
}

void MainWindow::DrawMenuBar() {
   auto menu_bar = this->menuBar();

   auto menu_sessions = menu_bar->addMenu("Sessions"); {
      menu_sessions->addAction("Status");
      menu_sessions->addSeparator();
      menu_sessions->addAction("Settings");
   }
   auto menu_remc = menu_bar->addMenu("REMC"); {
      auto exit = menu_remc->addAction("Exit"); {
         connect(exit, &QAction::triggered, this, &QApplication::exit);
      }
   }
   menu_bar->addMenu("Help");

}

} // namespace remc::gui
