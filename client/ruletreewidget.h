#ifndef RULETREEWIDGET_H
#define RULETREEWIDGET_H

class RuleTreeWidget : public QTreeWidget
{
  Q_OBJECT
public:
  RuleTreeWidget(QWidget* parent);

  void mouseReleaseEvent(QMouseEvent *event);
signals:
  void signalItemRightClicked(const QModelIndex& item);
};
#endif //RULETREEWIDGET_H

