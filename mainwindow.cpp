#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //显示窗体初始化
    initImageBoxes();
    //初始化程序工况
    initWorkCondition();
    //文本输出初始化
    initTextBrowsers();
    //初始化参数设置
    initBaseSettings();
    //图像处理类初始化
    initImageProcess();
    //初始化网络相机设置
    initCamSettings();
    //数字相机类初始化
    initVideoPlayers();
}

MainWindow::~MainWindow()
{
    for(int i=0;i<cams.size();i++)
        cams[i]->isCapturing=0;

    delete ui;
}

void MainWindow::initImageBoxes()
{
    ui->imagebox1->setScene(&scenes[0]);
    ui->imagebox2->setScene(&scenes[1]);
    ui->imagebox3->setScene(&scenes[2]);
    ui->imagebox4->setScene(&scenes[3]);
    for(int i=0;i<scenes.size();i++)
        scenes[i].addItem(&pixmapShows[i]);

    connect(this->ui->imagebox1,&PictureView::loadImgRequest,
            this,&MainWindow::on_imageboxOpenImage);
    connect(this->ui->imagebox2,&PictureView::loadImgRequest,
            this,&MainWindow::on_imageboxOpenImage);
    connect(this->ui->imagebox1,&PictureView::saveImgRequest,
            this,&MainWindow::on_imageboxSaveImage);
    connect(this->ui->imagebox2,&PictureView::saveImgRequest,
            this,&MainWindow::on_imageboxSaveImage);
    connect(this->ui->imagebox3,&PictureView::loadImgRequest,
            this,&MainWindow::on_imageboxOpenImage);
    connect(this->ui->imagebox3,&PictureView::saveImgRequest,
            this,&MainWindow::on_imageboxSaveImage);
    connect(this->ui->imagebox4,&PictureView::loadImgRequest,
            this,&MainWindow::on_imageboxOpenImage);
    connect(this->ui->imagebox4,&PictureView::saveImgRequest,
            this,&MainWindow::on_imageboxSaveImage);
}

void MainWindow::initWorkCondition()
{
    iniRW = new QSettings("LastSettings.ini",QSettings::IniFormat);
    workCond=WorkConditionsEnum(iniRW->value("WorkCondition/WorkCondition").toInt());
    ui->condComboBox->setCurrentIndex(workCond);
    froceLag=iniRW->value("CameraSettings/FroceLag").toInt();
    urls[0]=iniRW->value("Urls/url1").toString();
    urls[1]=iniRW->value("Urls/url2").toString();
    urls[2]=iniRW->value("Urls/url3").toString();
    urls[3]=iniRW->value("Urls/url4").toString();
    proj_paths[0] = iniRW->value("TensorRTInference/ModelPath1").toString();
    proj_paths[1] = iniRW->value("TensorRTInference/ModelPath2").toString();
    proj_paths[2] = iniRW->value("TensorRTInference/ModelPath3").toString();
    proj_paths[3] = iniRW->value("TensorRTInference/ModelPath4").toString();
}

void MainWindow::initTextBrowsers()
{
    ui->textBrowser1->setStyleSheet("background:transparent;border-width:0;border-style:outset");
    ui->textBrowser2->setStyleSheet("background:transparent;border-width:0;border-style:outset");
    ui->textBrowser3->setStyleSheet("background:transparent;border-width:0;border-style:outset");
    ui->textBrowser4->setStyleSheet("background:transparent;border-width:0;border-style:outset");

    for(int i=0;i<strOutputs.size();i++)
        strOutputs[i]="place holder "+QString::number(i)+" ...\n";

    ui->textBrowser1->setText(strOutputs[0]);
    ui->textBrowser2->setText(strOutputs[1]);
    ui->textBrowser3->setText(strOutputs[2]);
    ui->textBrowser4->setText(strOutputs[3]);
}

void MainWindow::initBaseSettings()
{
    isDetecting=false;
    QThreadPool::globalInstance()->setMaxThreadCount(12);
}

