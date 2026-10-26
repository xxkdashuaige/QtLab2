#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QPushButton>
#include <QPalette>
#include <QClipboard>
#include <QFileDialog>
#include <QFontDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTextStream>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QTextBlockFormat>
#include <QTextOption>
#include <QKeySequence>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    editor = new QTextEdit(this);
    editor->setAcceptRichText(false);
    editor->setPlaceholderText(QStringLiteral("在此输入文本……"));
    editor->setFont(QFont(QStringLiteral("Microsoft YaHei"), 11));
    setCentralWidget(editor);
    resize(950, 650);

    createActions();
    createMenusAndToolBar();
    createFindDialog();
    createReplaceDialog();

    statusLabel = new QLabel(this);
    statusBar()->addPermanentWidget(statusLabel);
    connect(editor->document(), &QTextDocument::modificationChanged,
            this, &MainWindow::updateWindowTitle);
    connect(editor, &QTextEdit::copyAvailable, this, [this](bool available) {
        cutAct->setEnabled(available);
        copyAct->setEnabled(available);
    });
    connect(editor, &QTextEdit::undoAvailable, undoAct, &QAction::setEnabled);
    connect(editor, &QTextEdit::redoAvailable, redoAct, &QAction::setEnabled);
    connect(editor, &QTextEdit::textChanged, this, &MainWindow::updateStatus);
    connect(editor, &QTextEdit::cursorPositionChanged, this, &MainWindow::updateStatus);
    connect(editor, &QTextEdit::selectionChanged, this, [this]() {
        const bool selected = editor->textCursor().hasSelection();
        cutAct->setEnabled(selected);
        copyAct->setEnabled(selected);
    });

    setCurrentFile(QString());
    updateActions();
    updateStatus();
}

void MainWindow::createActions()
{
    auto makeAction = [this](const QString &text, const QKeySequence &shortcut,
                             const QObject *receiver, const char *slot) {
        QAction *action = new QAction(text, this);
        if (!shortcut.isEmpty()) action->setShortcut(shortcut);
        connect(action, SIGNAL(triggered()), receiver, slot);
        return action;
    };

    QAction *newAct = makeAction(QStringLiteral("新建(&N)"), QKeySequence::New, this, SLOT(newFile()));
    QAction *openAct = makeAction(QStringLiteral("打开(&O)..."), QKeySequence::Open, this, SLOT(openFile()));
    saveAct = makeAction(QStringLiteral("保存(&S)"), QKeySequence::Save, this, SLOT(saveFile()));
    saveAsAct = makeAction(QStringLiteral("另存为(&A)..."), QKeySequence::SaveAs, this, SLOT(saveFileAs()));
    QAction *exitAct = makeAction(QStringLiteral("退出(&X)"), QKeySequence::Quit, this, SLOT(close()));

    undoAct = makeAction(QStringLiteral("撤销(&U)"), QKeySequence::Undo, editor, SLOT(undo()));
    redoAct = makeAction(QStringLiteral("重做(&R)"), QKeySequence::Redo, editor, SLOT(redo()));
    cutAct = makeAction(QStringLiteral("剪切(&T)"), QKeySequence::Cut, editor, SLOT(cut()));
    copyAct = makeAction(QStringLiteral("复制(&C)"), QKeySequence::Copy, editor, SLOT(copy()));
    pasteAct = makeAction(QStringLiteral("粘贴(&P)"), QKeySequence::Paste, editor, SLOT(paste()));
    QAction *selectAllAct = makeAction(QStringLiteral("全选(&A)"), QKeySequence::SelectAll, editor, SLOT(selectAll()));
    findAct = makeAction(QStringLiteral("查找(&F)..."), QKeySequence::Find, this, SLOT(showFindDialog()));
    replaceAct = makeAction(QStringLiteral("替换(&H)..."), QKeySequence::Replace, this, SLOT(showReplaceDialog()));

    QAction *fontAct = new QAction(QStringLiteral("设置字体..."), this);
    connect(fontAct, &QAction::triggered, this, &MainWindow::chooseFont);
    QAction *textColorAct = new QAction(QStringLiteral("字体颜色..."), this);
    connect(textColorAct, &QAction::triggered, this, &MainWindow::chooseTextColor);
    QAction *backgroundAct = new QAction(QStringLiteral("编辑区背景色..."), this);
    connect(backgroundAct, &QAction::triggered, this, &MainWindow::chooseBackgroundColor);

    wordWrapAct = new QAction(QStringLiteral("自动换行"), this);
    wordWrapAct->setCheckable(true);
    wordWrapAct->setChecked(true);
    connect(wordWrapAct, &QAction::toggled, this, &MainWindow::toggleWordWrap);
    toolBarAct = new QAction(QStringLiteral("显示工具栏"), this);
    toolBarAct->setCheckable(true);
    toolBarAct->setChecked(true);
    statusBarAct = new QAction(QStringLiteral("显示状态栏"), this);
    statusBarAct->setCheckable(true);
    statusBarAct->setChecked(true);
    connect(toolBarAct, &QAction::toggled, this, [this](bool visible) {
        for (QToolBar *bar : findChildren<QToolBar *>()) bar->setVisible(visible);
    });
    connect(statusBarAct, &QAction::toggled, statusBar(), &QStatusBar::setVisible);

    QAction *aboutAct = new QAction(QStringLiteral("关于(&A)"), this);
    connect(aboutAct, &QAction::triggered, this, &MainWindow::showAbout);

    // Store actions on the window so menus and toolbar share the same QAction instances.
    newAct->setObjectName("newAct"); openAct->setObjectName("openAct");
    saveAct->setObjectName("saveAct"); saveAsAct->setObjectName("saveAsAct");
    exitAct->setObjectName("exitAct"); selectAllAct->setObjectName("selectAllAct");
    fontAct->setObjectName("fontAct"); textColorAct->setObjectName("textColorAct");
    backgroundAct->setObjectName("backgroundAct"); aboutAct->setObjectName("aboutAct");
}

