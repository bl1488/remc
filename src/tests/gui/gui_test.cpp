#include "gui/gui_main.h"

#include <any>
#include <iostream>

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>

#include <future>
#include <thread>

void foo(remc::gui::MainWindow* main_wnd) {
   assert(main_wnd);

   auto text_wnd = new QTextEdit(main_wnd->GetMainWidget()); {
      text_wnd->setReadOnly(true);
      text_wnd->setMinimumSize(0, 100);
   }

   auto btn_panel = new remc::gui::ButtonPanel(text_wnd, main_wnd->GetMainWidget());

   main_wnd->GetMainLayout()->addWidget(text_wnd,  1);
   main_wnd->GetMainLayout()->addWidget(btn_panel, 2);
   main_wnd->GetMainLayout()->addStretch();
}


int main(int argc, char** argv) {
   // QApplication app(argc, argv);

   // remc::gui::MainWindow main_wnd;
   
   // foo(&main_wnd);
   
   // main_wnd.show();
   // return app.exec();

   std::future<int> res = std::async(std::launch::async, []() -> int {
      std::cout << "async\n";
      return 0;
   });
   
   auto a = res.get();
   for (int i = 0; i < 10; ++i) {
      std::cout << i << "\tmain\n";
   }

   
}
