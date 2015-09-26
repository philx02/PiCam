#include "Operations.h"
#include "ActiveObject.h"
#include "CameraAndLightControl.h"
#include "TcpServer/TcpServer.h"
#include "RemoteControl.h"

#include <boost/asio.hpp>

#include <iostream>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>

void startServerAndMonitorPins(DataActiveObject< CameraAndLightControl > &iCameraAndLightControl, const char *iInputGpio, unsigned short iPort);

int main(int argc, char *argv[])
{
  if (argc != 5)
  {
    std::cerr << "Usage: PiCam path_to_db path_to_output_gpio path_to_input_gpio websocket_port" << std::endl;
    exit(1);
  }

  DataActiveObject< CameraAndLightControl > wCameraAndLightControl(CameraAndLightControl(argv[1], argv[2]));

  std::thread wDataActiveObjectThread([&]() { wCameraAndLightControl.run(); });

  startServerAndMonitorPins(wCameraAndLightControl, argv[3], static_cast< unsigned short >(std::strtoul(argv[4], nullptr, 10)));

  wCameraAndLightControl.stop();
  wDataActiveObjectThread.join();

  return 0;
}

void startServerAndMonitorPins(DataActiveObject< CameraAndLightControl > &iCameraAndLightControl, const char *iInputGpio, unsigned short iPort)
{
  boost::asio::io_service wIoService;

  auto wControlAndMonitoringServer = createTcpServer(wIoService, RemoteControl(iCameraAndLightControl), iPort);
    
  std::thread wIoServiceThread([&]() { wIoService.run(); });

  {
    std::ifstream wGpio(iInputGpio);
  
    int wGpioDebounceCounter = 0;
    bool wGpioDebouncedValue = false;
    while (true)
    {
      bool wGpioValue = false;
      wGpio.seekg(0);
      wGpio >> wGpioValue;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      wGpioDebounceCounter += wGpioValue ? 1 : -1;
      if (wGpioDebounceCounter > 10)
      {
        wGpioDebounceCounter = 10;
        if (!wGpioDebouncedValue)
        {
          wGpioDebouncedValue = true;
          iCameraAndLightControl.dataPush([wGpioDebouncedValue](CameraAndLightControl &iControl)
          {
            iControl.doorSwitch(wGpioDebouncedValue);
          });
        }
      }
      else if (wGpioDebounceCounter < 0)
      {
        wGpioDebounceCounter = 0;
        if (wGpioDebouncedValue)
        {
          wGpioDebouncedValue = false;
          iCameraAndLightControl.dataPush([wGpioDebouncedValue](CameraAndLightControl &iControl)
          {
            iControl.doorSwitch(wGpioDebouncedValue);
          });
        }
      }
    }
  }

  wControlAndMonitoringServer.stop();
  wIoServiceThread.join();
}
