
// $Id: dlgserver.h,v 1.4 2010/04/26 12:40:17 svaisane Exp $

#ifndef NEWSFLASH_DLGSERVER_H
#define NEWSFLASH_DLGSERVER_H

#include <QtGui/QDialog>

namespace app
{
    struct server;
}

namespace Ui {
    class DlgServer;
}

class DlgServer : public QDialog 
{
    Q_OBJECT
public:
    DlgServer(QWidget *parent, app::server& s, bool create_new);
   ~DlgServer();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::DlgServer *m_ui;
    app::server&  serv_;
    bool create_new_;

private slots:
    void on_btnOK_clicked();
    void on_btnBrowseData_clicked();
    void on_grpSecure_clicked(bool val);
    void on_grpNonSecure_clicked(bool val);
    void on_grpLogin_clicked(bool val);
    void on_edtName_textEdited();
};

#endif // NEWSFLASH_DLGSERVER_H
