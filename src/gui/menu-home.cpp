#include "gui/gui-main.h"

#include <QPushButton>

namespace remc::gui {

class PageHome : public QWidget {
public:
   PageHome(QWidget* parent) : QWidget(parent) {
      QPushButton* btn = new QPushButton("hello world", this);
      btn->setContentsMargins(0,0,0,0);
   }
};

void MainWindow::PageInitHome() {
   page_home_ = new PageHome(page_list_);
   page_home_->setObjectName("Pages::Home");

   page_list_->addWidget(page_home_);
}

} // namespace remc::gui
