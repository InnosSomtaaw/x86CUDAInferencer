#include "paddleinferencer.h"

paddleInferencer::paddleInferencer()
{
    hasInited=false;
    onGPU=true;
    iniTensorrt=false;
}

void paddleInferencer::iniImgProcessor()
{
    // 字符串 prog_file 为 Combine 模型文件所在路径
    // 字符串 params_file 为 Combine 模型参数文件所在路径
    QFileInfo modelFi(prog_file);
    if(!modelFi.isFile())
        return;
    QString modelFp=modelFi.filePath();

    params_file=modelFi.path()+"/"+modelFi.baseName()+".pdiparams";
    modelFi=QFileInfo(params_file);
    if(!modelFi.isFile())
        return;

    // 根据模型文件和参数文件构造 Config 对象
    Config config(prog_file.toStdString(), params_file.toStdString());

    config.EnableMKLDNN();
    // Open the memory optim.
    config.EnableMemoryOptim();
    if(onGPU)
        config.EnableUseGpu(1024,0);
    if(iniTensorrt && onGPU)
    {
        // 启用 TensorRT 进行预测加速 - FP16
        config.EnableTensorRtEngine(1 << 30, 1, 5,
                                    PrecisionType::kFloat32, false, false);
        // 通过 API 获取 TensorRT 启用结果 - true
        cout << "Enable TensorRT is: " << config.tensorrt_engine_enabled() << endl;
        // 开启 TensorRT 显存优化
        config.EnableTensorRTMemoryOptim();

//        // 设置模型输入的动态 Shape 范围
//        map<string, vector<int>> min_input_shape = {{"image", {1, 512, 3, 3}}};
//        map<string, vector<int>> max_input_shape = {{"image", {1, 512, 10, 10}}};
//        map<string, vector<int>> opt_input_shape = {{"image", {1, 512, 3, 3}}};
//        // 设置 TensorRT 的动态 Shape
//        config.SetTRTDynamicShapeInfo(min_input_shape, max_input_shape, opt_input_shape);
    }
    cout<<config.Summary()<<endl;

    // 根据 Config 对象创建预测器对象
    predictor = CreatePredictor(config);

    input_names = predictor->GetInputNames();
    output_names = predictor->GetOutputNames();//获取输出名字
    input_nums=input_names.size();
    output_nums=output_names.size();
    input_shape.resize(input_nums);
    output_shape.resize(output_nums);
    input_tensor.resize(input_nums);
    output_tensor.resize(output_nums);
    QString deploySettings =modelFi.path()+"/deploy.yaml";
    modelFi=QFileInfo(deploySettings);
    if(modelFi.isFile())
    {
        ymlNd=YAML::LoadFile(deploySettings.toStdString());
        for(size_t i=0;i<input_nums;i++)
            input_shape[i]=ymlNd["Deploy"]["input_shape"].as<vector<int>>();
    }
    else
        for(size_t i=0;i<input_nums;i++)
            input_shape[i]={1, 3, 224, 224};

    input_data.resize(input_nums);
    for(size_t i=0;i<input_data.size();i++)
        input_data[i].resize(accumulate(input_shape[i].begin(), input_shape[i].end(),
                                        1, multiplies<int>()));
    // 准备输入Tensor
    for(size_t i=0;i<input_nums;i++)
    {
        input_tensor[i]=predictor->GetInputHandle(input_names[i]);
        input_tensor[i]->Reshape(input_shape[i]);
        input_tensor[i]->CopyFromCpu(input_data[i].data());  //复制一份图片,拷贝数据输入
    }
    //运行
    predictor->Run();
    //准备输出Tensor
    for(size_t i=0;i<output_nums;i++)
    {
        output_tensor[i] = predictor->GetOutputHandle(output_names[i]);//获取输出句柄
        optDt.push_back(output_tensor[i]->type());
        output_shape[i]=output_tensor[i]->shape();
    }

    hasInited=true;
}

/*  图片预处理
img:   图片
img_size:图片大小
mean: 输入均值
std:  输入标准差
return chw储存方式的vector
*/
void paddleInferencer::preprocess()
{
//    // 准备输入 Tensor
//    input_names = predictor->GetInputNames();
//    input_tensor = predictor->GetInputHandle(input_names[0]);
//    input_tensor->Reshape(input_shape);

    if(input_nums==1)
    {
        dnn::blobFromImage(img_input1,img_output1,1.0f/255.0f,
                           Size(input_shape[0][3],input_shape[0][2]),
                Scalar(127.5, 127.5, 127.5));
        input_tensor[0]->CopyFromCpu((float*)img_output1.data);
    }
}

void paddleInferencer::postprocess()
{
//    //输出
//    output_names = predictor->GetOutputNames();//获取输出名字
//    output_tensor = predictor->GetOutputHandle(output_names[0]);//获取输出句柄

    for(size_t i=0;i<output_nums;i++)
        output_tensor[i] = predictor->GetOutputHandle(output_names[i]);//获取输出句柄

    if(output_nums>1)
        return;

    switch (optDt[0]) {
    case INT32:{
        img_output2=Mat(output_shape[0][1],output_shape[0][2],CV_32SC(output_shape[0][0]));
        output_tensor[0]->CopyToCpu((int*)img_output2.data);//将输出拷贝到out_data
        img_output2.convertTo(img_output2,CV_8UC3,10);
//        img_output2*=10;
        applyColorMap(img_output2,img_output3,COLORMAP_VIRIDIS);
        break;}
    default:{
        img_output3=Mat(output_shape[0][1],output_shape[0][2],CV_32FC(output_shape[0][0]));
        output_tensor[0]->CopyToCpu((float*)img_output3.data);//将输出拷贝到out_data
        break;}
    }
}

void paddleInferencer::startProcessOnce()
{
    this->setAutoDelete(false);
    QThreadPool::globalInstance()->start(this);
}

void paddleInferencer::run()
{
    usrtimer.start();
    if (!ipcMutex.tryLock())
        return;
    preprocess();
//    cout<< "pre-process time is " <<usrtimer.elapsed()<< " ms"<<endl;
    //运行
    predictor->Run();
    ipcMutex.unlock();
//    cout<< "predict time is " <<usrtimer.elapsed()<< " ms"<<endl;
    postprocess();

    onceRunTime=usrtimer.elapsed();
//    cout<< "post-process time is " <<onceRunTime<< " ms"<<endl;
    emit outputImgProcessedRequest();
}
