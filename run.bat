cd build-http/
del /S /Q *.*
rd /S /Q .
cmake .. -G "Unix Makefiles"
make -j4
GameNetworkClient.exe