#include "details.h"

#include <QFileInfo>
#include <QHBoxLayout>

namespace remc::gui::details {

QWidget* CreateHorizontalBoxWidget(QWidget* parent, const QList<QWidget*>& widgets) {
   QWidget*     result = new QWidget(parent);
   QHBoxLayout* layout = new QHBoxLayout(result);
   for (auto& i : widgets) {
      layout->addWidget(i);
      i->setParent(result);
   }
   return result;
}

QString GetFileAbsolutePath(const QString& additional) {
   return QFileInfo(__FILE__).absolutePath() + additional;
}

} // namespace remc::gui::details
