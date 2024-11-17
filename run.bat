for %%I in (client) do (
  ren CMakelists-%%I.txt CMakelists.txt 
  cd build/%%I
  del /S /Q *.*
  rd /S /Q .
  cmake ../.. -G "Unix Makefiles"
  make -j4
  cd ../../
  ren CMakelists.txt CMakelists-%%I.txt
  if %%I==server ("./build/%%I/GameNetworkServer.exe")
)