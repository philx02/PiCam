#include "Operations.h"
#include "ActiveObject.h"
#include "CameraAndLightControl.h"
#include "TcpServer/TcpServer.h"
#include "RemoteControl.h"

#include <boost/asio.hpp>

#include <iostream>
#include <chrono>

void startServerAndMonitorPins(ActiveObject< CameraAndLightControl > &iCameraAndLightControl, const char *iInputGpio, unsigned short iPort);

int main(int argc, char *argv[])
{
  if (argc != 7)
  {
    std::cerr << "Usage: PiCam raspivid_pid path_to_srt path_to_db path_to_output_gpio path_to_input_gpio websocket_port" << std::endl;
    exit(1);
  }

  ActiveObject< CameraAndLightControl > wCameraAndLightControl(CameraAndLightControl(argv[1], argv[2], argv[3], argv[4]));

  std::thread wActiveObjectThread([&]() { wCameraAndLightControl.run(); });

  startServerAndMonitorPins(wCameraAndLightControl, argv[5], static_cast< unsigned short >(std::strtoul(argv[6], nullptr, 10)));

  wCameraAndLightControl.stop();
  wActiveObjectThread.join();

  return 0;
}

void startServerAndMonitorPins(ActiveObject< CameraAndLightControl > &iCameraAndLightControl, const char *iInputGpio, unsigned short iPort)
{
  boost::asio::io_service wIoService;
  auto wTcpServer = createTcpServer(wIoService, RemoteControl(iCameraAndLightControl), iPort);

  std::thread wTcpServerThread([&]() { wIoService.run(); });

  auto wGpioFd = openGpio(iInputGpio);
  auto wPollFd = openSwitchListener(wGpioFd);

  while (true)
  {
    std::cout << "Waiting for input" << std::endl;
    auto wGpioValue = readGpio(wPollFd, wGpioFd);
    std::cout << "Gpio value: " << wGpioValue << std::endl;
    iCameraAndLightControl.push([wGpioValue](CameraAndLightControl &iControl)
    {
      iControl.doorSwitch(wGpioValue);
    });
  }

  wTcpServer.stop();
  wTcpServerThread.join();
}
