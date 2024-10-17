

## ISP
ISP3.2

## VI
rv1106/rv1103:
1*dev -> 1*pipe -> 6*channel

dev-pipe
```c
RK_MPI_VI_GetDevAttr()
RK_MPI_VI_SetDevAttr()

RK_MPI_VI_EnableDev()

RK_MPI_VI_SetDevBindPipe()

RK_MPI_VI_QueryDevStatus
```

channel
```c
RK_MPI_VI_SetChnAttr() 通道参数
RK_MPI_VI_SetChnWrapBufAttr() 卷绕模式


```

卷绕模式：
仅支持mainpath通道

## OSD

## VENC