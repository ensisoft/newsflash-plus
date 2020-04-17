// Copyright (c) 2010-2017 Sami Väisänen, Ensisoft
//
// http://www.ensisoft.com
//
// This software is copyrighted software. Unauthorized hacking, cracking, distribution
// and general assing around is prohibited.
// Redistribution and use in source and binary forms, with or without modification,
// without permission are prohibited.
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define LOGTAG "FileSystem"

#include "newsflash/config.h"

#include "newsflash/warnpush.h"
#  include <QFileDialog>
#  include <QMessageBox>
#  include <QDir>
#include "newsflash/warnpop.h"

#include "app/settings.h"
#include "app/engine.h"
#include "common.h"
#include "filesystem.h"

namespace gui
{

FileSystemSettings::FileSystemSettings()
{
    mUI.setupUi(this);
}

FileSystemSettings::~FileSystemSettings()
{}

bool FileSystemSettings::validate() const
{
    if (!gui::validate(mUI.edtAdult))
        return false;
    if (!gui::validate(mUI.edtApps))
        return false;
    if (!gui::validate(mUI.edtMusic))
        return false;
    if (!gui::validate(mUI.edtGames))
        return false;
    if (!gui::validate(mUI.edtImages))
        return false;
    if (!gui::validate(mUI.edtMovies))
        return false;
    if (!gui::validate(mUI.edtTelevision))
        return false;
    if (!gui::validate(mUI.edtOther))
        return false;
    if (!gui::validate(mUI.edtLog))
        return false;

    return true;
}

void FileSystemSettings::on_btnAdult_clicked()
{
    auto dir = QFileDialog::getExistingDirectory(this,
        tr("Select Download Folder for Adult Content"));
    if (dir.isEmpty())
        return;
    mUI.edtAdult->setText(dir);
}

void FileSystemSettings::on_btnApps_clicked()
{
    auto dir = QFileDialog::getExistingDirectory(this,
        tr("Select Download Folder for Applications"));
    if (dir.isEmpty())
        return;
    mUI.edtApps->setText(dir);
}

void FileSystemSettings::on_btnMusic_clicked()
{
    auto dir = QFileDialog::getExistingDirectory(this,
        tr("Select Download Folder for Music"));
    if (dir.isEmpty())
        return;
    mUI.edtMusic->setText(dir);

}
void FileSystemSettings::on_btnGames_clicked()
{
    auto dir = QFileDialog::getExistingDirectory(this,
        tr("Select Download Folder for Games"));
    if (dir.isEmpty())
        return;
    mUI.edtAdult->setText(dir);
}
void FileSystemSettings::on_btnImages_clicked()
{
    auto dir = QFileDialog::getExistingDirectory(this,
        tr("Select Download Folder for Images"));
    if (dir.isEmpty())
        return;
    mUI.edtImages->setText(dir);
}
void FileSystemSettings::on_btnMovies_clicked()
{
    auto dir = QFileDialog::getExistingDirectory(this,
        tr("Select Download Folder for Movies"));
    if (dir.isEmpty())
        return;
    mUI.edtMovies->setText(dir);
}
void FileSystemSettings::on_btnTelevision_clicked()
{
    auto dir = QFileDialog::getExistingDirectory(this,
        tr("Select Download Folder for Television"));
    if (dir.isEmpty())
        return;
    mUI.edtTelevision->setText(dir);
}
void FileSystemSettings::on_btnOther_clicked()
{
    auto dir = QFileDialog::getExistingDirectory(this,
        tr("Select Download Folder for Other Content"));
    if (dir.isEmpty())
        return;
    mUI.edtOther->setText(dir);
}
void FileSystemSettings::on_btnLog_clicked()
{
    auto dir = QFileDialog::getExistingDirectory(this,
        tr("Select Application Log Files Folder"));
    if (dir.isEmpty())
        return;
    mUI.edtLog->setText(dir);
}



FileSystemModule::FileSystemModule()
{}

FileSystemModule::~FileSystemModule()
{}

void FileSystemModule::loadState(app::Settings& settings)
{}

void FileSystemModule::saveState(app::Settings& settings)
{

}

SettingsWidget* FileSystemModule::getSettings()
{
    auto* ptr = new FileSystemSettings;
    ptr->mUI.edtAdult->setText(app::g_engine->getDownloadPath(app::MainMediaType::Adult));
    ptr->mUI.edtApps->setText(app::g_engine->getDownloadPath(app::MainMediaType::Apps));
    ptr->mUI.edtMusic->setText(app::g_engine->getDownloadPath(app::MainMediaType::Music));
    ptr->mUI.edtGames->setText(app::g_engine->getDownloadPath(app::MainMediaType::Games));
    ptr->mUI.edtImages->setText(app::g_engine->getDownloadPath(app::MainMediaType::Images));
    ptr->mUI.edtMovies->setText(app::g_engine->getDownloadPath(app::MainMediaType::Movies));
    ptr->mUI.edtTelevision->setText(app::g_engine->getDownloadPath(app::MainMediaType::Television));
    ptr->mUI.edtOther->setText(app::g_engine->getDownloadPath(app::MainMediaType::Other));
    ptr->mUI.edtLog->setText(app::g_engine->getLogfilesPath());

    const auto overwrite = app::g_engine->getOverwriteExistingFiles();
    const auto discard   = app::g_engine->getDiscardTextContent();
    const auto lowdisk   = app::g_engine->getCheckLowDisk();
    ptr->mUI.chkOverwriteExisting->setChecked(overwrite);
    ptr->mUI.chkDiscardText->setChecked(discard);
    ptr->mUI.chkCheckLowDisk->setChecked(lowdisk);
    return ptr;
}

void FileSystemModule::applySettings(SettingsWidget* gui)
{
    auto* ptr = dynamic_cast<FileSystemSettings*>(gui);

    app::g_engine->setDownloadPath(app::MainMediaType::Adult, ptr->mUI.edtAdult->text());
    app::g_engine->setDownloadPath(app::MainMediaType::Apps, ptr->mUI.edtApps->text());
    app::g_engine->setDownloadPath(app::MainMediaType::Music, ptr->mUI.edtMusic->text());
    app::g_engine->setDownloadPath(app::MainMediaType::Games, ptr->mUI.edtGames->text());
    app::g_engine->setDownloadPath(app::MainMediaType::Images, ptr->mUI.edtImages->text());
    app::g_engine->setDownloadPath(app::MainMediaType::Movies, ptr->mUI.edtMovies->text());
    app::g_engine->setDownloadPath(app::MainMediaType::Television, ptr->mUI.edtTelevision->text());
    app::g_engine->setDownloadPath(app::MainMediaType::Other, ptr->mUI.edtOther->text());
    app::g_engine->setLogfilesPath(ptr->mUI.edtLog->text());

    const bool overwrite = ptr->mUI.chkOverwriteExisting->isChecked();
    const bool discard   = ptr->mUI.chkDiscardText->isChecked();
    const bool lowdisk   = ptr->mUI.chkCheckLowDisk->isChecked();
    app::g_engine->setOverwriteExistingFiles(overwrite);
    app::g_engine->setDiscardTextContent(discard);
    app::g_engine->setCheckLowDisk(lowdisk);
}

void FileSystemModule::freeSettings(SettingsWidget* gui)
{
    delete gui;
}

} // namespace
