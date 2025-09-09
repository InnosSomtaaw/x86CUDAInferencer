#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QtWidgets>
#include <QtXml/QDomDocument>
#include <QTextStream>

#include "Camera/videoplayer.h"
#include "Camera/hwdecoder.h"
#include "ImageProcess/image_processing.h"
#include "ImageProcess/trtinferencer.h"
//#include "pictureview.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
using namespace cv;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool isDetecting,autoStart;
    std::array<QElapsedTimer,4> timers;

private:
        //显示窗体初始化
        void initImageBoxes();
        //初始化程序工况
        void initWorkCondition();
        //文本输出初始化
        void initTextBrowsers();
        //初始化参数设置
        void initBaseSettings();
        //图像处理类初始化
        void initImageProcess();
        //初始化网络相机设置
        void initCamSettings();
        //数字相机类初始化
        void initVideoPlayers();

        Ui::MainWindow *ui;
        std::array<Mat,4> img_inputs,img_outputs;

        //网络相机参数
        std::array<QString,4> urls;
        //AI工程相关文件路径
        std::array<QString,4> proj_paths;

        vector<Point2i> basePoints;
        WorkConditionsEnum workCond;
        QSettings *iniRW;

        int froceLag=0;
//        //数字相机类（ffmpeg读取）
//        std::array<VideoPlayer*,4> cams;
        //数字相机类（cuda读取）
        std::array<hwDecoder*,4> cams;
        //数字相机线程
        std::array<QThread*,4> camProThds;

        //图像处理类
        std::array<Image_Processing_Class*,4> imgProcessors;
        std::array<trtInferencer*,4> imgTrts;

        //图像处理线程
        std::array<QThread*,4> imgProThds;
        //图像保存类
        std::array<ImageWriter*, 4> iwts;

        std::array<QGraphicsScene,4> scenes;
        std::array<QGraphicsPixmapItem,4> pixmapShows;
        std::array<QString,4> strOutputs;

private slots:
        //窗体打开图片
        void on_imageboxOpenImage();
        //窗体保存图片
        void on_imageboxSaveImage();
        //选择AI工程和节点按钮槽
        void on_buttonOpenAIProject_clicked();
        //重置按钮槽
        void on_buttonReset_clicked();
        //实时处理按钮槽
        void on_buttonProcess_clicked();
        //开始连续采集按钮槽
        void on_buttonStartCapture_clicked();
        //打开图片序列按钮槽
        void on_buttonOpenImgList_clicked();
        //切换处理算法槽
        void on_condComboBox_activated(int index);

        //图片1刷新槽
        void slotimagebox1Refresh();
        //图片2刷新槽
        void slotimagebox2Refresh();
        //图片3刷新槽
        void slotimagebox3Refresh();
        //图片4刷新槽
        void slotimagebox4Refresh();

        //视频流1获取槽
        void slotGetOneFrame1(QImage img);
        //视频流2获取槽
        void slotGetOneFrame2(QImage img);
        //视频流3获取槽
        void slotGetOneFrame3(QImage img);
        //视频流4获取槽
        void slotGetOneFrame4(QImage img);

signals:
        //开始数字相机信号
        void startCam1Request();
        void startCam2Request();
        void startCam3Request();
        void startCam4Request();
        //开始单次处理信号
        void procCam1Request();
        void procCam2Request();
        void procCam3Request();
        void procCam4Request();

};
#endif // MAINWINDOW_H
