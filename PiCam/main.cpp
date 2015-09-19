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

class VideoDispatcher : public Subject< VideoDispatcher >, public boost::noncopyable
{
public:
  VideoDispatcher()
    : mCurrentChunkFinalSizeBufferIter(mCurrentChunkFinalSizeBuffer.rbegin())
    , mExtendedChunkSize(0)
    , mCurrentChunkFinalSize(0)
    , mStreamPhase(StreamPhase::FTYP)
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

  template< typename Array >
  void perform(const Array &iArray)
  {
    doPerform(&iArray[0], iArray.size());
  }

  const std::string &currentChunk() const
  {
    return mCurrentChunk;
  }

  const std::string &moov() const
  {
    return mMoov;
  }

private:
  enum class StreamPhase
  {
    FTYP,
    MOOV,
    MOOF,
    MDAT
  };

  void doPerform(const char *iData, size_t iSize)
  {
    switch (mStreamPhase)
    {
    case StreamPhase::FTYP:
      processData(iData, iSize, [&]()
      {
        // do nothing with FTYP
        mCurrentChunk.clear();
        setStreamPhase(StreamPhase::MOOV);
      });
      break;
    case StreamPhase::MOOV:
      processData(iData, iSize, [&]()
      {
        mCurrentChunk.swap(mMoov);
        setStreamPhase(StreamPhase::MOOF);
      });
      break;
    case StreamPhase::MOOF:
      processData(iData, iSize, [&]()
      {
        mExtendedChunkSize = mCurrentChunkFinalSize;
        setStreamPhase(StreamPhase::MDAT);
      });
      break;
    case StreamPhase::MDAT:
      processData(iData, iSize, [&]()
      {
        {
          std::lock_guard< std::mutex > wLock(mDispatchMutex);
          notify(*this);
        }
        mExtendedChunkSize = 0;
        mCurrentChunk.clear();
        setStreamPhase(StreamPhase::MOOF);
      });
      break;
    }
  }

  template< typename Function >
  void processData(const char *iData, size_t iSize, Function iFunction)
  {
    if (mCurrentChunkFinalSize == 0)
    {
      for (size_t wIndex = 0; wIndex < iSize; ++wIndex)
      {
        *mCurrentChunkFinalSizeBufferIter = iData[wIndex];
        if (++mCurrentChunkFinalSizeBufferIter == mCurrentChunkFinalSizeBuffer.rend())
        {
          mCurrentChunkFinalSizeBufferIter = mCurrentChunkFinalSizeBuffer.rbegin();
          mCurrentChunkFinalSize = reinterpret_cast< decltype(mCurrentChunkFinalSize) & >(mCurrentChunkFinalSizeBuffer[0]) + mExtendedChunkSize;
          break;
        }
      }
    }
    if (mCurrentChunkFinalSize != 0)
    {
      auto wIndexToGo = mCurrentChunkFinalSize - mCurrentChunk.size();
      auto wFinalIndex = std::min(wIndexToGo, iSize);
      std::copy(&iData[0], &iData[wFinalIndex], std::back_inserter(mCurrentChunk));
      if (mCurrentChunk.size() == mCurrentChunkFinalSize)
      {
        iFunction();
        doPerform(&iData[wFinalIndex], iSize - wFinalIndex);
      }
    }
  }

  void setStreamPhase(StreamPhase iStreamPhase)
  {
    mCurrentChunkFinalSize = 0;
    mStreamPhase = iStreamPhase;
  }

  std::array< char, 4 > mCurrentChunkFinalSizeBuffer;
  std::array< char, 4 >::reverse_iterator mCurrentChunkFinalSizeBufferIter;
  size_t mCurrentChunkFinalSize;
  size_t mExtendedChunkSize;
  std::string mCurrentChunk;
  std::mutex mDispatchMutex;
  StreamPhase mStreamPhase;
  std::string mMoov;
};

class VideoConnector : public IObserver< VideoDispatcher >
{
public:
  VideoConnector(VideoDispatcher &iVideoDispatcher)
    : mVideoDispatcher(iVideoDispatcher)
    , mPlaying(false)
    , mMoovSent(false)
    , mSequenceNumber(0)
  {
    mVideoDispatcher.threadSafeAttach(this);
  }

  VideoConnector(const VideoConnector &iVideoConnector)
    : mVideoDispatcher(iVideoConnector.mVideoDispatcher)
    , mPlaying(iVideoConnector.mPlaying.load())
    , mMoovSent(iVideoConnector.mMoovSent.load())
    , mSequenceNumber(iVideoConnector.mSequenceNumber)
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
      if (!mMoovSent.exchange(true))
      {
        send(iVideoDispatcher.moov());
        //send(iVideoDispatcher.firstMoof());
      }
      send(iVideoDispatcher.currentChunk());
      //std::string wSmuds;
      //auto wMfhdLocation = iVideoDispatcher.currentChunk().find("mfhd");
      //std::copy(&iVideoDispatcher.currentChunk()[0], &iVideoDispatcher.currentChunk()[wMfhdLocation], std::back_inserter(wSmuds));
      //std::copy(&iVideoDispatcher.currentChunk()[wMfhdLocation], &iVideoDispatcher.currentChunk()[wMfhdLocation + 8], std::back_inserter(wSmuds));
      //auto wSequenceNumber = reinterpret_cast< char * >(&(++mSequenceNumber));
      //std::copy(boost::make_reverse_iterator(wSequenceNumber + 4), boost::make_reverse_iterator(wSequenceNumber), std::back_inserter(wSmuds));
      //std::copy(&iVideoDispatcher.currentChunk()[wMfhdLocation + 12], &iVideoDispatcher.currentChunk()[iVideoDispatcher.currentChunk().size()], std::back_inserter(wSmuds));
      //send(wSmuds);
    }
  }

private:
  std::atomic< bool > mPlaying;
  std::atomic< bool > mMoovSent;
  std::weak_ptr< ISender > mSender;
  VideoDispatcher &mVideoDispatcher;
  uint32_t mSequenceNumber;

  void send(const std::string &iMessage)
  {
    //static std::ofstream wTest("z:/MyPassport/PiCam/data/test3.mp4", std::ios::binary);
    auto wSender = mSender.lock();
    if (wSender != nullptr)
    {
      //wTest.write(iMessage.c_str(), iMessage.size());
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
  //std::thread wStdinDispatch([&]()
  //{
  //  //std::ifstream wIn("z:/MyPassport/PiCam/data/test.mp4", std::ios::binary);
  //  std::array< char, 1024 > wChunk;
  //  while (std::cin)
  //  //while (wIn)
  //  {
  //    std::cin.read(&wChunk[0], wChunk.size());
  //    //wIn.read(&wChunk[0], wChunk.size());
  //    wVideoDispatcher.perform(wChunk);
  //  }
  //});

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
  //wStdinDispatch.join();
}
