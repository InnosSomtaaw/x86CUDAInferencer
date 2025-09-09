##when linux2windows:
#rm -f /usr/lib/x86_64-linux-gnu/libnvidia-ml.so.1
#rm -f /usr/lib/x86_64-linux-gnu/libcuda.so.1
#rm -f /usr/lib/x86_64-linux-gnu/libcudadebugger.so.1
#rm -f /usr/lib/x86_64-linux-gnu/libnvcuvid.so.1

##when windows2linux:
docker cp /usr/lib/x86_64-linux-gnu/libnvidia-ml.so.1 cuda_tank:/usr/lib/x86_64-linux-gnu/libnvidia-ml.so.1
docker cp /usr/lib/x86_64-linux-gnu/libcuda.so.1 cuda_tank:/usr/lib/x86_64-linux-gnu/libcuda.so.1
docker cp /usr/lib/x86_64-linux-gnu/libcudadebugger.so.1 cuda_tank:/usr/lib/x86_64-linux-gnu/libcudadebugger.so.1
docker cp /usr/lib/x86_64-linux-gnu/libnvcuvid.so.1 cuda_tank:/usr/lib/x86_64-linux-gnu/libnvcuvid.so.1