void MainWindow::initImageProcess()
{
    //trtInferencer类初始化
    for(int i=0;i<imgTrts.size();i++)
    {
        imgTrts[i]=new trtInferencer;

        imgTrts[i]->prog_file=proj_paths[i];
        imgTrts[i]->workCond=workCond;
        imgTrts[i]->iniImgProcessor();

        imgProThds[i]=new QThread;
        imgTrts[i]->moveToThread(imgProThds[i]);
        imgProThds[i]->start();

        imgProcessors[i]=imgTrts[i];
        imgProcessors[i]->ipcMutex.unlock();

        if(imgTrts[i]->hasInited)
            ui->buttonOpenAIProject->setEnabled(false);
    }
    connect(this,&MainWindow::procCam1Request,
            imgTrts[0],&trtInferencer::startProcessOnce);
    connect(imgTrts[0],&trtInferencer::outputImgProcessedRequest,
            this,&MainWindow::slotimagebox1Refresh);

    connect(this,&MainWindow::procCam2Request,
            imgTrts[1],&trtInferencer::startProcessOnce);
    connect(imgTrts[1],&trtInferencer::outputImgProcessedRequest,
            this,&MainWindow::slotimagebox2Refresh);

    connect(this,&MainWindow::procCam3Request,
            imgTrts[2],&trtInferencer::startProcessOnce);
    connect(imgTrts[2],&trtInferencer::outputImgProcessedRequest,
            this,&MainWindow::slotimagebox3Refresh);

    connect(this,&MainWindow::procCam4Request,
            imgTrts[3],&trtInferencer::startProcessOnce);
    connect(imgTrts[3],&trtInferencer::outputImgProcessedRequest,
            this,&MainWindow::slotimagebox4Refresh);
}

void MainWindow::initCamSettings()
{
    for(int i=0;i<iwts.size();i++)
    {
        iwts[i]=new ImageWriter;
        //保存方式：0-png;1-bmp;2-jpg;
        iwts[i]->method=2;
        iwts[i]->setAutoDelete(false);
    }
}

void MainWindow::initVideoPlayers()
{
    for(int i=0;i<cams.size();i++)
    {
        cams[i]=new hwDecoder;
        cams[i]->camURL=urls[i];
        cams[i]->froceLag=froceLag;
        camProThds[i]=new QThread;
        cams[i]->moveToThread(camProThds[i]);
        camProThds[i]->start();
    }

    connect(this,&MainWindow::startCam1Request,
            cams[0],&hwDecoder::startCamera);
    connect(cams[0],&hwDecoder::sigGetOneFrame,
            this,&MainWindow::slotGetOneFrame1);
    connect(cams[0],&hwDecoder::sigProcessOnce,
            imgTrts[0],&trtInferencer::proQImgOnce);

    connect(this,&MainWindow::startCam2Request,
            cams[1],&hwDecoder::startCamera);
    connect(cams[1],&hwDecoder::sigGetOneFrame,
            this,&MainWindow::slotGetOneFrame2);
    connect(cams[1],&hwDecoder::sigProcessOnce,
            imgTrts[1],&trtInferencer::proQImgOnce);

    connect(this,&MainWindow::startCam3Request,
            cams[2],&hwDecoder::startCamera);
    connect(cams[2],&hwDecoder::sigGetOneFrame,
            this,&MainWindow::slotGetOneFrame3);
    connect(cams[2],&hwDecoder::sigProcessOnce,
            imgTrts[2],&trtInferencer::proQImgOnce);

    connect(this,&MainWindow::startCam4Request,
            cams[3],&hwDecoder::startCamera);
    connect(cams[3],&hwDecoder::sigGetOneFrame,
            this,&MainWindow::slotGetOneFrame4);
    connect(cams[3],&hwDecoder::sigProcessOnce,
            imgTrts[3],&trtInferencer::proQImgOnce);
}


void MainWindow::on_buttonOpenAIProject_clicked()
{
    for(int i=0;i<proj_paths.size();i++)
        proj_paths[i] = QFileDialog::getOpenFileName(this,tr("Open AI Model: "),"./model/",
                                             tr("Model File(*.pdmodel *.onnx)"));
}

