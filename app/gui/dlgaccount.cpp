
// $Id: dlgserver.cpp,v 1.11 2010/08/22 12:01:15 svaisane Exp $

#include <QDir>
#include <QFileDialog>

#include "ui_dlgserver.h"
#include "dlgserver.h"
#include "settings.h"

using namespace app;

DlgServer::DlgServer(QWidget *parent, server& s, bool create_new) :
  QDialog(parent),
  m_ui(new Ui::DlgServer),
  serv_(s),
  create_new_(create_new)
{
    m_ui->setupUi(this);

    m_ui->edtName->setText(s.name);
    m_ui->edtHost->setText(s.host);
    m_ui->edtPort->setText(QString::number(s.port));
    m_ui->edtHostSecure->setText(s.secure_host);
    m_ui->edtPortSecure->setText(QString::number(s.secure_port));
    m_ui->edtUsername->setText(s.user);
    m_ui->edtPassword->setText(s.pass);
    m_ui->edtDataFiles->setText(s.datapath);
    m_ui->edtDataFiles->setCursorPosition(0);
    m_ui->chkCompressedHeaders->setChecked(s.compressed_headers);
    m_ui->chkBatchMode->setChecked(s.batch_mode);
    m_ui->grpSecure->setChecked(s.enable_secure);   
    m_ui->grpNonSecure->setChecked(s.enable_nonsecure);
    m_ui->grpLogin->setChecked(!s.pass.isEmpty() || !s.user.isEmpty());
    
    m_ui->btnOK->setEnabled(s.enable_secure || s.enable_nonsecure);
    
    m_ui->maxConnections->setValue(serv_.maxconn);

    //m_ui->edtDataFiles->setEnabled(false);
    if (create_new_)
        m_ui->edtDataFiles->setText(QDir::toNativeSeparators(s.datapath + "/" + s.name));
    else
        m_ui->edtDataFiles->setText(QDir::toNativeSeparators(s.datapath));
    
    m_ui->edtDataFiles->setCursorPosition(0);
    m_ui->edtName->setFocus();
    m_ui->edtName->setSelection(0, s.name.size());
}

DlgServer::~DlgServer()
{
    delete m_ui;
}

void DlgServer::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void DlgServer::on_btnOK_clicked()
{
    QString name = m_ui->edtName->text();
    if (name.isEmpty())
    {
        m_ui->edtName->setFocus();
        return;
    }

    // need to have at least secure or non-secure
    if (!m_ui->grpNonSecure->isChecked() && !m_ui->grpSecure->isChecked())
        return;

    QString non_secure_host;
    QString secure_host;
    int non_secure_port = 0;
    int secure_port = 0;

    if (m_ui->grpNonSecure->isChecked())
    {
        bool ok = false;
        non_secure_port = m_ui->edtPort->text().toInt(&ok);
        non_secure_host = m_ui->edtHost->text().trimmed();
        if (!ok || non_secure_port < 0)
        {
            m_ui->edtPort->setFocus();
            return;
        }
        if (non_secure_host.isEmpty())
        {
            m_ui->edtHost->setFocus();
            return;
        }
    }
    
    if (m_ui->grpSecure->isChecked())
    {
        bool ok = false;
        secure_port = m_ui->edtPortSecure->text().toInt(&ok);
        secure_host = m_ui->edtHostSecure->text().trimmed();
        if (!ok || secure_port < 0)
        {
            m_ui->edtPortSecure->setFocus();
            return;
        }
        if (secure_host.isEmpty())
        {
            m_ui->edtHostSecure->setFocus();
            return;
        }
    }
    
    QString user; 
    QString pass;
    if (m_ui->grpLogin->isChecked())
    {
        user = m_ui->edtUsername->text().trimmed();
        if (user.isEmpty())
        {
            m_ui->edtUsername->setFocus();
            return;
        }
        pass = m_ui->edtPassword->text().trimmed();
        if (pass.isEmpty())
        {
            m_ui->edtPassword->setFocus();
            return;
        }
    }

    serv_.name = name;

    if (!non_secure_host.isEmpty())
    {
        serv_.host = non_secure_host;
        serv_.port = non_secure_port;
    }
    if (!secure_host.isEmpty())
    {
        serv_.secure_host = secure_host;
        serv_.secure_port = secure_port;
    }
    serv_.enable_secure    = m_ui->grpSecure->isChecked();
    serv_.enable_nonsecure = m_ui->grpNonSecure->isChecked();
    serv_.user             = user;
    serv_.pass             = pass;
    serv_.datapath         = m_ui->edtDataFiles->text();
    serv_.maxconn          = m_ui->maxConnections->value();
    serv_.compressed_headers = m_ui->chkCompressedHeaders->isChecked();
    serv_.batch_mode = m_ui->chkBatchMode->isChecked();

    accept();
}

void DlgServer::on_btnBrowseData_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Folder for data files"));
    if (dir.isNull())
        return;
    
    QString path;
    if (create_new_)
    {
        QString name = m_ui->edtName->text();
        path = QDir::toNativeSeparators(dir + "/" + name);
    }
    else
    {
        path = QDir::toNativeSeparators(dir);
    }
    m_ui->edtDataFiles->setText(path);
}

void DlgServer::on_grpSecure_clicked(bool val)
{
    m_ui->btnOK->setEnabled(m_ui->grpSecure->isChecked() || m_ui->grpNonSecure->isChecked());
}

void DlgServer::on_grpNonSecure_clicked(bool val)
{
    m_ui->btnOK->setEnabled(m_ui->grpSecure->isChecked() || m_ui->grpNonSecure->isChecked());
}

void DlgServer::on_grpLogin_clicked(bool val)
{
    m_ui->edtUsername->clear();
    m_ui->edtPassword->clear();
}

void DlgServer::on_edtName_textEdited()
{
    if (!create_new_)
        return;

    QString name = m_ui->edtName->text();
    QString path = QDir::toNativeSeparators(QDir::cleanPath(serv_.datapath + "/" + name));
    m_ui->edtDataFiles->setText(path);
}
