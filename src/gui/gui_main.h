#ifndef REMC_GUI_MAIN_H_
#define REMC_GUI_MAIN_H_

#include <cstring>

#include <QMainWindow>
#include <QPushButton>
#include <QApplication>
#include <QWidget>
#include <QTextEdit>
#include <QPainter>
#include <QVBoxLayout>
#include <QGraphicsColorizeEffect>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>

namespace remc::gui {

namespace lex {

void CalculatorLexer(const std::string& expr);

} // namespace remc::gui::lex

// ===== MainWindow =====
//
class MainWindow : public QMainWindow {
   Q_OBJECT
public:
   explicit MainWindow(QWidget* parent = nullptr);

public:
   QWidget* GetMainWidget() 
      const noexcept { return main_widget_; }

   QVBoxLayout* GetMainLayout()
      const noexcept { return main_layout_; }

private:
   QWidget*     main_widget_{};
   QVBoxLayout* main_layout_{};
};

// ===== ButtonPanel =====
//
class ButtonPanel : public QWidget {
   Q_OBJECT
public:
   explicit ButtonPanel(QTextEdit* out_text_edit, QWidget* parent = nullptr);

public slots:
   void OnClick();

private:
   // buttons animation
   // just for fun
   void InitAnimation(int delay = 2000);

private:
   QGridLayout* layout_;
   QTextEdit*   out_text_edit_;
};

} // namespace remc::gui

#endif // REMC_GUI_MAIN_H_
