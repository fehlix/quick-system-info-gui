/**********************************************************************
 *  mainwindow.cpp
 **********************************************************************
 * Copyright (C) 2015 MX Authors
 *
 * Authors: Adrian
 *          Paul David Callahan
 *          Dolphin Oracle
 *          MX Linux <http://mxlinux.org>
 *
 * This file is part of mx-welcome.
 *
 * mx-welcome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mx-welcome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mx-welcome.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTextEdit>
#include <QScreen>
#include <QAction>
#include <QFileDialog>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "version.h"
#include "QClipboard"
#include "about.h"

MainWindow::MainWindow(const QCommandLineParser& arg_parser, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::MainWindow)
{
    qDebug().noquote() << QCoreApplication::applicationName() << "version:" << VERSION;
    ui->setupUi(this);
    setWindowFlags(Qt::Window); // for the close, min and max buttons
    setup();

}

MainWindow::~MainWindow()
{
    delete ui;
}

// setup versious items first time program runs
void MainWindow::setup()
{
    version = getVersion("quick-system-info-gui");
    this->setWindowTitle(tr("Quick System Info"));
    this->setWindowIcon(QIcon::fromTheme("mx-qsi"));
    systeminfo();
    ui->textSysInfo->setWordWrapMode(QTextOption::NoWrap);
    resize(QGuiApplication::primaryScreen()->availableGeometry().size() * 0.6);
    QAction *copyreport = new QAction(this);
    copyreport->setShortcut(Qt::Key_C | Qt::CTRL);
    connect(copyreport, SIGNAL(triggered()), this, SLOT(on_ButtonCopy_clicked()));
    this->addAction(copyreport);

    QAction *copyreport2 = new QAction(this);
    copyreport2->setShortcut((Qt::Key_C | Qt::ALT));
    connect(copyreport2, SIGNAL(triggered()), this, SLOT(on_ButtonCopy_clicked()));
    this->addAction(copyreport2);

    ui->ButtonCopy->setDefault(true);
}

// Util function for getting bash command output and error code
Result MainWindow::runCmd(QString cmd)
{
    QEventLoop loop;
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    connect(&proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::quit);
    proc.start("/bin/bash", QStringList() << "-c" << cmd);
    loop.exec();
    return { proc.exitCode(), proc.readAll().trimmed() };
}

// Get version of the program
QString MainWindow::getVersion(QString name)
{
    return runCmd("dpkg-query -f '${Version}' -W " + name).output;
}

// About button clicked
void MainWindow::on_buttonAbout_clicked()
{
    this->hide();
    QMessageBox msgBox(QMessageBox::NoIcon,
        tr("About Quick-System-Info-gui"), "<p align=\"center\"><b><h2>" + tr("Quick System Info") + "</h2></b></p><p align=\"center\">" + tr("Version: ") + version + "</p><p align=\"center\"><h3>" + tr("Program for displaying a quick system info report") + "</h3></p><p align=\"center\"><a href=\"http://www.mxlinux.org/mx\">http://www.mxlinux.org</a><br /></p><p align=\"center\">" + tr("Copyright (c) MX Linux") + "<br /><br /></p>");
    QPushButton* btnLicense = msgBox.addButton(tr("License"), QMessageBox::HelpRole);
    QPushButton* btnChangelog = msgBox.addButton(tr("Changelog"), QMessageBox::HelpRole);
    QPushButton* btnCancel = msgBox.addButton(tr("Cancel"), QMessageBox::NoRole);
    btnCancel->setIcon(QIcon::fromTheme("window-close"));

    msgBox.exec();

    if (msgBox.clickedButton() == btnLicense) {
        QString cmd = QString("mx-viewer file:///usr/share/doc/quick-system-info-gui/license.html '%1'").arg(tr("Quick System Info"));
        system(cmd.toUtf8());
    } else if (msgBox.clickedButton() == btnChangelog) {
        QDialog* changelog = new QDialog(this);
        changelog->resize(600, 500);

        QTextEdit* text = new QTextEdit;
        text->setReadOnly(true);
        text->setText(runCmd("zless /usr/share/doc/" + QFileInfo(QCoreApplication::applicationFilePath()).fileName() + "/changelog.gz").output);

        QPushButton* btnClose = new QPushButton(tr("&Close"));
        btnClose->setIcon(QIcon::fromTheme("window-close"));
        connect(btnClose, &QPushButton::clicked, changelog, &QDialog::close);

        QVBoxLayout* layout = new QVBoxLayout;
        layout->addWidget(text);
        layout->addWidget(btnClose);
        changelog->setLayout(layout);
        changelog->exec();
    }
    this->show();
}

void MainWindow::on_pushSave_clicked()
{
    QFileDialog dialog(this, tr("Save System Information"));
    dialog.setDefaultSuffix("txt");
    dialog.setNameFilters({"*.txt"});
    dialog.selectFile("sysinfo.txt");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if(dialog.exec()) {
        QFile file(dialog.selectedFiles().at(0));
        bool ok = false;
        if (file.open(QFile::Truncate|QFile::WriteOnly)) {
            const QByteArray &text = ui->textSysInfo->toPlainText().toUtf8();
            ok = (file.write(text) == text.size());
            file.close();
        }
        if (ok) QMessageBox::information(this, windowTitle(), tr("System information saved."));
        else QMessageBox::critical(this, windowTitle(), tr("Could not save system information."));
    }
}

void MainWindow::on_ButtonCopy_clicked()
{
    QClipboard *clipboard = QApplication::clipboard();
    QString text2 = ui->textSysInfo->toPlainText();
    text2.append("[/code]");
    text2.prepend("[code]");
    clipboard->setText(text2);
}

void MainWindow::systeminfo()
{
    QString text = runCmd(QStringLiteral("/usr/bin/quick-system-info-mx -g")).output;
    text.remove("[code]");
    text.remove("[/code]");
    text.replace("http: /","http:/");
    text.replace("https: /","https:/");
    ui->textSysInfo->setPlainText(text.trimmed());
}



void MainWindow::on_ButtonHelp_clicked()
{
    QString url = QStringLiteral("file:///usr/share/doc/quick-system-info-gui/quick-system-info-gui.html");

    displayDoc(url, tr("%1 Help").arg(tr("Quick System Info (gui)")));
}
