##!/bin/bash
xhost +

docker run -it --privileged --name cuda_tank --network=host --shm-size=16g --gpus all  \
-e NVIDIA_DRIVER_CAPABILITIES=compute,utility,video \
-v /tmp/.X11-unix:/tmp/.X11-unix -v $HOME/.Xauthority:/root/.Xauthority -v /mnt:/mnt -v ~/.ssh/:/root/.ssh/ \
-e DISPLAY=$DISPLAY -e GDK_SCALE -e GDK_DPI_SCALE -e LC_ALL=C.utf8 \
dev_env_x86inference:CUDA-all /bin/bash
