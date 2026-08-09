#include "gui_main.h"

#include <string>
#include <cassert>

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

enum class TokenType : uint16_t {
   TOKEN_TYPE_DIGIT,
   TOKEN_TYPE_PLUS,
   TOKEN_TYPE_MINUS,
   TOKEN_TYPE_DIV,
   TOKEN_TYPE_MUL,
   TOKEN_TYPE_ASSIGN,
   TOKEN_TYPE_MOD,
   TOKEN_TYPE_COMMA
};
   
template<TokenType type, typename Type>
struct Token {
   static constexpr auto token_type = type;
public:
   Type value;

   std::string GetTokenTypeAsString() const {
      switch (type) {
      case TokenType::TOKEN_TYPE_DIGIT:  return "TOKEN_TYPE_DIGIT";
      case TokenType::TOKEN_TYPE_PLUS:   return "TOKEN_TYPE_PLUS";
      case TokenType::TOKEN_TYPE_MINUS:  return "TOKEN_TYPE_MINUS";
      case TokenType::TOKEN_TYPE_DIV:    return "TOKEN_TYPE_DIV";
      case TokenType::TOKEN_TYPE_MUL:    return "TOKEN_TYPE_MUL";
      case TokenType::TOKEN_TYPE_ASSIGN: return "TOKEN_TYPE_ASSIGN";
      case TokenType::TOKEN_TYPE_MOD:    return "TOKEN_TYPE_MOD";
      case TokenType::TOKEN_TYPE_COMMA:  return "TOKEN_TYPE_COMMA";
      default:
         return "unknown token";
      }
   }
};

class AST {


};

void CalculatorLexer(const std::string& expr) {
   
}

} // namespace remc::gui::lex

//
// MainWindow
//
MainWindow::MainWindow(QWidget* parent) : 
   QMainWindow(parent), 
   main_widget_(new QWidget(this)),
   main_layout_(new QVBoxLayout(main_widget_)) 
{
   this->setWindowTitle("MainWindow");
   this->setCentralWidget(main_widget_);
}

//
// ButtonPanel
//
ButtonPanel::ButtonPanel(QTextEdit* out_text_edit, QWidget* parent) : 
   QWidget(parent), layout_(new QGridLayout(this)), out_text_edit_(out_text_edit)
{
   const char* ops_array    = "CD%/*-+=";
   const char* digits_array = "789654123P0,";
   // operations
   for (std::size_t i = 0, down = 0; i < std::strlen(ops_array); ++i) {
      auto btn = new QPushButton(this); {
         btn->setText(std::string(1, ops_array[i]).c_str());
         btn->setStyleSheet(
            "background-color: #598278;"
            "color: black;"
            "font-weight: bold;"
         );
         btn->setMaximumSize(70, 70);
         btn->setMinimumSize(40, 40);
         QObject::connect(btn, &QPushButton::clicked, this, &ButtonPanel::OnClick);
      }
      if (i > 3)
            layout_->addWidget(btn, ++down, 3);
      else layout_->addWidget(btn, 0, i);
   }
   // digits
   for (std::size_t i = 0; i < std::strlen(digits_array); ++i) {
      auto btn = new QPushButton(this); {
         if (digits_array[i] == 'P')
               btn->setText("PI");
         else btn->setText(std::string(1, digits_array[i]).c_str());
         btn->setMaximumSize(70, 70);
         btn->setMinimumSize(40, 40);
         QObject::connect(btn, &QPushButton::clicked, this, &ButtonPanel::OnClick);
      }
      layout_->addWidget(btn, 1 + (i / 3), i % 3);
   }

   InitAnimation();
}

void ButtonPanel::InitAnimation(int delay) {
   auto effect = new QGraphicsColorizeEffect(this);
   this->setGraphicsEffect(effect);

   QColor color_a = QColorConstants::Svg::darkcyan,
            color_b = QColorConstants::Svg::darkviolet;

   auto forward = new QPropertyAnimation(effect, "color"); {
      forward->setDuration(delay);
      forward->setStartValue(color_a); forward->setEndValue(color_b);
   }
   auto back    = new QPropertyAnimation(effect, "color"); {
      back->setDuration(delay);
      back->setStartValue(color_b);    back->setEndValue(color_a);
   }
   auto group   = new QSequentialAnimationGroup(this); {
      group->addAnimation(forward);
      group->addAnimation(back);
      group->setLoopCount(-1); 
   }

   group->start();
}

void ButtonPanel::OnClick() {
   assert(out_text_edit_);

   auto sender_obj = this->sender();
   if (!sender_obj)
      return;

   auto btn = qobject_cast<QPushButton*>(sender_obj);

   if (btn->text() == "=") {
      // parse
      std::string expr = out_text_edit_->toPlainText().toStdString();
      lex::CalculatorLexer(expr);
   }
   else out_text_edit_->insertPlainText(btn->text());
}

} // namespace remc::gui
