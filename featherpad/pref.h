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
    void prefIcon (int checked);
    void prefToolbar (int checked);
    void prefSearchbar (int checked);
    void prefStatusbar (int checked);
    void prefFont (int checked);
    void prefWrap (int checked);
    void prefIndent (int checked);
    void prefLine (int checked);
    void prefSyntax (int checked);
    void prefDarkColScheme (int checked);
    void prefScrollJumpWorkaround (int checked);
    void prefTranslucencyWorkaround (int checked);
    void prefMaxSHSize (int value);

private:
    Ui::PrefDialog *ui;
    QWidget * parent_;
};

}

#endif // PREF_H