void MainWindow::on_imageboxOpenImage()
{
    QString fileName = QFileDialog::getOpenFileName(
                this,tr("Open Image"),".",tr("Image File(*.png *.jpg *.jpeg *.bmp)"));

    if (fileName.isEmpty())
        return;

    QImageReader qir(fileName);
    QImage img=qir.read();

    img=img.convertToFormat(QImage::Format_RGB888);
    Mat srcImage(img.height(), img.width(), CV_8UC3,img.bits(), img.bytesPerLine());

    QObject *ptrSender=sender();
    if(ptrSender==ui->imagebox1)
    {
        cvtColor(srcImage,img_inputs[0],COLOR_BGR2RGB);
        //窗体1显示
        pixmapShows[0].setPixmap(QPixmap::fromImage(img));
        ui->imagebox1->Adapte();
    }
    else if(ptrSender==ui->imagebox2)
    {
        cvtColor(srcImage,img_inputs[1],COLOR_BGR2RGB);
        //窗体2显示
        pixmapShows[1].setPixmap(QPixmap::fromImage(img));
        ui->imagebox2->Adapte();
    }
    else if(ptrSender==ui->imagebox3)
    {
        cvtColor(srcImage,img_inputs[2],COLOR_BGR2RGB);
        //窗体3显示
        pixmapShows[2].setPixmap(QPixmap::fromImage(img));
        ui->imagebox3->Adapte();
    }
    else if(ptrSender==ui->imagebox4)
    {
        cvtColor(srcImage,img_inputs[3],COLOR_BGR2RGB);
        //窗体4显示
        pixmapShows[3].setPixmap(QPixmap::fromImage(img));
        ui->imagebox4->Adapte();
    }
    srcImage.release();
}

void MainWindow::on_imageboxSaveImage()
{
    ImageWriter *iw=new ImageWriter;
    //保存方式：0-png;1-bmp;2-jpg;
    iw->method=2;
    iw->headname="_mannul_";

    QObject *ptrSender=sender();
    if(ptrSender==ui->imagebox1)
        iw->qimg=pixmapShows[0].pixmap().toImage();
    else if(ptrSender==ui->imagebox2)
        iw->qimg=pixmapShows[1].pixmap().toImage();
    else if(ptrSender==ui->imagebox3)
        iw->qimg=pixmapShows[2].pixmap().toImage();
    else if(ptrSender==ui->imagebox4)
        iw->qimg=pixmapShows[3].pixmap().toImage();

    QThreadPool::globalInstance()->start(iw);
}

void MainWindow::on_buttonProcess_clicked()
{
    if(imgProcessors[0]->img_filenames->size()==0 &&
            cams[0]->isCapturing==0 &&
            cams[1]->isCapturing==0 &&
            cams[2]->isCapturing==0 &&
            cams[3]->isCapturing==0)
    {
        isDetecting=true;
        for(int i=0;i<imgProcessors.size();i++)
        {
            imgProcessors[i]->isDetecting=isDetecting;
            if(imgProcessors[i]->ipcMutex.tryLock())
            {
                if(!img_inputs[i].empty())
                {
                    img_inputs[i].copyTo(imgProcessors[i]->img_input1);
                    imgProcessors[i]->ipcMutex.unlock();
                }
            }
        }
        emit procCam1Request();
        emit procCam2Request();
        emit procCam3Request();
        emit procCam4Request();
        isDetecting = false;
        return;
    }

    if(ui->buttonProcess->text()=="Process" && isDetecting==false)
    {
        isDetecting=true;
        ui->buttonProcess->setText("StopProcess");
        ui->condComboBox->setEnabled(false);
        for(int i=0;i<imgProcessors.size();i++)
        {
            cams[i]->isCapturing=2;
            imgProcessors[i]->isDetecting=isDetecting;
            imgProcessors[i]->ipcMutex.unlock();
        }
    }
    else if (ui->buttonProcess->text() == "StopProcess" && isDetecting == true)
    {
        isDetecting=false;
        ui->buttonProcess->setText("Process");
        ui->condComboBox->setEnabled(true);
        for(int i=0;i<imgProcessors.size();i++)
        {
            cams[i]->isCapturing=1;
            imgProcessors[i]->isDetecting=isDetecting;
            imgProcessors[i]->ipcMutex.unlock();
        }
    }
}

