#ifndef WAITDIALOG_H
#define WAITDIALOG_H

#include <QDialog>

namespace Ui
{
class WaitDialog;
}

class WaitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WaitDialog(QWidget *parent = 0);
    ~WaitDialog();

public slots:
   void onUndoAllowed();
   void onUndoDisallowed();

protected:
   void closeEvent(QCloseEvent* event) override;

private:
    Ui::WaitDialog *ui;
    bool m_allowed;

signals:
    void undoAllowed();
    void undoDisallowed();
};

#endif // WAITDIALOG_H
