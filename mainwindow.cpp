#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    sourceFilePath = QString();
    destFilePath = QString();

    QSettings settings("Chenlin", "Shuma");
    QVariant lastSession = settings.value("lastSession");
    if (!lastSession.isNull()) {
        loadSession(lastSession.value<ShumaSession>());
        loadFiles();
        updateEncodePage();
        ui->stackedWidget->setCurrentWidget(ui->encodePage);
    } else {
        ui->stackedWidget->setCurrentWidget(ui->selectPage);
    }

    connect(ui->selSrcBut, &QPushButton::clicked,
            this, &MainWindow::selectSourceFile);
    connect(ui->selDestBut, &QPushButton::clicked,
            this, &MainWindow::selectDestFile);
    connect(ui->inputBar, &QLineEdit::textChanged,
            this, &MainWindow::handleInputChange);
    connect(ui->inputBar, &QLineEdit::returnPressed,
            this, &MainWindow::nextEntry);

    fontlistmodel = new FontListModel();
    ui->fontListView->setModel(fontlistmodel);
}

MainWindow::~MainWindow()
{
    delete ui;
    destFile.close();
}

void MainWindow::on_maxCodeLenCB_clicked()
{
    if (ui->maxCodeLenCB->isChecked()) {
        ui->maxCodeLenLE->setEnabled(true);
    } else {
        ui->maxCodeLenLE->clear();
        ui->maxCodeLenLE->setEnabled(false);
    }
}

void MainWindow::selectSourceFile() {
    sourceFilePath = QFileDialog::getOpenFileName(this,
        "指定原表", QDir::homePath(), "(*)");

    if (sourceFilePath.isNull()) {
        return;
    }

    ui->selSrcLab->setText(QFileInfo(sourceFilePath).fileName());
}

void MainWindow::selectDestFile() {
    destFilePath = QFileDialog::getSaveFileName(this,
        "指定新表", QDir::homePath(), "(*.tsv)");

    if (destFilePath.isNull()) {
        return;
    }

    ui->selDestLab->setText(QFileInfo(destFilePath).fileName());
}

void MainWindow::loadSession(ShumaSession session) {
    sourceFilePath = session.sourceFilePath;
    destFilePath = session.destFilePath;
    fonts = session.fonts;
    maxCodeLength = session.maxCodeLength;
    banDuplicate = session.banDuplicate;
}

bool MainWindow::loadSelection() {
    QMessageBox msgBox;

    if (sourceFilePath.isEmpty()) {
        msgBox.setText("请指定原表");
        msgBox.exec();
        return false;
    }

    if (sourceFilePath.isEmpty()) {
        msgBox.setText("请指定新表");
        msgBox.exec();
        return false;
    }

    if (ui->maxCodeLenCB->isChecked()) {
        bool ok;
        int i = ui->maxCodeLenLE->text().toInt(&ok);

        if (!ok || i <= 0) {
            msgBox.setText("码长限制有问题");
            msgBox.exec();
            return false;
        }

        maxCodeLength = i;
    } else {
        maxCodeLength = noMaxLen;
    }

    if (ui->banDuplicateCB->isChecked())
        banDuplicate = true;
    else
        banDuplicate = false;

    fonts = fontlistmodel->getFonts();
    if (fonts.isEmpty())
        fonts.append(QFont());

    return true;
}

