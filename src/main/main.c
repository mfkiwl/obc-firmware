// #include "../firmware.h"
#include "../mission/mission.h"

int main(void){
  Mission_init();

  while(1){
    Mission_loop();
  }

  return 0;
}