void MainWindow::createMenusAndToolBar()
{
    QMenu *fileMenu = menuBar()->addMenu(QStringLiteral("文件(&F)"));
    for (const QString &name : {"newAct", "openAct", "saveAct", "saveAsAct"}) {
        QAction *a = findChild<QAction *>(name); if (a) fileMenu->addAction(a);
    }
    fileMenu->addSeparator();
    fileMenu->addAction(findChild<QAction *>("exitAct"));

    QMenu *editMenu = menuBar()->addMenu(QStringLiteral("编辑(&E)"));
    editMenu->addAction(undoAct); editMenu->addAction(redoAct); editMenu->addSeparator();
    editMenu->addAction(cutAct); editMenu->addAction(copyAct); editMenu->addAction(pasteAct);
    editMenu->addAction(findChild<QAction *>("selectAllAct")); editMenu->addSeparator();
    editMenu->addAction(findAct); editMenu->addAction(replaceAct);

    QMenu *formatMenu = menuBar()->addMenu(QStringLiteral("格式(&O)"));
    formatMenu->addAction(findChild<QAction *>("fontAct"));
    formatMenu->addAction(findChild<QAction *>("textColorAct"));
    formatMenu->addAction(findChild<QAction *>("backgroundAct"));
    formatMenu->addSeparator(); formatMenu->addAction(wordWrapAct);

    QMenu *viewMenu = menuBar()->addMenu(QStringLiteral("视图(&V)"));
    viewMenu->addAction(toolBarAct); viewMenu->addAction(statusBarAct);
    QMenu *helpMenu = menuBar()->addMenu(QStringLiteral("帮助(&H)"));
    helpMenu->addAction(findChild<QAction *>("aboutAct"));

    QToolBar *toolBar = addToolBar(QStringLiteral("常用工具"));
    toolBar->setObjectName("mainToolBar");
    toolBar->setMovable(false);
    toolBar->addAction(findChild<QAction *>("newAct"));
    toolBar->addAction(findChild<QAction *>("openAct"));
    toolBar->addAction(saveAct); toolBar->addSeparator();
    toolBar->addAction(undoAct); toolBar->addAction(redoAct); toolBar->addSeparator();
    toolBar->addAction(cutAct); toolBar->addAction(copyAct); toolBar->addAction(pasteAct);
    toolBar->addSeparator(); toolBar->addAction(findAct); toolBar->addAction(replaceAct);
}

void MainWindow::createFindDialog()
{
    findDialog = new QDialog(this);
    findDialog->setWindowTitle(QStringLiteral("查找"));
    findDialog->setModal(false);
    findEdit = new QLineEdit(findDialog);
    findEdit->setPlaceholderText(QStringLiteral("输入要查找的内容"));
    auto *findButton = new QPushButton(QStringLiteral("查找下一个"), findDialog);
    auto *closeButton = new QPushButton(QStringLiteral("关闭"), findDialog);
    auto *row = new QHBoxLayout; row->addWidget(findButton); row->addWidget(closeButton);
    auto *layout = new QVBoxLayout(findDialog); layout->addWidget(new QLabel(QStringLiteral("查找内容："), findDialog));
    layout->addWidget(findEdit); layout->addLayout(row);
    connect(findButton, &QPushButton::clicked, this, &MainWindow::findNext);
    connect(findEdit, &QLineEdit::returnPressed, this, &MainWindow::findNext);
    connect(closeButton, &QPushButton::clicked, findDialog, &QDialog::hide);
    findDialog->resize(360, 110);
}

