#include "Operations.h"
#include "ActiveObject.h"
#include "CameraAndLightControl.h"
#include "TcpServer/TcpServer.h"

#include <boost/asio.hpp>

#include <iostream>
#include <chrono>

class RemoteControl : public IObserver< CameraAndLightControl >
{
public:
  RemoteControl(ActiveObject< CameraAndLightControl > &iCameraAndLightControl)
    : mCameraAndLightControl(iCameraAndLightControl)
  {
  }

  ~RemoteControl()
  {
    mCameraAndLightControl.push([&](CameraAndLightControl &iControl)
    {
      iControl.detach(this);
    });
  }
  
  void setSender(const std::shared_ptr< ISender > &iSender)
  {
    mSender = iSender;
    mCameraAndLightControl.push([&](CameraAndLightControl &iControl)
    {
      iControl.attach(this);
    });
  }

  void operator()(const std::string &iPayload)
  {
  }

  void update(const CameraAndLightControl &iControl)
  {
  }

private:
  ActiveObject< CameraAndLightControl > &mCameraAndLightControl;
  std::weak_ptr< ISender > mSender;
};

int main(int argc, char *argv[])
{
  boost::asio::io_service wIoService;
  ActiveObject< CameraAndLightControl > wCameraAndLightControl(CameraAndLightControl(argv[1], argv[2]));
  
  auto wTcpServer = createTcpServer(wIoService, RemoteControl(wCameraAndLightControl), 80);

  std::thread wTcpServerThread([&]() { wIoService.run(); });

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
  
  wIoService.stop();
  wTcpServerThread.join();

  return 0;
}
