#include <iostream>


#include <fstream>

int main() {
  std::ifstream in("shaders/fragmentShaderRed.frag");
  std::string vertShaderString((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
  std::cout << vertShaderString.c_str() << std::endl;
  std::cout << vertShaderString.size() << std::endl;

}
