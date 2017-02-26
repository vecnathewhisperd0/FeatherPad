#ifndef PREF_H
#define PREF_H

#include <QDialog>
#include "config.h"

namespace FeatherPad {

namespace Ui {
class PrefDialog;
}

class PrefDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PrefDialog (QWidget *parent = 0);
    ~PrefDialog();

private slots:
    void prefSize (int checked);
    void prefStartSize (int value);
    void prefIcon (int checked);
    void prefToolbar (int checked);
    void prefMenubar (int checked);
    void prefSearchbar (int checked);
    void prefStatusbar (int checked);
    void prefFont (int checked);
    void prefWrap (int checked);
    void prefIndent (int checked);
    void prefLine (int checked);
    void prefSyntax (int checked);
    void prefDarkColScheme (int checked);
    void prefColValue (int value);
    void prefScrollJumpWorkaround (int checked);
    void prefTabWrapAround (int checked);
    void prefHideSingleTab (int checked);
    void prefMaxSHSize (int value);
    void prefExecute (int checked);
    void prefCommand (QString command);
    void showWhatsThis();

private:
    void closeEvent (QCloseEvent *event);
    void preftabPosition();

    Ui::PrefDialog *ui;
    QWidget * parent_;
};

}

#endif // PREF_H
