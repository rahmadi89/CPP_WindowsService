# CPP_WindowsService
Service Skeleton. Just a sample service for windows. 

Write a single-file Windows service with cpp

After Compiling to an exe file named: MyService.exe (path: D:\MyService.exe)
run cmd as administrator and then run these commands:

```
sc create MyService binPath= "D:\MyService.exe"
```
CMD must return:
[SC] DeleteService SUCCESS


To Start Service:
```
sc start MyService
```

To Stop Service:
```
sc stop MyService
```

To Delete Service:
```
sc delete MyService
```



May Help: https://www.codeproject.com/Articles/14500/Detecting-Hardware-Insertion-and-or-Removal
