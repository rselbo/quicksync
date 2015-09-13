#include "PreCompile.h"
#include "ruletreewidget.h"

RuleTreeWidget::RuleTreeWidget(QWidget* parent) :
  QTreeWidget(parent)
{

}

void RuleTreeWidget::mouseReleaseEvent(QMouseEvent *event)
{
  if(event->button() == Qt::RightButton)
  {
    QPoint pos = event->pos();
    QPersistentModelIndex index = indexAt(pos);
    bool click = (index.isValid());
    if(click)
    {
      event->accept();
      emit signalItemRightClicked(index);
      return;
    }
  }

  QTreeWidget::mouseReleaseEvent(event);
}