void MainWindow::on_buttonReset_clicked()
{
//    ui->buttonOpenImageList->setText("OpenImageList");
    printf("reset clicked!");
    ui->buttonOpenImgList->setText("OpenImgList");
    isDetecting = false;
    ui->buttonProcess->setText("Process");
    ui->buttonStartCapture->setText("StartCapture");
    for(int i=0;i<imgProcessors.size();i++)
    {
        cams.at(i)->isCapturing=0;
        imgProcessors[i]->resetPar();
    }
    ui->buttonOpenAIProject->setEnabled(true);
}

void MainWindow::on_buttonStartCapture_clicked()
{
    if(ui->buttonStartCapture->text()=="StartCapture")
    {
        for(int i=0;i<cams.size();i++)
            cams[i]->isCapturing=1;
        ui->buttonStartCapture->setText("StopCapture");

        if(!cams[0]->hasStarted)
            emit startCam1Request();
        else
            QThreadPool::globalInstance()->start(cams[0]);
        if(!cams[1]->hasStarted)
            emit startCam2Request();
        else
            QThreadPool::globalInstance()->start(cams[1]);
        if(!cams[2]->hasStarted)
            emit startCam3Request();
        else
            QThreadPool::globalInstance()->start(cams[2]);
        if(!cams[3]->hasStarted)
            emit startCam4Request();
        else
            QThreadPool::globalInstance()->start(cams[3]);

        for(int i=0;i<camProThds.size();i++)
            camProThds[i]->quit();
    }
    else if(ui->buttonStartCapture->text()=="StopCapture")
    {
        for(int i=0;i<cams.size();i++)
            cams[i]->isCapturing=0;
        ui->buttonStartCapture->setText("StartCapture");
    }
}

void MainWindow::slotimagebox1Refresh()
{
    QImage disImage;
//    窗体1显示
    if(imgProcessors[0]->img_output3.dims)
        disImage = QImage(imgProcessors[0]->img_output3.data,
                imgProcessors[0]->img_output3.cols,
                imgProcessors[0]->img_output3.rows,
                imgProcessors[0]->img_output3.cols*imgProcessors[0]->img_output3.channels(),
                QImage::Format_BGR888);
    else
        disImage = QImage(imgProcessors[0]->img_input1.data,
                imgProcessors[0]->img_input1.cols,
                imgProcessors[0]->img_input1.rows,
                imgProcessors[0]->img_input1.cols*imgProcessors[0]->img_input1.channels(),
                QImage::Format_RGB888);
    pixmapShows[0].setPixmap(QPixmap::fromImage(disImage));
    long gap2recv=timers[0].elapsed();
    timers[0].start();
    //界面状态显示
    QString tempStr;
    tempStr="1st camera refresh time(processed): "+QString::number(gap2recv)+
            "ms. In which process consumed: "+QString::number(imgProcessors[0]->onceRunTime)+
            "ms.\n";
    strOutputs[0]+=tempStr;
    double fps=1000/double(gap2recv);
    tempStr="1st camera FPS(processed): "+QString::number(fps,'f',1)+".\n";
    strOutputs[0]+=tempStr;
    if(imgProcessors[0]->img_output2.rows==1)
    {
        tempStr="class scores: ";
        for(int i=0;i<imgProcessors[0]->img_output2.cols;i++)
            tempStr+=(QString::number(imgProcessors[0]->img_output2.at<float>(0, i),'f',1)+",");
        strOutputs[0]+=tempStr;
        if(ui->checkBoxSaving->isChecked())
        {
            if(imgProcessors[0]->img_output2.at<float>(0, 0)>imgProcessors[0]->img_output2.at<float>(0, 1)
                    && iwts[0]->iwtMutex.tryLock())
            {
                iwts[0]->headname="_ch1_";
                iwts[0]->qimg=disImage;
                QThreadPool::globalInstance()->start(iwts[0]);
                iwts[0]->iwtMutex.unlock();
            }
        }
    }
    ui->textBrowser1->setText(strOutputs[0]);
    strOutputs[0].clear();
    tempStr.clear();
}

