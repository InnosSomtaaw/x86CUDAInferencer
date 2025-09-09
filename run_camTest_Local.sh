#!/bin/bash
docker stop cuda_tank

xhost +

docker start cuda_tank

docker cp /mnt/data/LastSettings.ini cuda_tank:/LastSettings.ini

docker exec -e LD_LIBRARY_PATH=/usr/local/cuda/lib64:/home/paddle_inference/paddle/lib:/home/paddle_inference/third_party/install/paddle2onnx/lib:/home/paddle_inference/third_party/install/onnxruntime/lib:/home/paddle_inference/third_party/install/mklml/lib:/home/paddle_inference/third_party/install/protobuf/lib:/home/paddle_inference/third_party/install/mkldnn/lib:/home/TensorRT-8.6.1.6/lib \
-e DISPLAY=$DISPLAY -e XAUTHORITY=/root/.Xauthority \
cuda_tank /mnt/data/camsObjDet
