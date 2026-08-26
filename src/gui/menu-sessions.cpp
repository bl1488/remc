#include "gui/gui-main.h"
#include "include/remc-spdlog.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QToolBar>
#include <qboxlayout.h>

namespace remc::gui::pages {

// ===== StatusPageWidget =====
//
class PageSessions : public QWidget {
public:
   class WidgetSessionNode;
public:
   PageSessions(QWidget* parent) : 
      QWidget(parent), 
      main_layout_(new QVBoxLayout(this))
   {
      assert(parent);

      Init();

      main_layout_->addStretch();
      this->setLayout(main_layout_);
   }

private:
   void Init() {

   }

private:
   QVBoxLayout* main_layout_{};
   
};

// ===== WidgetSessionNode =====
//
class WidgetSessionNode : public QWidget {
public:
   struct NodeData {
      QString     session_name{ "noname" };
      std::size_t session_id{};
      bool        status{};
   };
public:
   WidgetSessionNode(QWidget* parent) : 
      QWidget(parent), main_layout_(new QVBoxLayout(parent))
   {
      assert(parent);

      this->setStyleSheet("background-color: #185490;");
      this->setLayout(main_layout_);
   }

private:
   NodeData     data_;
   QVBoxLayout* main_layout_{};
};

} // namespace remc::gui::pages

void remc::gui::MainWindow::PageInitSessions() {
   page_sessions_ = new pages::PageSessions(page_list_);
   page_sessions_->setObjectName("Pages::Sessions");

   page_list_->addWidget(page_sessions_);
}
