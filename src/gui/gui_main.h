#ifndef REMC_GUI_MAIN_H_
#define REMC_GUI_MAIN_H_

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QMenuBar>
#include <QStackedWidget>

namespace remc::gui {

// ===== MainWindow =====  
//
class MainWindow : public QMainWindow {
   Q_OBJECT
public:
   explicit MainWindow(QApplication* app);

private:
   void InitMenuBar();
   
   void InitStyles(QApplication* qapp);

   // pages
   void PageInitHome();
   void PageInitSessions();
   void PageInitHelp();

private:
   QStackedWidget* page_list_{};
   QWidget*        page_sessions_{};
   QWidget*        page_help_{};
   QWidget*        page_home_{};
};

} // namespace remc::gui

#endif // REMC_GUI_MAIN_H_