void MainWindow::createReplaceDialog()
{
    replaceDialog = new QDialog(this);
    replaceDialog->setWindowTitle(QStringLiteral("查找与替换"));
    replaceDialog->setModal(false);
    replaceFindEdit = new QLineEdit(replaceDialog);
    replaceWithEdit = new QLineEdit(replaceDialog);
    replaceFindEdit->setPlaceholderText(QStringLiteral("要查找的内容"));
    replaceWithEdit->setPlaceholderText(QStringLiteral("替换为"));
    auto *oneButton = new QPushButton(QStringLiteral("替换"), replaceDialog);
    auto *allButton = new QPushButton(QStringLiteral("全部替换"), replaceDialog);
    auto *closeButton = new QPushButton(QStringLiteral("关闭"), replaceDialog);
    auto *buttons = new QHBoxLayout; buttons->addWidget(oneButton); buttons->addWidget(allButton); buttons->addWidget(closeButton);
    auto *layout = new QVBoxLayout(replaceDialog);
    layout->addWidget(new QLabel(QStringLiteral("查找内容："), replaceDialog)); layout->addWidget(replaceFindEdit);
    layout->addWidget(new QLabel(QStringLiteral("替换为："), replaceDialog)); layout->addWidget(replaceWithEdit); layout->addLayout(buttons);
    connect(oneButton, &QPushButton::clicked, this, &MainWindow::replaceOne);
    connect(allButton, &QPushButton::clicked, this, &MainWindow::replaceAll);
    connect(closeButton, &QPushButton::clicked, replaceDialog, &QDialog::hide);
    replaceDialog->resize(400, 180);
}

void MainWindow::newFile()
{
    if (!maybeSave()) return;
    editor->clear();
    editor->document()->setModified(false);
    setCurrentFile(QString());
}

void MainWindow::openFile()
{
    if (!maybeSave()) return;
    const QString fileName = QFileDialog::getOpenFileName(this, QStringLiteral("打开文件"), QString(),
        QStringLiteral("文本文件 (*.txt *.md *.cpp *.h *.log);;所有文件 (*.*)"));
    if (!fileName.isEmpty()) loadFile(fileName);
}

bool MainWindow::saveFile()
{
    if (currentFile.isEmpty()) return saveFileAs();
    return writeFile(currentFile);
}

bool MainWindow::saveFileAs()
{
    const QString fileName = QFileDialog::getSaveFileName(this, QStringLiteral("另存为"), currentFile,
        QStringLiteral("文本文件 (*.txt);;所有文件 (*.*)"));
    if (fileName.isEmpty()) return false;
    return writeFile(fileName);
}

bool MainWindow::maybeSave()
{
    if (!editor->document()->isModified()) return true;
    const auto answer = QMessageBox::warning(this, QStringLiteral("文档尚未保存"),
        QStringLiteral("当前文档已修改，是否保存？"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Save) return saveFile();
    if (answer == QMessageBox::Cancel) return false;
    return true;
}

bool MainWindow::loadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("打开失败"),
            QStringLiteral("无法打开文件：\n%1\n原因：%2").arg(fileName, file.errorString()));
        return false;
    }
    QTextStream in(&file);
    editor->setPlainText(in.readAll());
    editor->document()->setModified(false);
    setCurrentFile(fileName);
    statusBar()->showMessage(QStringLiteral("已打开：%1").arg(fileName), 3000);
    return true;
}

bool MainWindow::writeFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"),
            QStringLiteral("无法保存文件：\n%1\n原因：%2").arg(fileName, file.errorString()));
        return false;
    }
    QTextStream out(&file);
    out << editor->toPlainText();
    if (file.error() != QFileDevice::NoError) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("写入文件时发生错误。"));
        return false;
    }
    editor->document()->setModified(false);
    setCurrentFile(fileName);
    statusBar()->showMessage(QStringLiteral("保存成功"), 3000);
    return true;
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    currentFile = fileName;
    editor->document()->setModified(false);
    updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    const QString name = currentFile.isEmpty() ? QStringLiteral("未命名") : QFileInfo(currentFile).fileName();
    setWindowTitle(QStringLiteral("%1%2 - 简单文本编辑器")
        .arg(editor->document()->isModified() ? QStringLiteral("*") : QString(), name));
    saveAct->setEnabled(editor->document()->isModified() || currentFile.isEmpty());
}

