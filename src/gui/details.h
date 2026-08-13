#ifndef REMC_GUI_DETAILS_H_
#define REMC_GUI_DETAILS_H_

#include <QWidget>
#include <QString>

namespace remc::gui::details {

QWidget* CreateHorizontalBoxWidget(
   QWidget* parent, 
   const QList<QWidget*>& widgets
);

QString GetFileAbsolutePath(const QString& additional = {});

} // namespace remc::gui::details

#endif // REMC_GUI_DETAILS_H_