bool MainWindow::loadFiles() {
    destFile.close();
    QMessageBox msgBox;
    sourceList = QStringList();
    bool destFileExist = true;
    QTextStream in;

    QFile sourceFile(sourceFilePath);
    if (!sourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        msgBox.setText("无法打开原表");
        sourceFile.close();
        goto loadFilesError;
    }

    destFileExist = QFileInfo(destFilePath).exists();

    if (destFileExist) {
        destFile.setFileName(destFilePath);
        if (!destFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            msgBox.setText("无法打开新表");
            goto errorCloseFiles;
        }
        in.setDevice(&destFile);
    }

    finishCount = 0;
    if (!destFileExist) {
        destTable = QVector<QStringList>();
    } else if (banDuplicate) {
        destTable = QVector<QStringList>();
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList fields = line.split(u'\t');

            if (fields.size() < 2) {
                msgBox.setText("新表有一行少于两个值");
                goto errorCloseFiles;
            }

            destTable.append({fields[0], fields[1]});
            finishCount++;
        }
    } else {
        while (!in.atEnd()) {
            in.readLine();
            finishCount++;
        }
    }
    destFile.close();

    in.setDevice(&sourceFile);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.isEmpty()) {
            msgBox.setText("原表有一行是空的");
            goto errorCloseFiles;
        }

        QStringList fields = line.split(u'\t');
        sourceList.append(line.split(u'\t')[0]);
    }
    sourceFile.close();
    sourceListSize = sourceList.size();

    if (finishCount >= sourceListSize) {
        msgBox.setText(
            QString("新表大于等于原表\n原表：%1\n新表：%2")
                .arg(sourceListSize)
                .arg(finishCount)
        );
        goto loadFilesError;
    }

    destFile.setFileName(destFilePath);
    if (!destFile.open(QIODevice::Append | QIODevice::Text)) {
        msgBox.setText("无法打开新表");
        goto errorCloseFiles;
    }
    destStream.setDevice(&destFile);

    return true;

errorCloseFiles:
    sourceFile.close();
    destFile.close();

loadFilesError:
    msgBox.exec();
    return false;
}

void MainWindow::updateEncodePage() {
    ui->processLabel->setText(
        QString("%1/%2").arg(finishCount).arg(sourceListSize)
    );

    ui->textBrowser->clear();
    ui->textBrowser->setAlignment(Qt::AlignHCenter);
    for (QFont font : fonts) {
        ui->textBrowser->setCurrentFont(font);
        ui->textBrowser->append(
            sourceList[finishCount]
        );
    }
}

void MainWindow::handleInputChange(QString input) {
    if (maxCodeLength == noMaxLen)
        return;

    if (input.length() >= maxCodeLength)
        nextEntry();
}

void MainWindow::nextEntry() {
    QString input = ui->inputBar->text();
    QMessageBox msgBox;

    if (input.isEmpty()) {
        msgBox.setText("输入框为空");
        msgBox.exec();
        return;
    }

    if (banDuplicate) {
        for (int i=0; i < sourceListSize; i++) {
            QString text = destTable[i][0];
            QString code = destTable[i][1];

            if (input == code) {
                msgBox.setText(
                    QString("重码：\n%1").arg(text)
                );
                msgBox.exec();
                return;
            }
        }

        destTable.append({sourceList[finishCount], input});
    }

    destStream << sourceList[finishCount] << '\t'
               << input << '\n';
    ui->inputBar->clear();

    finishCount++;
    if (finishCount >= sourceListSize) {
        msgBox.setText("输码完成");
        msgBox.exec();
        destFile.close();
        exit(0);
    }

    updateEncodePage();
}

FontListModel::FontListModel(QList<QFont> list) {
    fonts = list;
}

FontListModel::FontListModel(QObject *parent) {
    fonts = QList<QFont>();
}

int FontListModel::rowCount(const QModelIndex &parent) const {
    return fonts.size();
}

QVariant FontListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();
    if (role == Qt::DisplayRole) {
        QFont font = fonts[index.row()];
        return QString("%1 %2").arg(font.family()).arg(font.pointSize());
    }
    return QVariant();
}

void MainWindow::on_addFontBut_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, this);

    if (ok) {
        fontlistmodel->append(font);
    }
}

void FontListModel::append(QFont font) {
    fonts.append(font);
    emit dataChanged(QModelIndex(), QModelIndex());
}

QList<QFont> FontListModel::getFonts() const {
    return fonts;
}

void FontListModel::setFonts(QList<QFont> list) {
    fonts = list;
    emit dataChanged(QModelIndex(), QModelIndex());
}

void MainWindow::on_removeFontBut_clicked()
{
    return;
}

void MainWindow::on_finishSelBut_clicked()
{
    bool ok;

    ok = loadSelection();
    if (!ok)
        return;

    ok = loadFiles();
    if (!ok)
        return;

    updateEncodePage();
    ui->stackedWidget->setCurrentWidget(ui->encodePage);
}