void MainWindow::updateActions()
{
    undoAct->setEnabled(editor->document()->isUndoAvailable());
    redoAct->setEnabled(editor->document()->isRedoAvailable());
    cutAct->setEnabled(false); copyAct->setEnabled(false);
    pasteAct->setEnabled(!QApplication::clipboard()->text().isEmpty());
}

void MainWindow::showFindDialog()
{
    if (!editor->textCursor().selectedText().isEmpty()) findEdit->setText(editor->textCursor().selectedText());
    findDialog->show(); findDialog->raise(); findDialog->activateWindow(); findEdit->setFocus(); findEdit->selectAll();
}

void MainWindow::showReplaceDialog()
{
    if (!editor->textCursor().selectedText().isEmpty()) replaceFindEdit->setText(editor->textCursor().selectedText());
    replaceDialog->show(); replaceDialog->raise(); replaceDialog->activateWindow(); replaceFindEdit->setFocus();
}

void MainWindow::findNext()
{
    const QString query = findEdit->text();
    if (query.isEmpty()) { QMessageBox::information(findDialog, QStringLiteral("查找"), QStringLiteral("请先输入查找内容。")); return; }
    lastSearchText = query;
    if (!editor->find(query)) {
        QTextCursor cursor = editor->textCursor(); cursor.movePosition(QTextCursor::Start); editor->setTextCursor(cursor);
        if (!editor->find(query)) showNotFoundMessage(query);
    }
}

void MainWindow::replaceOne()
{
    const QString query = replaceFindEdit->text();
    if (query.isEmpty()) { QMessageBox::information(replaceDialog, QStringLiteral("替换"), QStringLiteral("请先输入查找内容。")); return; }
    QTextCursor cursor = editor->textCursor();
    if (cursor.hasSelection() && cursor.selectedText() == query) {
        cursor.insertText(replaceWithEdit->text());
    } else if (!editor->find(query)) {
        QTextCursor start = editor->textCursor(); start.movePosition(QTextCursor::Start); editor->setTextCursor(start);
        if (!editor->find(query)) { showNotFoundMessage(query); return; }
    }
    if (editor->textCursor().hasSelection()) editor->textCursor().insertText(replaceWithEdit->text());
}

void MainWindow::replaceAll()
{
    const QString query = replaceFindEdit->text();
    if (query.isEmpty()) { QMessageBox::information(replaceDialog, QStringLiteral("替换"), QStringLiteral("请先输入查找内容。")); return; }
    QString content = editor->toPlainText();
    const qsizetype count = content.count(query);
    if (count == 0) { showNotFoundMessage(query); return; }
    content.replace(query, replaceWithEdit->text());
    editor->setPlainText(content);
    statusBar()->showMessage(QStringLiteral("已替换 %1 处。 ").arg(count), 3000);
}

void MainWindow::showNotFoundMessage(const QString &text)
{
    QMessageBox::information(this, QStringLiteral("查找结果"), QStringLiteral("未找到“%1”。").arg(text));
}

void MainWindow::chooseFont()
{
    bool ok = false;
    const QFont font = QFontDialog::getFont(&ok, editor->font(), this, QStringLiteral("选择字体"));
    if (ok) editor->setFont(font);
}

void MainWindow::chooseTextColor()
{
    const QColor color = QColorDialog::getColor(editor->textColor(), this, QStringLiteral("选择字体颜色"));
    if (color.isValid()) editor->setTextColor(color);
}

void MainWindow::chooseBackgroundColor()
{
    const QColor color = QColorDialog::getColor(editor->palette().color(QPalette::Base), this, QStringLiteral("选择编辑区背景色"));
    if (color.isValid()) {
        QPalette palette = editor->palette(); palette.setColor(QPalette::Base, color); editor->setPalette(palette);
    }
}

void MainWindow::toggleWordWrap(bool enabled)
{
    editor->setLineWrapMode(enabled ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);
}

void MainWindow::updateStatus()
{
    const QTextCursor cursor = editor->textCursor();
    statusLabel->setText(QStringLiteral("行 %1，列 %2 | 字符数：%3")
        .arg(cursor.blockNumber() + 1).arg(cursor.positionInBlock() + 1).arg(editor->toPlainText().size()));
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, QStringLiteral("关于简单文本编辑器"),
        QStringLiteral("简单文本编辑器\n基于 Qt Widgets 实现，支持文件读写、编辑、查找替换和格式设置。"));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) event->accept(); else event->ignore();
}
