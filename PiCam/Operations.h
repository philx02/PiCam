#pragma once

#include "CommonTypedefs.h"

#ifndef WIN32
#include <signal.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <thread>
#include <iostream>

extern char **environ;

CameraPid openCameraChildProcess(const char *iH264FilePath)
{
  pid_t wCameraChild = 0;
  const char * wExecutableLocation = "/usr/bin/raspivid";
  const char * const wArgs[] = { wExecutableLocation, "-o", iH264FilePath, "-v", "-t", "0", "-i", "pause", "-s", "-vf", "-hf", "-fps", "25", "-n", nullptr };
  auto wResult = posix_spawn(&wCameraChild, wExecutableLocation, nullptr, nullptr, const_cast< char * const * >(wArgs), environ);
  if (wResult != 0)
  {
    std::cerr << "Error starting camera, errno is: " << errno << std::endl;
    exit(1);
  }
  return wCameraChild;
}

int openGpio(const char *iGpioPath)
{
  return open(iGpioPath, O_RDONLY);
}

pollfd openSwitchListener(int iGpioFd)
{
  pollfd wPollFd;
  wPollFd.fd = iGpioFd;
  wPollFd.events = POLLPRI | POLLERR;
  return wPollFd;
}

bool readGpio(pollfd &ioPollFd, int iGpioFd)
{
  auto wResult = poll(&ioPollFd, 1, -1);
  lseek(iGpioFd, 0, SEEK_SET);
  char wByte;
  read(iGpioFd, &wByte, 1);
  return wByte == '1';
}

void notifyCameraChildProcess(int iCameraChild)
{
  kill(iCameraChild, SIGUSR1);
}

void waitForCamera()
{
  std::this_thread::sleep_for(std::chrono::seconds(2));
}

#else

CameraPid openCameraChildProcess(const char *)
{
  return 0;
}

int openGpio(const char *)
{
  return 0;
}

int openSwitchListener(int)
{
  return 0;
}

template< typename T >
bool readGpio(T, int)
{
  static bool wValue = false;
  wValue = !wValue;
  return wValue;
}

template< typename T >
void notifyCameraChildProcess(T)
{
}

void waitForCamera()
{
}

#endif