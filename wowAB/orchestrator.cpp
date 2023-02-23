#include <iostream>
#include "WowSocket.H"

#include "WowConcert.H"

int main() {
  std::cout << "hello" << std::endl;
  
  WowOrchestra perf( 61617 );
 
  perf.perform(2);
}
