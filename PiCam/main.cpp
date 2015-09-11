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
  if (argc != 7)
  {
    std::cerr << "Usage: PiCam raspivid_pid path_to_srt path_to_db path_to_output_gpio path_to_input_gpio websocket_port" << std::endl;
    exit(1);
  }

  DataActiveObject< CameraAndLightControl > wCameraAndLightControl(CameraAndLightControl(argv[1], argv[2], argv[3], argv[4]));

  std::thread wDataActiveObjectThread([&]() { wCameraAndLightControl.run(); });

  startServerAndMonitorPins(wCameraAndLightControl, argv[5], static_cast< unsigned short >(std::strtoul(argv[6], nullptr, 10)));

  wCameraAndLightControl.stop();
  wDataActiveObjectThread.join();

  return 0;
}

class VideoDispatcher : public Subject< VideoDispatcher >
{
public:
  VideoDispatcher()
  {
  }

  void threadSafeAttach(IObserver< VideoDispatcher > *iObserver)
  {
    std::lock_guard< std::mutex > wLock(mDispatchMutex);
    attach(iObserver);
  }

  void threadSafeDetach(IObserver< VideoDispatcher > *iObserver)
  {
    std::lock_guard< std::mutex > wLock(mDispatchMutex);
    detach(iObserver);
  }

  void perform(const char *iData, size_t iSize)
  {
    std::lock_guard< std::mutex > wLock(mDispatchMutex);
    mCurrentChunk.assign(iData, iSize);
    notify(*this);
  }

  const std::string &currentChunk() const
  {
    return mCurrentChunk;
  }

private:
  std::string mCurrentChunk;
  std::mutex mDispatchMutex;
};

class VideoConnector : public IObserver< VideoDispatcher >
{
public:
  VideoConnector(VideoDispatcher &iVideoDispatcher)
    : mVideoDispatcher(iVideoDispatcher)
    , mPlaying(false)
  {
    mVideoDispatcher.threadSafeAttach(this);
  }

  VideoConnector(const VideoConnector &iVideoConnector)
    : mVideoDispatcher(iVideoConnector.mVideoDispatcher)
    , mPlaying(iVideoConnector.mPlaying.load())
  {
    mVideoDispatcher.threadSafeAttach(this);
  }

  ~VideoConnector()
  {
    mVideoDispatcher.threadSafeDetach(this);
  }
  
  void setSender(const std::shared_ptr< ISender > &iSender)
  {
    mSender = iSender;
  }

  void operator()(const std::string &iPayload)
  {
    std::cout << "message: " << iPayload << std::endl;
    if (iPayload == "play")
    {
      mPlaying = true;
    }
    else if (iPayload == "pause")
    {
      mPlaying = false;
    }
  }

  void update(const VideoDispatcher &iVideoDispatcher)
  {
    if (mPlaying)
    {
      send(iVideoDispatcher.currentChunk());
    }
  }

private:
  std::atomic< bool > mPlaying;
  std::weak_ptr< ISender > mSender;
  VideoDispatcher &mVideoDispatcher;

  void send(const std::string &iMessage)
  {
    auto wSender = mSender.lock();
    if (wSender != nullptr)
    {
      wSender->send(iMessage, ISender::MessageType::BINARY);
    }
  }
};

void startServerAndMonitorPins(DataActiveObject< CameraAndLightControl > &iCameraAndLightControl, const char *iInputGpio, unsigned short iPort)
{
  boost::asio::io_service wIoService;

  auto wControlAndMonitoringServer = createTcpServer(wIoService, RemoteControl(iCameraAndLightControl), iPort);

  VideoDispatcher wVideoDispatcher;
  auto wLiveVideoServer = createTcpServer(wIoService, VideoConnector(wVideoDispatcher), iPort + 1);
  
  std::thread wIoServiceThread([&]() { wIoService.run(); });
  std::thread wStdinDispatch([&]()
  {
    std::array< char, 32768 > wChunk;
    while (true)
    {
      auto wSize = static_cast< size_t >(std::cin.readsome(&wChunk[0], wChunk.size()));
      wVideoDispatcher.perform(&wChunk[0], wSize);
    }
  });

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
  wLiveVideoServer.stop();
  wIoServiceThread.join();
}
