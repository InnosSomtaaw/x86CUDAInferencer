#include "yolopostprocess.h"

yoloPostProcess::yoloPostProcess()
{

}

void yoloPostProcess::postprocess(Mat& frame, Mat& outs)
{
    std::vector<int> classIds;
    std::vector<float> confidences;
    std::vector<Rect> boxes;

    // Network produces output blob with a shape NxC where N is a number of
    // detected objects and C is a number of classes + 4 where the first 4
    // numbers are [center_x, center_y, width, height]
    float* data = (float*)outs.data;
    for (int j = 0; j < outs.rows; ++j, data += outs.cols)
    {
        Mat scores = outs.row(j).colRange(4, outs.cols);
        Point classIdPoint;
        double confidence;
        minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
        if (confidence > SCORE_THRESHOLD)
        {
            int centerX = (int)(data[0]);
            int centerY = (int)(data[1]);
            int width = (int)(data[2]);
            int height = (int)(data[3]);
            int left = centerX - width / 2;
            int top = centerY - height / 2;

            classIds.push_back(classIdPoint.x);
            confidences.push_back((float)confidence);
            boxes.push_back(Rect(left, top, width, height));
        }
    }

    // NMS
    std::map<int, std::vector<size_t> > class2indices;
    for (size_t i = 0; i < classIds.size(); i++)
        if (confidences[i] >= SCORE_THRESHOLD)
            class2indices[classIds[i]].push_back(i);
    std::vector<Rect> nmsBoxes;
    std::vector<float> nmsConfidences;
    std::vector<int> nmsClassIds;
    for (std::map<int, std::vector<size_t> >::iterator it = class2indices.begin();
         it != class2indices.end(); ++it)
    {
        std::vector<Rect> localBoxes;
        std::vector<float> localConfidences;
        std::vector<size_t> classIndices = it->second;
        for (size_t i = 0; i < classIndices.size(); i++)
        {
            localBoxes.push_back(boxes[classIndices[i]]);
            localConfidences.push_back(confidences[classIndices[i]]);
        }
        std::vector<int> nmsIndices;
        dnn::NMSBoxes(localBoxes, localConfidences, SCORE_THRESHOLD, NMS_THRESHOLD, nmsIndices);
        for (size_t i = 0; i < nmsIndices.size(); i++)
        {
            size_t idx = nmsIndices[i];
            nmsBoxes.push_back(localBoxes[idx]);
            nmsConfidences.push_back(localConfidences[idx]);
            nmsClassIds.push_back(it->first);
        }
    }
    boxes = nmsBoxes;
    classIds = nmsClassIds;
    confidences = nmsConfidences;

    for (size_t idx = 0; idx < boxes.size(); ++idx)
    {
        Rect box = boxes[idx];
        drawPred(classIds[idx], confidences[idx], box.x, box.y,
                 box.x + box.width, box.y + box.height, frame);
    }
}

void yoloPostProcess::postprocess(Mat &out, Size rawSz)
{
    ids.clear();
    confds.clear();
    posRects.clear();
    float x_factor = float(rawSz.width)/float(INPUT_WIDTH);
    float y_factor = float(rawSz.height)/float(INPUT_HEIGHT);
    // Network produces output blob with a shape NxC where N is a number of
    // detected objects and C is a number of classes + 4 where the first 4
    // numbers are [center_x, center_y, width, height]
    float* data = (float*)out.data;
    for (int i = 0; i < out.rows; ++i, data += out.cols)
    {
        Mat scores = out.row(i).colRange(4, out.cols);
        Point classIdPoint;
        double confidence;
        minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
        if (confidence > SCORE_THRESHOLD)
        {
            int left = int((data[0] - 0.5 * data[2]) * x_factor);
            int top = int((data[1] - 0.5 * data[3]) * y_factor);
            int width = int(data[2] * x_factor);
            int height = int(data[3] * y_factor);
            ids.push_back(classIdPoint.x);
            confds.push_back((float)confidence);
            posRects.push_back(Rect(left, top, width, height));
        }
    }
    // NMS
    std::map<int, std::vector<size_t> > class2indices;
    for (size_t i = 0; i < ids.size(); i++)
        if (confds[i] >= SCORE_THRESHOLD)
            class2indices[ids[i]].push_back(i);
    std::vector<Rect> nmsBoxes;
    std::vector<float> nmsConfidences;
    std::vector<int> nmsClassIds;
    for (std::map<int, std::vector<size_t> >::iterator it = class2indices.begin();
         it != class2indices.end(); ++it)
    {
        std::vector<Rect> localBoxes;
        std::vector<float> localConfidences;
        std::vector<size_t> classIndices = it->second;
        for (size_t i = 0; i < classIndices.size(); i++)
        {
            localBoxes.push_back(posRects[classIndices[i]]);
            localConfidences.push_back(confds[classIndices[i]]);
        }
        std::vector<int> nmsIndices;
        dnn::NMSBoxes(localBoxes, localConfidences, SCORE_THRESHOLD, NMS_THRESHOLD, nmsIndices);
        for (size_t i = 0; i < nmsIndices.size(); i++)
        {
            size_t idx = nmsIndices[i];
            nmsBoxes.push_back(localBoxes[idx]);
            nmsConfidences.push_back(localConfidences[idx]);
            nmsClassIds.push_back(it->first);
        }
    }
    posRects = nmsBoxes;
    ids = nmsClassIds;
    confds = nmsConfidences;
}

void yoloPostProcess::drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame)
{
    rectangle(frame, Point(left, top), Point(right, bottom), Scalar(0, 255, 0));

    std::string label = format("%.2f", conf);
    if (!classes.empty())
    {
        CV_Assert(classId < (int)classes.size());
        label = classes[classId] + ": " + label;
    }

    int baseLine;
    Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    top = max(top, labelSize.height);
    rectangle(frame, Point(left, top - labelSize.height),
              Point(left + labelSize.width, top + baseLine), Scalar::all(255), FILLED);
    putText(frame, label, Point(left, top), FONT_HERSHEY_SIMPLEX, 0.5, Scalar());
}