void MainWindow::slotimagebox2Refresh()
{
    QImage disImage;
//    窗体2显示
    if(imgProcessors[1]->img_output3.dims)
        disImage = QImage(imgProcessors[1]->img_output3.data,
                imgProcessors[1]->img_output3.cols,
                imgProcessors[1]->img_output3.rows,
                imgProcessors[1]->img_output3.cols*imgProcessors[1]->img_output3.channels(),
                QImage::Format_BGR888);
    else
        disImage = QImage(imgProcessors[1]->img_input1.data,
                imgProcessors[1]->img_input1.cols,
                imgProcessors[1]->img_input1.rows,
                imgProcessors[1]->img_input1.cols*imgProcessors[1]->img_input1.channels(),
                QImage::Format_RGB888);
    pixmapShows[1].setPixmap(QPixmap::fromImage(disImage));
    long gap2recv=timers[1].elapsed();
    timers[1].start();
    //界面状态显示
    QString tempStr;
    tempStr="2nd camera refresh time(processed): "+QString::number(gap2recv)+
            "ms. In which process consumed: "+QString::number(imgProcessors[1]->onceRunTime)+
            "ms.\n";
    strOutputs[1]+=tempStr;
    double fps=1000/double(gap2recv);
    tempStr="2nd camera FPS(processed): "+QString::number(fps,'f',1)+".\n";
    strOutputs[1]+=tempStr;
    if(imgProcessors[1]->img_output2.rows==1)
    {
        tempStr="class scores: ";
        for(int i=0;i<imgProcessors[1]->img_output2.cols;i++)
            tempStr+=(QString::number(imgProcessors[1]->img_output2.at<float>(0, i),'f',1)+",");
        strOutputs[1]+=tempStr;
        if(ui->checkBoxSaving->isChecked())
        {
            if(imgProcessors[1]->img_output2.at<float>(0, 0)>imgProcessors[1]->img_output2.at<float>(0, 1)
                    && iwts[1]->iwtMutex.tryLock())
            {
                iwts[1]->headname="_ch2_";
                iwts[1]->qimg=disImage;
                QThreadPool::globalInstance()->start(iwts[1]);
                iwts[1]->iwtMutex.unlock();
            }
        }
    }
    ui->textBrowser2->setText(strOutputs[1]);
    strOutputs[1].clear();
    tempStr.clear();
}

void MainWindow::slotimagebox3Refresh()
{
    QImage disImage;
//    窗体3显示
    if(imgProcessors[2]->img_output3.dims)
        disImage = QImage(imgProcessors[2]->img_output3.data,
                imgProcessors[2]->img_output3.cols,
                imgProcessors[2]->img_output3.rows,
                imgProcessors[2]->img_output3.cols*imgProcessors[2]->img_output3.channels(),
                QImage::Format_BGR888);
    else
        disImage = QImage(imgProcessors[2]->img_input1.data,
                imgProcessors[2]->img_input1.cols,
                imgProcessors[2]->img_input1.rows,
                imgProcessors[2]->img_input1.cols*imgProcessors[2]->img_input1.channels(),
                QImage::Format_RGB888);
    pixmapShows[2].setPixmap(QPixmap::fromImage(disImage));
    long gap2recv=timers[2].elapsed();
    timers[2].start();
    //界面状态显示
    QString tempStr;
    tempStr="3rd camera refresh time(processed): "+QString::number(gap2recv)+
            "ms. In which process consumed: "+QString::number(imgProcessors[2]->onceRunTime)+
            "ms.\n";
    strOutputs[2]+=tempStr;
    double fps=1000/double(gap2recv);
    tempStr="3rd camera FPS(processed): "+QString::number(fps,'f',1)+".\n";
    strOutputs[2]+=tempStr;
    if(imgProcessors[2]->img_output2.rows==1)
    {
        tempStr="class scores: ";
        for(int i=0;i<imgProcessors[2]->img_output2.cols;i++)
            tempStr+=(QString::number(imgProcessors[2]->img_output2.at<float>(0, i),'f',1)+",");
        strOutputs[2]+=tempStr;
        if(ui->checkBoxSaving->isChecked())
        {
            if(imgProcessors[2]->img_output2.at<float>(0, 0)>imgProcessors[2]->img_output2.at<float>(0, 1)
                    && iwts[2]->iwtMutex.tryLock())
            {
                iwts[2]->headname="_ch3_";
                iwts[2]->qimg=disImage;
                QThreadPool::globalInstance()->start(iwts[2]);
                iwts[2]->iwtMutex.unlock();
            }
        }
    }
    ui->textBrowser3->setText(strOutputs[2]);
    strOutputs[2].clear();
    tempStr.clear();
}

