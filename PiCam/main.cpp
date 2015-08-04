#include "Operations.h"
#include "ActiveObject.h"
#include "CameraAndLightControl.h"
#include "TcpServer/TcpServer.h"

#include <boost/asio.hpp>

#include <iostream>
#include <chrono>

int main(int argc, char *argv[])
{
  boost::asio::io_service wIoService;
  ActiveObject< CameraAndLightControl > wCameraAndLightControl(CameraAndLightControl(argv[1], argv[2]));
  std::thread wActiveObjectThread([&]() { wCameraAndLightControl.run(); });
  auto wGpioFd = openGpio(argv[3]);
  auto wPollFd = openSwitchListener(wGpioFd);
  
  waitForCamera();
  
  while (true)
  {
    std::cout << "Waiting for input" << std::endl;
    auto wGpioValue = readGpio(wPollFd, wGpioFd);
    std::cout << "Gpio value: " << wGpioValue << std::endl;
    if (wGpioValue)
    {
      wCameraAndLightControl.push([](CameraAndLightControl &iControl)
      {
        iControl.startRecording();
      });
    }
    else
    {
      wCameraAndLightControl.push([](CameraAndLightControl &iControl)
      {
        iControl.stopRecording();
      });
    }
  }

  wCameraAndLightControl.stop();
  wActiveObjectThread.join();

  return 0;
}
