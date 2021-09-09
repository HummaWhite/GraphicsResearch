#include "FSR.h"

int main()
{
    FSR engine;
    Inputs::bindEngine(&engine);
    Inputs::setup();
    return engine.run();
}