void MainWindow::slotimagebox4Refresh()
{
    QImage disImage;
//    窗体4显示
    if(imgProcessors[3]->img_output3.dims)
        disImage = QImage(imgProcessors[3]->img_output3.data,
                imgProcessors[3]->img_output3.cols,
                imgProcessors[3]->img_output3.rows,
                imgProcessors[3]->img_output3.cols*imgProcessors[3]->img_output3.channels(),
                QImage::Format_BGR888);
    else
        disImage = QImage(imgProcessors[3]->img_input1.data,
                imgProcessors[3]->img_input1.cols,
                imgProcessors[3]->img_input1.rows,
                imgProcessors[3]->img_input1.cols*imgProcessors[3]->img_input1.channels(),
                QImage::Format_RGB888);

    pixmapShows[3].setPixmap(QPixmap::fromImage(disImage));
    int gap2recv=timers[3].elapsed();
    timers[3].start();
    //界面状态显示
    QString tempStr;
    tempStr="4th camera refresh time(processed): "+QString::number(gap2recv)+
            "ms. In which process consumed: "+QString::number(imgProcessors[3]->onceRunTime)+
            "ms.\n";
    strOutputs[3]+=tempStr;
    double fps=1000/double(gap2recv);
    tempStr="4th camera FPS(processed): "+QString::number(fps,'f',1)+".\n";
    strOutputs[3]+=tempStr;
    if(imgProcessors[3]->img_output2.rows==1)
    {
        tempStr="class scores: ";
        for(int i=0;i<imgProcessors[3]->img_output2.cols;i++)
            tempStr+=(QString::number(imgProcessors[3]->img_output2.at<float>(0, i),'f',1)+",");
        strOutputs[3]+=tempStr;
        if(ui->checkBoxSaving->isChecked())
        {
            if(imgProcessors[3]->img_output2.at<float>(0, 0)>imgProcessors[3]->img_output2.at<float>(0, 1)
                    && iwts[3]->iwtMutex.tryLock())
            {
                iwts[3]->headname="_ch4_";
                iwts[3]->qimg=disImage;
                QThreadPool::globalInstance()->start(iwts[3]);
                iwts[3]->iwtMutex.unlock();
            }
        }
    }
    ui->textBrowser4->setText(strOutputs[3]);
    strOutputs[3].clear();
    tempStr.clear();
}

void MainWindow::slotGetOneFrame1(QImage img)
{
//    if(cams[0]->isCapturing==1 && !imgProcessors[0]->isDetecting)
    {
        //窗体1显示
        pixmapShows[0].setPixmap(QPixmap::fromImage(img));
//        ui->imagebox1->Adapte();
        int gap2recv=timers[0].elapsed();
        timers[0].start();
        QString tempStr;
        tempStr="\n 1st camera refresh time: "+QString::number(gap2recv)+" ms.\n";
        strOutputs[0]+=tempStr;
        double fps=1000/double(gap2recv);
        tempStr="1st camera FPS: "+QString::number(fps,'f',1)+".\n";
        strOutputs[0]+=tempStr;
        ui->textBrowser1->setText(strOutputs[0]);
        strOutputs[0].clear();
        tempStr.clear();
    }

//    if(cams[0]->isCapturing && imgProcessors[0]->isDetecting)
//    {
//        img_inputs[0]=Mat(img.height(), img.width(), CV_8UC3,
//                       img.bits(), img.bytesPerLine());
//        if(imgProcessors[0]->ipcMutex.tryLock())
//        {
//            img_inputs[0].copyTo(imgProcessors[0]->img_input1);
//            imgProcessors[0]->ipcMutex.unlock();
//            emit procCam1Request();
//        }
//    }
}

