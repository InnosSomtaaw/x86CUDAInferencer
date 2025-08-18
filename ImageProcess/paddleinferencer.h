#ifndef PADDLEINFERENCER_H
#define PADDLEINFERENCER_H

#include "ImageProcess/image_processing.h"

#include <yaml-cpp/yaml.h>
#include <paddle_inference_api.h>

QT_BEGIN_NAMESPACE
using namespace paddle_infer;
QT_END_NAMESPACE

class paddleInferencer : public Image_Processing_Class
{
    Q_OBJECT
public:
    paddleInferencer();

    bool onGPU,iniTensorrt;
    QString prog_file,params_file;
    YAML::Node ymlNd;

    void iniImgProcessor() override;

protected:
    void run() override;

public slots:
    void startProcessOnce() override;

private:
    shared_ptr<Predictor> predictor;
    vector<vector<float>> input_data,output_data;
    vector<vector<int>> input_shape,output_shape;
    vector<string> input_names,output_names;
    vector<unique_ptr<Tensor>> input_tensor,output_tensor;
    size_t input_nums,output_nums;
    vector<paddle_infer::DataType> optDt;

    void preprocess();
    void postprocess();
};

#endif // PADDLEINFERENCER_H
