#include <QtTest>
#include <QApplication>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QTimer>
#include <QElapsedTimer>
#include <QPainter>
#include <QFile>
#include <QMessageBox>
#include <QDialog>
#include <QLineEdit>
#include <QDir>
#ifdef LAB1
#include "calculatorwindow.h"
#else
#include "editorwindow.h"
#include "codeeditor.h"
#endif
int main(int argc,char **argv) {
    QApplication app(argc,argv);
    if (argc < 4) return 2;
    QString output=QString::fromLocal8Bit(argv[1]), ffmpeg=QString::fromLocal8Bit(argv[2]), repo=QString::fromLocal8Bit(argv[3]);
    QWidget canvas; canvas.setFixedSize(1200,800);
    auto *outer=new QVBoxLayout(&canvas);
    auto *title=new QLabel; title->setStyleSheet("font-size:24px;font-weight:bold;padding:10px;"); outer->addWidget(title);
    auto *row=new QHBoxLayout; outer->addLayout(row,1);
#ifdef LAB1
    CalculatorWindow program; program.setFixedWidth(400);
    title->setText("实验一 带键盘事件的计算器  郭毅川 2024414290411");
#else
    EditorWindow program;
    title->setText("实验二 简单文本编辑器  郭毅川 2024414290411");
#endif
    row->addWidget(&program,2);
    auto *notes=new QPlainTextEdit; notes->setReadOnly(true); notes->setMinimumWidth(310);
    notes->setStyleSheet("font-family:monospace;font-size:16px;background:#edf2f8;padding:10px;"); row->addWidget(notes,1);
    auto *caption=new QLabel; caption->setWordWrap(true);caption->setMinimumHeight(65);
    caption->setStyleSheet("font-size:19px;padding:8px;background:#e6effb;color:#123960;");outer->addWidget(caption);
    canvas.show(); program.show();
    QProcess encoder; encoder.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    encoder.start(ffmpeg,{"-y","-loglevel","error","-f","rawvideo","-pixel_format","rgb24","-video_size","1200x800","-framerate","5","-i","pipe:0","-an","-c:v","libx264","-preset","veryfast","-crf","23","-pix_fmt","yuv420p","-movflags","+faststart",output});
    if(!encoder.waitForStarted()) return 3;
    QElapsedTimer elapsed; elapsed.start(); int stage=-1, frames=0;
    QTimer timer; timer.setInterval(200);
    QObject::connect(&timer,&QTimer::timeout,&canvas,[&] {
        int t=elapsed.elapsed()/1000;
        int next=t/10;
        if(next!=stage) {
            stage=next;
#ifdef LAB1
            auto click=[&](const char *name){QTest::mouseClick(program.findChild<QPushButton*>(name),Qt::LeftButton);};
            if(stage==0){caption->setText("连续自动演示：真实 Qt 程序，鼠标与键盘事件共用计算状态机。");notes->setPlainText("验证环境\nLinux / Qt 5.15.3\n\n演示顺序\n小数鼠标计算\n键盘输入\n退格与清除\n除零与恢复\n连续运算\n重复小数点\n负小数\nGit 提交历史");}
            if(stage==1){caption->setText("鼠标点击 12.5 + 7.25 = ，结果应为 19.75。");click("clearButton");for(auto n:{"digit1Button","digit2Button","decimalButton","digit5Button","plusButton","digit7Button","decimalButton","digit2Button","digit5Button","equalsButton"})click(n);}
            if(stage==2){caption->setText("键盘输入 9 / 4 后按 Enter，结果 2.25。");QTest::keyClick(&program,Qt::Key_Escape);QTest::keyClicks(&program,"9/4");QTest::keyClick(&program,Qt::Key_Return);}
            if(stage==3){caption->setText("键盘输入 123，再按 Backspace：显示 12。");QTest::keyClick(&program,Qt::Key_Escape);QTest::keyClicks(&program,"123");QTest::keyClick(&program,Qt::Key_Backspace);}
            if(stage==4){caption->setText("按 Esc 清除，再输入 7 - 2 =，结果 5。");QTest::keyClick(&program,Qt::Key_Escape);QTest::keyClicks(&program,"7-2=");}
            if(stage==5){caption->setText("除零边界：8 / 0 =，提示不能除以零。");QTest::keyClick(&program,Qt::Key_Escape);QTest::keyClicks(&program,"8/0=");}
            if(stage==6){caption->setText("错误恢复：直接输入 2 + 3 =，重新开始计算，结果 5。");QTest::keyClicks(&program,"2+3=");}
            if(stage==7){caption->setText("连续运算采用即时计算：2 + 3 * 4 = 20。");QTest::keyClick(&program,Qt::Key_Escape);QTest::keyClicks(&program,"2+3*4=");}
            if(stage==8){caption->setText("结果继续运算：按 + 2 =，得到 22；运算符可接续结果。");QTest::keyClicks(&program,"+2=");}
            if(stage==9){caption->setText("异常输入：1..2 + 3 =，第二个小数点被忽略，结果 4.2。");QTest::keyClick(&program,Qt::Key_Escape);QTest::keyClicks(&program,"1..2+3=");}
            if(stage==10){caption->setText("回归验证：先按 ± 再输入 .5 + 1 =，得到 0.5。");click("clearButton");click("signButton");QTest::keyClicks(&program,".5+1=");}
#else
            auto *edit=program.editor();
            if(stage==0){caption->setText("连续自动演示：真实 Qt 编辑器；文件测试调用与菜单共用的读写函数。");notes->setPlainText("验证环境\nLinux / Qt 5.15.3\n\n演示顺序\n新建与编辑\nUTF-8 保存及打开\n查找与替换\n撤销和重做\n异常文件保护\n关闭未保存提示\n行号与换行\nGit 提交历史");}
            if(stage==1){caption->setText("新建空白文档后输入中文与 Qt 文本，标题显示修改标记。");edit->insertPlainText("Qt 应用程序开发\n郭毅川 2024414290411\nQt Widgets\nQt Designer\n");}
            if(stage==2){caption->setText("保存 UTF-8 文本到演示文件；成功后清除修改标记。");program.saveFile(repo+"/evidence/testdata/demo.txt");}
            if(stage==3){caption->setText("重新打开刚保存的文件，验证中文内容与路径恢复。");program.loadFile(repo+"/evidence/testdata/demo.txt");}
            if(stage==4){caption->setText("查找 Qt：非模态查找窗口支持连续查找和循环搜索。");program.showSearch(false);program.findChild<QLineEdit*>("findEdit")->setText("Qt");program.findChild<QPushButton*>("findNextButton")->click();}
            if(stage==5){caption->setText("全部替换 Qt 为 Qt6，显示替换数量。");program.showSearch(true);program.findChild<QLineEdit*>("replaceEdit")->setText("Qt6");program.findChild<QPushButton*>("replaceAllButton")->click();}
            if(stage==6){caption->setText("撤销全部替换：一次恢复所有匹配位置。");program.findChild<QDialog*>("searchDialog")->hide();edit->undo();}
            if(stage==7){caption->setText("重做全部替换：再次得到 Qt6 文本。");edit->redo();}
            if(stage==8){caption->setText("打开不存在的文件：错误提示出现，原文档保留。");QTimer::singleShot(4500,[]{if(auto *b=qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))b->accept();});program.loadFile(repo+"/evidence/testdata/not-found.txt");}
            if(stage==9){caption->setText("关闭未保存文档出现保存／放弃／取消提示，选择取消保留文档。");QTimer::singleShot(4500,[]{if(auto *b=qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))b->button(QMessageBox::Cancel)->click();});program.maybeSave();}
            if(stage==10){caption->setText("关闭自动换行并切换行号，行号区域随设置更新。");program.findChild<QAction*>("actionWrap")->setChecked(false);program.findChild<QAction*>("actionLineNumbers")->setChecked(false);QTimer::singleShot(4000,[&]{program.findChild<QAction*>("actionLineNumbers")->setChecked(true);});}
#endif
            if(stage==11){QProcess git;git.setWorkingDirectory(repo);git.start("git",{"--no-pager","log","-8","--oneline"});git.waitForFinished();notes->setPlainText("真实 Git 提交历史\n\n"+QString::fromUtf8(git.readAllStandardOutput()));caption->setText("展示实际 Git 提交记录：功能开发、自动化测试、缺陷定位与修复。");}
            if(stage==12){caption->setText("演示结束。原始测试日志、截图、源码和实验报告分别保存在本实验私有仓库。");}
        }
    });
    QTimer frameTimer; frameTimer.setInterval(200);
    QObject::connect(&frameTimer,&QTimer::timeout,&canvas,[&] {
        int t=elapsed.elapsed()/1000;
        QImage image(canvas.size(),QImage::Format_RGB888);image.fill(Qt::white);canvas.render(&image);
        if(auto *dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget())){
            QPainter p(&image);QPixmap shot=dialog->grab();p.drawPixmap((image.width()-shot.width())/2,(image.height()-shot.height())/2,shot);
        }
        {QPainter p(&image);p.setPen(QColor("#526278"));p.drawText(1030,30,QString("%1:%2").arg(t/60,2,10,QChar('0')).arg(t%60,2,10,QChar('0')));}
        encoder.write(reinterpret_cast<const char*>(image.constBits()),image.sizeInBytes());++frames;
        if(t>=130){timer.stop();frameTimer.stop();encoder.closeWriteChannel();encoder.waitForFinished(30000);
#ifndef LAB1
            program.editor()->document()->setModified(false);
#endif
            app.quit();}
    });
    timer.start();frameTimer.start();int result=app.exec();program.setParent(nullptr);return result;
}