void MainWindow::slotGetOneFrame2(QImage img)
{
//    if(cams[1]->isCapturing==1 && !imgProcessors[1]->isDetecting)
    {
        //窗体1显示
        pixmapShows[1].setPixmap(QPixmap::fromImage(img));
//        ui->imagebox1->Adapte();
        int gap2recv=timers[1].elapsed();
        timers[1].start();
        QString tempStr;
        tempStr="\n 2nd camera refresh time: "+QString::number(gap2recv)+" ms.\n";
        strOutputs[1]+=tempStr;
        double fps=1000/double(gap2recv);
        tempStr="2nd camera FPS: "+QString::number(fps,'f',1)+".\n";
        strOutputs[1]+=tempStr;
        ui->textBrowser2->setText(strOutputs[1]);
        strOutputs[1].clear();
        tempStr.clear();
    }

//    if(cams[1]->isCapturing && imgProcessors[1]->isDetecting)
//    {
//        img_inputs[1]=Mat(img.height(), img.width(), CV_8UC3,
//                       img.bits(), img.bytesPerLine());
//        if(imgProcessors[1]->ipcMutex.tryLock())
//        {
//            img_inputs[1].copyTo(imgProcessors[1]->img_input1);
//            imgProcessors[1]->ipcMutex.unlock();
//            emit procCam2Request();
//        }
//    }
}

void MainWindow::slotGetOneFrame3(QImage img)
{
//    if(cams[2]->isCapturing==1 && !imgProcessors[2]->isDetecting)
    {
        //窗体1显示
        pixmapShows[2].setPixmap(QPixmap::fromImage(img));
//        ui->imagebox1->Adapte();
        int gap2recv=timers[2].elapsed();
        timers[2].start();
        QString tempStr;
        tempStr="\n 3rd camera refresh time: "+QString::number(gap2recv)+" ms.\n";
        strOutputs[2]+=tempStr;
        double fps=1000/double(gap2recv);
        tempStr="3rd camera FPS: "+QString::number(fps,'f',1)+".\n";
        strOutputs[2]+=tempStr;
        ui->textBrowser3->setText(strOutputs[2]);
        strOutputs[2].clear();
        tempStr.clear();
    }

//    if(cams[2]->isCapturing && imgProcessors[2]->isDetecting)
//    {
//        img_inputs[2]=Mat(img.height(), img.width(), CV_8UC3,
//                       img.bits(), img.bytesPerLine());
//        if(imgProcessors[2]->ipcMutex.tryLock())
//        {
//            img_inputs[2].copyTo(imgProcessors[2]->img_input1);
//            imgProcessors[2]->ipcMutex.unlock();
//            emit procCam3Request();
//        }
//    }
}

void MainWindow::slotGetOneFrame4(QImage img)
{
//    if(cams[3]->isCapturing==1 && !imgProcessors[3]->isDetecting)
    {
        //窗体1显示
        pixmapShows[3].setPixmap(QPixmap::fromImage(img));
//        ui->imagebox1->Adapte();
        int gap2recv=timers[3].elapsed();
        timers[3].start();
        QString tempStr;
        tempStr="\n 4th camera refresh time: "+QString::number(gap2recv)+" ms.\n";
        strOutputs[3]+=tempStr;
        double fps=1000/double(gap2recv);
        tempStr="4th camera FPS: "+QString::number(fps,'f',1)+".\n";
        strOutputs[3]+=tempStr;
        ui->textBrowser4->setText(strOutputs[3]);
        strOutputs[3].clear();
        tempStr.clear();
    }

//    if(cams[3]->isCapturing && imgProcessors[3]->isDetecting)
//    {
//        img_inputs[3]=Mat(img.height(), img.width(), CV_8UC3,
//                       img.bits(), img.bytesPerLine());
//        if(imgProcessors[3]->ipcMutex.tryLock())
//        {
//            img_inputs[3].copyTo(imgProcessors[3]->img_input1);
//            imgProcessors[3]->ipcMutex.unlock();
//            emit procCam4Request();
//        }
//    }
}

