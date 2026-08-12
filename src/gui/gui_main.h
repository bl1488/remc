#ifndef REMC_GUI_MAIN_H_
#define REMC_GUI_MAIN_H_

#include <initializer_list>
#include <qapplication.h>
#include <string>
#include <unordered_map>

#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QWidget>
#include <QMenuBar>
#include <QVBoxLayout>

namespace remc::gui {

// ===== MainWindow =====
//
class MainWindow : public QMainWindow {
   Q_OBJECT
public:
   class MyMenuBar;

public:
   explicit MainWindow(QApplication* app);

public:
   void DrawMenuBar();
   
   QWidget*     GetMainWidget() const noexcept { return main_widget_; }

   QVBoxLayout* GetMainLayout() const noexcept { return main_layout_; }

private:
   void MenuBarMenuSettings(QMenuBar* menu_bar);

   void InitStyles(QApplication* qapp);

private:
   QWidget*     main_widget_{};
   QVBoxLayout* main_layout_{};
};

// ===== MainWindow::MyMenuBar =====
//
class MainWindow::MyMenuBar {
public:
   MyMenuBar(QMenuBar* menu_bar) : menu_bar_(menu_bar) {
      assert(menu_bar);
   }

public:
   QMenu* AddMenu(std::string_view name) {
      assert(!name.empty());

      auto menu = menu_bar_->addMenu(name.data());
      if (menu)
         menu_list_.emplace(name.data(), menu);

      return menu;
   }

   QMenu* AddSubMenu(std::string_view name, std::string_view sub_name) {
      assert(!name.empty() && !sub_name.empty());

      if (!menu_list_.contains(name.data()))
         return nullptr;

      return menu_list_[name.data()]->addMenu(sub_name.data());
   }

   QAction* AddSubActions(std::string_view name, std::initializer_list<std::string_view> actions) {
      assert(!name.empty() && actions.size());

      if (!menu_list_.contains(name.data()))
         return nullptr;

      return nullptr;
   }

private:
   QMenuBar* menu_bar_;
   std::unordered_map<std::string, QMenu*> 
             menu_list_;
};

} // namespace remc::gui

#endif // REMC_GUI_MAIN_H_
