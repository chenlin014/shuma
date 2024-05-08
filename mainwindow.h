#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <string>

#include <QMainWindow>
#include <QSettings>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QAbstractItemModel>
#include <QFontDialog>
#include <QTextStream>

#define noMaxLen 0

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

struct ShumaSession {
    QString sourceFilePath;
    QString destFilePath;
    QList<QFont> fonts;
    quint32 maxCodeLength;
    bool banDuplicate;
};

Q_DECLARE_METATYPE(ShumaSession)

class FontListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    FontListModel(QObject * parent = 0);
    FontListModel(QList<QFont> fonts);
    int rowCount(const QModelIndex & parent) const;
    QVariant data(const QModelIndex &index,
                    int role = Qt::DisplayRole) const;

    void append(QFont);
    QList<QFont> getFonts() const;
    void setFonts(QList<QFont>);

private:
    QList<QFont> fonts;
};

class MainWindow : public QMainWindow, public ShumaSession
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void selectSourceFile();
    void selectDestFile();
    void nextEntry();
    void handleInputChange(QString input);
    void on_maxCodeLenCB_clicked();
    void on_addFontBut_clicked();
    void on_removeFontBut_clicked();
    void on_finishSelBut_clicked();

private:
    Ui::MainWindow *ui;
    FontListModel *fontlistmodel;

    QStringList sourceList;
    uint sourceListSize;
    QFile destFile;
    QTextStream destStream;
    QVector<QStringList> destTable;
    uint finishCount;

    void loadSession(ShumaSession);
    bool loadSelection();
    bool loadFiles();
    void updateEncodePage();
};
#endif // MAINWINDOW_H