void MainWindow::on_buttonOpenImgList_clicked()
{
    if(ui->buttonOpenImgList->text()=="OpenImgList")
    {
        QString filename = QFileDialog::getExistingDirectory();
        QDir *dir=new QDir(filename);
        QStringList filter;
        filter<<"*.png"<<"*.jpg"<<"*.jpeg"<<"*.bmp";
        imgProcessors[0]->img_filenames =new QList<QFileInfo>(dir->entryInfoList(filter));
        imgProcessors[0]->processFilePic();
        if(imgProcessors[0]->img_filenames->count()>0)
            ui->buttonOpenImgList->setText("Next");
    }
    else if(ui->buttonOpenImgList->text()=="Next")
    {
        if (!imgProcessors[0]->processFilePic())
            ui->buttonOpenImgList->setText("OpenImgList");
    }
}

void MainWindow::on_condComboBox_activated(int index)
{
    ui->buttonReset->click();
    workCond = (WorkConditionsEnum)index;

    iniRW = new QSettings("LastSettings.ini",QSettings::IniFormat);
    iniRW->setValue("WorkCondition/WorkCondition",index);
    iniRW->setValue("TensorRTInference/ModelPath1",proj_paths[0]);
    iniRW->setValue("TensorRTInference/ModelPath2",proj_paths[1]);
    iniRW->setValue("TensorRTInference/ModelPath3",proj_paths[2]);
    iniRW->setValue("TensorRTInference/ModelPath4",proj_paths[3]);

//    switch (workCond){
//    default:{
//        for(int i=0;i<imgTrts.size();i++)
//        {
//            imgTrts[i]->prog_file=proj_paths[i];
//            imgTrts[i]->iniImgProcessor();
//            imgProcessors[i]=imgTrts[i];
//        }
////        connect(this,&MainWindow::startCam1Request,
////                imgTrt1,&trtInferencer::startProcessOnce);
//        connect(imgTrts[0],&trtInferencer::outputImgProcessedRequest,
//                this,&MainWindow::slotimagebox1Refresh);
//        connect(imgTrts[0],&trtInferencer::outputMulImgAIRequest,
//                this,&MainWindow::slotimagebox1Refresh);

//        //        connect(this,&MainWindow::startCam2Request,
//        //                imgTrt2,&trtInferencer::startProcessOnce);
//        connect(imgTrts[1],&trtInferencer::outputImgProcessedRequest,
//                this,&MainWindow::slotimagebox2Refresh);
//        connect(imgTrts[1],&trtInferencer::outputMulImgAIRequest,
//                this,&MainWindow::slotimagebox2Refresh);

//        //        connect(this,&MainWindow::startCam3Request,
//        //                imgTrt3,&trtInferencer::startProcessOnce);
//        connect(imgTrts[2],&trtInferencer::outputImgProcessedRequest,
//                this,&MainWindow::slotimagebox3Refresh);
//        connect(imgTrts[2],&trtInferencer::outputMulImgAIRequest,
//                this,&MainWindow::slotimagebox3Refresh);

//        //        connect(this,&MainWindow::startCam4Request,
//        //                imgTrt4,&trtInferencer::startProcessOnce);
//        connect(imgTrts[3],&trtInferencer::outputImgProcessedRequest,
//                this,&MainWindow::slotimagebox4Refresh);
//        connect(imgTrts[3],&trtInferencer::outputMulImgAIRequest,
//                this,&MainWindow::slotimagebox4Refresh);

//        for(int i=0;i<imgTrts.size();i++)
//            if(imgTrts[i]->hasInited)
//            {
//                ui->buttonOpenAIProject->setEnabled(false);
//                break;
//            }
//        break;}
//    }
}








