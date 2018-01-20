#include <iostream>
#include <fstream>
#include <cstdio>
#include <thread>
#include "videoCamera.h"

void record(videoCamera *camera){
  
  std::ofstream lfile("rdbuf.txt");
  std::streambuf *x = std::cout.rdbuf(lfile.rdbuf());  
  std::streambuf *r = std::cerr.rdbuf(lfile.rdbuf());
  camera->encode();
  std::cout.rdbuf(x);
  std::cerr.rdbuf(r);
}

int main(int argc, char **argv) {
  
  videoCamera camera;  
  
  char ter;
  std::thread t(record, &camera);
  t.detach();
  
  std::cin>>ter;
  camera.terminal();
//   camera.encode();
  
  return 0;
}
