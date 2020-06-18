CXX=g++

release: dcsmp.o dc_mqtt.o userdata.o xyz.o
	$(CXX) -o dcsmp dcsmp.o dc_mqtt.o userdata.o xyz.o -ljsoncpp -lmosquitto -std=c++11

self-test: dcsmp-test.o dc_mqtt.o userdata-debug.o xyz.o
	$(CXX) -o dcsmp-test dcsmp-test.o dc_mqtt.o userdata.o xyz.o -ljsoncpp -lmosquitto -std=c++11

debug: dcsmp-debug.o dc_mqtt.o userdata-debug.o xyz.o
	$(CXX) -o dcsmp-debug dcsmp-debug.o dc_mqtt.o userdata.o xyz.o -ljsoncpp -lmosquitto -std=c++11

dcsmp.o: src/dcsmp.cpp
	$(CXX) -c -o dcsmp.o src/dcsmp.cpp -std=c++11

dcsmp-debug.o: src/dcsmp.cpp
	$(CXX) -c -o dcsmp-debug.o src/dcsmp.cpp -DDEBUG -std=c++11

dcsmp-test.o: src/dcsmp.cpp
	$(CXX) -c -o dcsmp-test.o src/dcsmp.cpp -DDEBUG -DTEST -std=c++11

userdata.o: src/userdata.cpp
	$(CXX) -c -o userdata.o src/userdata.cpp -std=c++11

userdata-debug.o: src/userdata.cpp
	$(CXX) -c -o userdata.o src/userdata.cpp -DDEBUG -std=c++11

xyz.o: src/xyz.cpp
	$(CXX) -c -o xyz.o src/xyz.cpp -std=c++11
	
dc_mqtt.o: src/dc_mqtt/dc_mqtt.cpp
	$(CXX) -c -o dc_mqtt.o src/dc_mqtt/dc_mqtt.cpp -std=c++11

clean:
	rm -f dcsmp* *.dmp *.o
