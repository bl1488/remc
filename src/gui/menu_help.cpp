#include "gui_main.h"

#include <QVBoxLayout>
#include <qboxlayout.h>
#include <qwidget.h>

namespace remc::gui::pages {

class PageHelp : public QWidget {
public:
   PageHelp(QWidget* parent) : 
      QWidget(parent),
      main_layout_(new QVBoxLayout(this))
   {
      assert(parent);


   }

private:
   QVBoxLayout* main_layout_{};
};

} // namespace remc::gui::pages

void remc::gui::MainWindow::PageInitHelp() {
   page_help_ = new pages::PageHelp(page_list_);
   page_help_->setObjectName("Pages::Help");

   page_list_->addWidget(page_help_);
}
