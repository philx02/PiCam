#include "Operations.h"
#include "ActiveObject.h"
#include "CameraAndLightControl.h"
#include "TcpServer/TcpServer.h"
#include "RemoteControl.h"

#include <boost/asio.hpp>

#include <iostream>
#include <chrono>


void startServerAndMonitorPins(ActiveObject< CameraAndLightControl > &iCameraAndLightControl, const char *iInputGpio);

int main(int argc, char *argv[])
{
  if (argc != 4)
  {
    std::cerr << "Usage: PiCam path_to_video path_to_srt path_to_input_gpio";
    exit(1);
  }

  ActiveObject< CameraAndLightControl > wCameraAndLightControl(CameraAndLightControl(argv[1], argv[2]));

  std::thread wActiveObjectThread([&]() { wCameraAndLightControl.run(); });

  startServerAndMonitorPins(wCameraAndLightControl, argv[3]);

  wCameraAndLightControl.stop();
  wActiveObjectThread.join();

  return 0;
}

void startServerAndMonitorPins(ActiveObject< CameraAndLightControl > &iCameraAndLightControl, const char *iInputGpio)
{
  boost::asio::io_service wIoService;
  auto wTcpServer = createTcpServer(wIoService, RemoteControl(iCameraAndLightControl), 80);

  std::thread wTcpServerThread([&]() { wIoService.run(); });

  auto wGpioFd = openGpio(iInputGpio);
  auto wPollFd = openSwitchListener(wGpioFd);

  while (true)
  {
    std::cout << "Waiting for input" << std::endl;
    auto wGpioValue = readGpio(wPollFd, wGpioFd);
    std::cout << "Gpio value: " << wGpioValue << std::endl;
    if (wGpioValue)
    {
      iCameraAndLightControl.push([](CameraAndLightControl &iControl)
      {
        iControl.startRecording();
      });
    }
    else
    {
      iCameraAndLightControl.push([](CameraAndLightControl &iControl)
      {
        iControl.stopRecording();
      });
    }
  }

  wTcpServer.stop();
  wTcpServerThread.join();
}
