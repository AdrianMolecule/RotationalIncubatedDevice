# 

for connections schematics see MKS_DLC32 _V2.1_002_SCH.pdf in the root dir.

This is the basic code for a generic rotational device like the orbital shaker or tube rotator
it supports temperature and microstepping where available and a extremly powerful web interface that allows you to even change the pins for the sensors etc so you can accommodate any types of boards.Works well and I'm using both cores so the UI still responds enven if the backend is blocking (which is not at this point anyway).

You need to go to pass.h and relace the existing ssid and password tith you local network ssid and password

git log -S pass
git log --all -- src/Controller.h
git clone https://github.com/AdrianMolecule/RotationalIncubatedDevice


remote add origin https://github.com/AdrianMolecule/RotationalIncubatedDevice
git push origin --force --mirror             will overwrite hist on remote
