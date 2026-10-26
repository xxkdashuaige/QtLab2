#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPointer>

class QAction;
class QCloseEvent;
class QDialog;
class QLineEdit;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void openFile();
    bool saveFile();
    bool saveFileAs();
    void updateWindowTitle();
    void updateActions();
    void showFindDialog();
    void showReplaceDialog();
    void findNext();
    void replaceOne();
    void replaceAll();
    void chooseFont();
    void chooseTextColor();
    void chooseBackgroundColor();
    void toggleWordWrap(bool enabled);
    void updateStatus();
    void showAbout();

private:
    bool maybeSave();
    bool loadFile(const QString &fileName);
    bool writeFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);
    void createActions();
    void createMenusAndToolBar();
    void createFindDialog();
    void createReplaceDialog();
    void showNotFoundMessage(const QString &text);

    QTextEdit *editor = nullptr;
    QString currentFile;
    QString lastSearchText;
    QDialog *findDialog = nullptr;
    QLineEdit *findEdit = nullptr;
    QDialog *replaceDialog = nullptr;
    QLineEdit *replaceFindEdit = nullptr;
    QLineEdit *replaceWithEdit = nullptr;
    QLabel *statusLabel = nullptr;

    QAction *saveAct = nullptr;
    QAction *saveAsAct = nullptr;
    QAction *cutAct = nullptr;
    QAction *copyAct = nullptr;
    QAction *pasteAct = nullptr;
    QAction *undoAct = nullptr;
    QAction *redoAct = nullptr;
    QAction *findAct = nullptr;
    QAction *replaceAct = nullptr;
    QAction *wordWrapAct = nullptr;
    QAction *toolBarAct = nullptr;
    QAction *statusBarAct = nullptr;
};

#endif // MAINWINDOW_H
